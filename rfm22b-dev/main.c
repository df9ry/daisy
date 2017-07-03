/*
 * Simple synchronous userspace interface to SPI devices
 *
 * Copyright (C) 2006 SWAPP
 *	Andrea Paterniani <a.paterniani@swapp-eng.it>
 * Copyright (C) 2007 David Brownell (simplification, cleanup)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/compat.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/acpi.h>

#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>

#include <linux/uaccess.h>

#define CONFIG_OF     1
#define VERBOSE       1
#undef  CONFIG_COMPAT
#define DEBUG         1

/*
 * This supports access to SPI devices using normal userspace I/O calls.
 * Note that while traditional UNIX/POSIX I/O semantics are half duplex,
 * and often mask message boundaries, full SPI support requires full duplex
 * transfers.  There are several kinds of internal message boundaries to
 * handle chipselect management and other protocol options.
 *
 * SPI has a character major number assigned.  We allocate minor numbers
 * dynamically using a bitmask.  You must use hotplug tools, such as udev
 * (or mdev with busybox) to create and destroy the /dev/rfm22b_devB.C device
 * nodes, since there is no fixed association of minor numbers with any
 * particular SPI bus or device.
 */
#define SPIDEV_MAJOR			153	/* assigned */
#define N_SPI_MINORS			32	/* ... up to 256 */

static DECLARE_BITMAP(minors, N_SPI_MINORS);


/* Bit masks for spi_device.mode management.  Note that incorrect
 * settings for some settings can cause *lots* of trouble for other
 * devices on a shared bus:
 *
 *  - CS_HIGH ... this device will be active when it shouldn't be
 *  - 3WIRE ... when active, it won't behave as it should
 *  - NO_CS ... there will be no explicit message boundaries; this
 *	is completely incompatible with the shared bus model
 *  - READY ... transfers may proceed when they shouldn't.
 *
 * REVISIT should changing those flags be privileged?
 */
#define SPI_MODE_MASK		(SPI_CPHA | SPI_CPOL | SPI_CS_HIGH \
				| SPI_LSB_FIRST | SPI_3WIRE | SPI_LOOP \
				| SPI_NO_CS | SPI_READY | SPI_TX_DUAL \
				| SPI_TX_QUAD | SPI_RX_DUAL | SPI_RX_QUAD)

struct rfm22b_dev_data {
	dev_t			devt;
	spinlock_t		spi_lock;
	struct spi_device	*spi;
	struct list_head	device_entry;

	/* TX/RX buffers are NULL unless this device is open (users > 0) */
	struct mutex		buf_lock;
	unsigned		users;
	u8			*tx_buffer;
	u8			*rx_buffer;
	u32			speed_hz;
};

static LIST_HEAD(device_list);
static DEFINE_MUTEX(device_list_lock);

static unsigned bufsiz = 4096;
module_param(bufsiz, uint, S_IRUGO);
MODULE_PARM_DESC(bufsiz, "data bytes in biggest supported SPI message");

/*-------------------------------------------------------------------------*/

static ssize_t
rfm22b_dev_sync(struct rfm22b_dev_data *rfm22b_dev, struct spi_message *message)
{
	DECLARE_COMPLETION_ONSTACK(done);
	int status;
	struct spi_device *spi;

	spin_lock_irq(&rfm22b_dev->spi_lock);
	spi = rfm22b_dev->spi;
	spin_unlock_irq(&rfm22b_dev->spi_lock);

	if (spi == NULL)
		status = -ESHUTDOWN;
	else
		status = spi_sync(spi, message);

	if (status == 0)
		status = message->actual_length;

	return status;
}

static inline ssize_t
rfm22b_dev_sync_write(struct rfm22b_dev_data *rfm22b_dev, size_t len)
{
	struct spi_transfer	t = {
			.tx_buf		= rfm22b_dev->tx_buffer,
			.len		= len,
			.speed_hz	= rfm22b_dev->speed_hz,
		};
	struct spi_message	m;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	return rfm22b_dev_sync(rfm22b_dev, &m);
}

static inline ssize_t
rfm22b_dev_sync_read(struct rfm22b_dev_data *rfm22b_dev, size_t len)
{
	struct spi_transfer	t = {
			.rx_buf		= rfm22b_dev->rx_buffer,
			.len		= len,
			.speed_hz	= rfm22b_dev->speed_hz,
		};
	struct spi_message	m;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	return rfm22b_dev_sync(rfm22b_dev, &m);
}

/*-------------------------------------------------------------------------*/

/* Read-only message with current device setup */
static ssize_t
rfm22b_dev_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	struct rfm22b_dev_data	*rfm22b_dev;
	ssize_t			status = 0;

	printk(KERN_DEBUG "RFM22B-DEV: READ\n");
	/* chipselect only toggles at start or end of operation */
	if (count > bufsiz)
		return -EMSGSIZE;

	rfm22b_dev = filp->private_data;

	mutex_lock(&rfm22b_dev->buf_lock);
	status = rfm22b_dev_sync_read(rfm22b_dev, count);
	if (status > 0) {
		unsigned long	missing;

		missing = copy_to_user(buf, rfm22b_dev->rx_buffer, status);
		if (missing == status)
			status = -EFAULT;
		else
			status = status - missing;
	}
	mutex_unlock(&rfm22b_dev->buf_lock);

	return status;
}

/* Write-only message with current device setup */
static ssize_t
rfm22b_dev_write(struct file *filp, const char __user *buf,
		size_t count, loff_t *f_pos)
{
	struct rfm22b_dev_data	*rfm22b_dev;
	ssize_t			status = 0;
	unsigned long		missing;

	printk(KERN_DEBUG "RFM22B-DEV: WRITE\n");
	/* chipselect only toggles at start or end of operation */
	if (count > bufsiz)
		return -EMSGSIZE;

	rfm22b_dev = filp->private_data;

	mutex_lock(&rfm22b_dev->buf_lock);
	missing = copy_from_user(rfm22b_dev->tx_buffer, buf, count);
	if (missing == 0)
		status = rfm22b_dev_sync_write(rfm22b_dev, count);
	else
		status = -EFAULT;
	mutex_unlock(&rfm22b_dev->buf_lock);

	return status;
}

static int rfm22b_dev_message(struct rfm22b_dev_data *rfm22b_dev,
		struct spi_ioc_transfer *u_xfers, unsigned n_xfers)
{
	struct spi_message	msg;
	struct spi_transfer	*k_xfers;
	struct spi_transfer	*k_tmp;
	struct spi_ioc_transfer *u_tmp;
	unsigned		n, total, tx_total, rx_total;
	u8			*tx_buf, *rx_buf;
	int			status = -EFAULT;

	printk(KERN_DEBUG "RFM22B-DEV: MESSAGE\n");
	spi_message_init(&msg);
	k_xfers = kcalloc(n_xfers, sizeof(*k_tmp), GFP_KERNEL);
	if (k_xfers == NULL)
		return -ENOMEM;

	/* Construct spi_message, copying any tx data to bounce buffer.
	 * We walk the array of user-provided transfers, using each one
	 * to initialize a kernel version of the same transfer.
	 */
	tx_buf = rfm22b_dev->tx_buffer;
	rx_buf = rfm22b_dev->rx_buffer;
	total = 0;
	tx_total = 0;
	rx_total = 0;
	printk(KERN_DEBUG "RFM22B-DEV: MESSAGE[1]\n");
	for (n = n_xfers, k_tmp = k_xfers, u_tmp = u_xfers;
			n;
			n--, k_tmp++, u_tmp++) {
		k_tmp->len = u_tmp->len;
		printk(KERN_DEBUG "RFM22B-DEV: MESSAGE[LOOP]\n");

		total += k_tmp->len;
		/* Since the function returns the total length of transfers
		 * on success, restrict the total to positive int values to
		 * avoid the return value looking like an error.  Also check
		 * each transfer length to avoid arithmetic overflow.
		 */
		if (total > INT_MAX || k_tmp->len > INT_MAX) {
			status = -EMSGSIZE;
			goto done;
		}

		if (u_tmp->rx_buf) {
			/* this transfer needs space in RX bounce buffer */
			rx_total += k_tmp->len;
			if (rx_total > bufsiz) {
				status = -EMSGSIZE;
				goto done;
			}
			k_tmp->rx_buf = rx_buf;
			if (!access_ok(VERIFY_WRITE, (u8 __user *)
						(uintptr_t) u_tmp->rx_buf,
						u_tmp->len))
				goto done;
			rx_buf += k_tmp->len;
		}
		if (u_tmp->tx_buf) {
			/* this transfer needs space in TX bounce buffer */
			tx_total += k_tmp->len;
			if (tx_total > bufsiz) {
				status = -EMSGSIZE;
				goto done;
			}
			k_tmp->tx_buf = tx_buf;
			if (copy_from_user(tx_buf, (const u8 __user *)
						(uintptr_t) u_tmp->tx_buf,
					u_tmp->len))
				goto done;
			tx_buf += k_tmp->len;
		}

		k_tmp->cs_change = !!u_tmp->cs_change;
		k_tmp->tx_nbits = u_tmp->tx_nbits;
		k_tmp->rx_nbits = u_tmp->rx_nbits;
		k_tmp->bits_per_word = u_tmp->bits_per_word;
		k_tmp->delay_usecs = u_tmp->delay_usecs;
		k_tmp->speed_hz = u_tmp->speed_hz;
		if (!k_tmp->speed_hz)
			k_tmp->speed_hz = rfm22b_dev->speed_hz;
#ifdef VERBOSE
		dev_dbg(&rfm22b_dev->spi->dev,
			"  xfer len %u %s%s%s%dbits %u usec %uHz\n",
			u_tmp->len,
			u_tmp->rx_buf ? "rx " : "",
			u_tmp->tx_buf ? "tx " : "",
			u_tmp->cs_change ? "cs " : "",
			u_tmp->bits_per_word ? : rfm22b_dev->spi->bits_per_word,
			u_tmp->delay_usecs,
			u_tmp->speed_hz ? : rfm22b_dev->spi->max_speed_hz);
		printk(KERN_DEBUG "xfer len %u %s%s%s%dbits %u usec %uHz\n",
			u_tmp->len,
			u_tmp->rx_buf ? "rx " : "",
			u_tmp->tx_buf ? "tx " : "",
			u_tmp->cs_change ? "cs " : "",
			u_tmp->bits_per_word ? : rfm22b_dev->spi->bits_per_word,
			u_tmp->delay_usecs,
			u_tmp->speed_hz ? : rfm22b_dev->spi->max_speed_hz);
#endif
		spi_message_add_tail(k_tmp, &msg);
	}

	status = rfm22b_dev_sync(rfm22b_dev, &msg);
	if (status < 0)
		goto done;

	{
		struct spi_ioc_transfer *u = u_xfers;
		printk(KERN_DEBUG "TX:%02x|%02x|%02x\n",
				((uint8_t*)u->tx_buf)[0],
				((uint8_t*)u->tx_buf)[1],
				((uint8_t*)u->tx_buf)[2]);
		printk(KERN_DEBUG "RX:%02x|%02x|%02x\n",
				((uint8_t*)u->rx_buf)[0],
				((uint8_t*)u->rx_buf)[1],
				((uint8_t*)u->rx_buf)[2]);
	}
	/* copy any rx data out of bounce buffer */
	rx_buf = rfm22b_dev->rx_buffer;
	for (n = n_xfers, u_tmp = u_xfers; n; n--, u_tmp++) {
		if (u_tmp->rx_buf) {
			if (__copy_to_user((u8 __user *)
					(uintptr_t) u_tmp->rx_buf, rx_buf,
					u_tmp->len)) {
				status = -EFAULT;
				goto done;
			}
			rx_buf += u_tmp->len;
		}
	}
	status = total;

done:
	kfree(k_xfers);
	return status;
}

static struct spi_ioc_transfer *
rfm22b_dev_get_ioc_message(unsigned int cmd, struct spi_ioc_transfer __user *u_ioc,
		unsigned *n_ioc)
{
	struct spi_ioc_transfer	*ioc;
	u32	tmp;

	printk(KERN_DEBUG "RFM22B-DEV: IOCTL %x\n", cmd);
	/* Check type, command number and direction */
	if (_IOC_TYPE(cmd) != SPI_IOC_MAGIC
			|| _IOC_NR(cmd) != _IOC_NR(SPI_IOC_MESSAGE(0))
			|| _IOC_DIR(cmd) != _IOC_WRITE)
		return ERR_PTR(-ENOTTY);

	tmp = _IOC_SIZE(cmd);
	if ((tmp % sizeof(struct spi_ioc_transfer)) != 0)
		return ERR_PTR(-EINVAL);
	*n_ioc = tmp / sizeof(struct spi_ioc_transfer);
	if (*n_ioc == 0)
		return NULL;

	/* copy into scratch area */
	ioc = kmalloc(tmp, GFP_KERNEL);
	if (!ioc)
		return ERR_PTR(-ENOMEM);
	if (__copy_from_user(ioc, u_ioc, tmp)) {
		kfree(ioc);
		return ERR_PTR(-EFAULT);
	}
	return ioc;
}

static long
rfm22b_dev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int			err = 0;
	int			retval = 0;
	struct rfm22b_dev_data	*rfm22b_dev;
	struct spi_device	*spi;
	u32			tmp;
	unsigned		n_ioc;
	struct spi_ioc_transfer	*ioc;

	printk(KERN_DEBUG "RFM22B-DEV: IOCTL %x\n", cmd);
	/* Check type and command number */
	if (_IOC_TYPE(cmd) != SPI_IOC_MAGIC)
		return -ENOTTY;


	/* Check access direction once here; don't repeat below.
	 * IOC_DIR is from the user perspective, while access_ok is
	 * from the kernel perspective; so they look reversed.
	 */
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE,
				(void __user *)arg, _IOC_SIZE(cmd));
	if (err == 0 && _IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ,
				(void __user *)arg, _IOC_SIZE(cmd));
	if (err)
		return -EFAULT;

	/* guard against device removal before, or while,
	 * we issue this ioctl.
	 */
	rfm22b_dev = filp->private_data;
	spin_lock_irq(&rfm22b_dev->spi_lock);
	spi = spi_dev_get(rfm22b_dev->spi);
	spin_unlock_irq(&rfm22b_dev->spi_lock);

	if (spi == NULL)
		return -ESHUTDOWN;

	/* use the buffer lock here for triple duty:
	 *  - prevent I/O (from us) so calling spi_setup() is safe;
	 *  - prevent concurrent SPI_IOC_WR_* from morphing
	 *    data fields while SPI_IOC_RD_* reads them;
	 *  - SPI_IOC_MESSAGE needs the buffer locked "normally".
	 */
	mutex_lock(&rfm22b_dev->buf_lock);

	switch (cmd) {
	/* read requests */
	case SPI_IOC_RD_MODE:
		retval = __put_user(spi->mode & SPI_MODE_MASK,
					(__u8 __user *)arg);
		break;
	case SPI_IOC_RD_MODE32:
		retval = __put_user(spi->mode & SPI_MODE_MASK,
					(__u32 __user *)arg);
		break;
	case SPI_IOC_RD_LSB_FIRST:
		retval = __put_user((spi->mode & SPI_LSB_FIRST) ?  1 : 0,
					(__u8 __user *)arg);
		break;
	case SPI_IOC_RD_BITS_PER_WORD:
		retval = __put_user(spi->bits_per_word, (__u8 __user *)arg);
		break;
	case SPI_IOC_RD_MAX_SPEED_HZ:
		retval = __put_user(rfm22b_dev->speed_hz, (__u32 __user *)arg);
		break;

	/* write requests */
	case SPI_IOC_WR_MODE:
	case SPI_IOC_WR_MODE32:
		if (cmd == SPI_IOC_WR_MODE)
			retval = __get_user(tmp, (u8 __user *)arg);
		else
			retval = __get_user(tmp, (u32 __user *)arg);
		if (retval == 0) {
			u32	save = spi->mode;

			if (tmp & ~SPI_MODE_MASK) {
				retval = -EINVAL;
				break;
			}

			tmp |= spi->mode & ~SPI_MODE_MASK;
			spi->mode = (u16)tmp;
			retval = spi_setup(spi);
			if (retval < 0)
				spi->mode = save;
			else
				dev_dbg(&spi->dev, "spi mode %x\n", tmp);
		}
		break;
	case SPI_IOC_WR_LSB_FIRST:
		retval = __get_user(tmp, (__u8 __user *)arg);
		if (retval == 0) {
			u32	save = spi->mode;

			if (tmp)
				spi->mode |= SPI_LSB_FIRST;
			else
				spi->mode &= ~SPI_LSB_FIRST;
			retval = spi_setup(spi);
			if (retval < 0)
				spi->mode = save;
			else
				dev_dbg(&spi->dev, "%csb first\n",
						tmp ? 'l' : 'm');
		}
		break;
	case SPI_IOC_WR_BITS_PER_WORD:
		retval = __get_user(tmp, (__u8 __user *)arg);
		if (retval == 0) {
			u8	save = spi->bits_per_word;

			spi->bits_per_word = tmp;
			retval = spi_setup(spi);
			if (retval < 0)
				spi->bits_per_word = save;
			else
				dev_dbg(&spi->dev, "%d bits per word\n", tmp);
		}
		break;
	case SPI_IOC_WR_MAX_SPEED_HZ:
		retval = __get_user(tmp, (__u32 __user *)arg);
		if (retval == 0) {
			u32	save = spi->max_speed_hz;

			spi->max_speed_hz = tmp;
			retval = spi_setup(spi);
			if (retval >= 0)
				rfm22b_dev->speed_hz = tmp;
			else
				dev_dbg(&spi->dev, "%d Hz (max)\n", tmp);
			spi->max_speed_hz = save;
		}
		break;

	default:
		/* segmented and/or full-duplex I/O request */
		/* Check message and copy into scratch area */
		ioc = rfm22b_dev_get_ioc_message(cmd,
				(struct spi_ioc_transfer __user *)arg, &n_ioc);
		if (IS_ERR(ioc)) {
			retval = PTR_ERR(ioc);
			break;
		}
		if (!ioc)
			break;	/* n_ioc is also 0 */

		/* translate to spi_message, execute */
		retval = rfm22b_dev_message(rfm22b_dev, ioc, n_ioc);
		kfree(ioc);
		break;
	}

	mutex_unlock(&rfm22b_dev->buf_lock);
	spi_dev_put(spi);
	return retval;
}

#ifdef CONFIG_COMPAT
static long
rfm22b_dev_compat_ioc_message(struct file *filp, unsigned int cmd,
		unsigned long arg)
{
	struct spi_ioc_transfer __user	*u_ioc;
	int				retval = 0;
	struct rfm22b_dev_data		*rfm22b_dev;
	struct spi_device		*spi;
	unsigned			n_ioc, n;
	struct spi_ioc_transfer		*ioc;

	u_ioc = (struct spi_ioc_transfer __user *) compat_ptr(arg);
	if (!access_ok(VERIFY_READ, u_ioc, _IOC_SIZE(cmd)))
		return -EFAULT;

	/* guard against device removal before, or while,
	 * we issue this ioctl.
	 */
	rfm22b_dev = filp->private_data;
	spin_lock_irq(&rfm22b_dev->spi_lock);
	spi = spi_dev_get(rfm22b_dev->spi);
	spin_unlock_irq(&rfm22b_dev->spi_lock);

	if (spi == NULL)
		return -ESHUTDOWN;

	/* SPI_IOC_MESSAGE needs the buffer locked "normally" */
	mutex_lock(&rfm22b_dev->buf_lock);

	/* Check message and copy into scratch area */
	ioc = rfm22b_dev_get_ioc_message(cmd, u_ioc, &n_ioc);
	if (IS_ERR(ioc)) {
		retval = PTR_ERR(ioc);
		goto done;
	}
	if (!ioc)
		goto done;	/* n_ioc is also 0 */

	/* Convert buffer pointers */
	for (n = 0; n < n_ioc; n++) {
		ioc[n].rx_buf = (uintptr_t) compat_ptr(ioc[n].rx_buf);
		ioc[n].tx_buf = (uintptr_t) compat_ptr(ioc[n].tx_buf);
	}

	/* translate to spi_message, execute */
	retval = rfm22b_dev_message(rfm22b_dev, ioc, n_ioc);
	kfree(ioc);

done:
	mutex_unlock(&rfm22b_dev->buf_lock);
	spi_dev_put(spi);
	return retval;
}

static long
rfm22b_dev_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	if (_IOC_TYPE(cmd) == SPI_IOC_MAGIC
			&& _IOC_NR(cmd) == _IOC_NR(SPI_IOC_MESSAGE(0))
			&& _IOC_DIR(cmd) == _IOC_WRITE)
		return rfm22b_dev_compat_ioc_message(filp, cmd, arg);

	return rfm22b_dev_ioctl(filp, cmd, (unsigned long)compat_ptr(arg));
}
#else
#define rfm22b_dev_compat_ioctl NULL
#endif /* CONFIG_COMPAT */

static int rfm22b_dev_open(struct inode *inode, struct file *filp)
{
	struct rfm22b_dev_data	*rfm22b_dev;
	int			status = -ENXIO;

	printk(KERN_DEBUG "RFM22B-DEV: OPEN");
	mutex_lock(&device_list_lock);

	list_for_each_entry(rfm22b_dev, &device_list, device_entry) {
		if (rfm22b_dev->devt == inode->i_rdev) {
			status = 0;
			break;
		}
	}

	if (status) {
		pr_debug("rfm22b_dev: nothing for minor %d\n", iminor(inode));
		goto err_find_dev;
	}

	if (!rfm22b_dev->tx_buffer) {
		rfm22b_dev->tx_buffer = kmalloc(bufsiz, GFP_KERNEL);
		if (!rfm22b_dev->tx_buffer) {
			dev_dbg(&rfm22b_dev->spi->dev, "open/ENOMEM\n");
			status = -ENOMEM;
			goto err_find_dev;
		}
	}

	if (!rfm22b_dev->rx_buffer) {
		rfm22b_dev->rx_buffer = kmalloc(bufsiz, GFP_KERNEL);
		if (!rfm22b_dev->rx_buffer) {
			dev_dbg(&rfm22b_dev->spi->dev, "open/ENOMEM\n");
			status = -ENOMEM;
			goto err_alloc_rx_buf;
		}
	}

	rfm22b_dev->users++;
	filp->private_data = rfm22b_dev;
	nonseekable_open(inode, filp);

	mutex_unlock(&device_list_lock);
	return 0;

err_alloc_rx_buf:
	kfree(rfm22b_dev->tx_buffer);
	rfm22b_dev->tx_buffer = NULL;
err_find_dev:
	mutex_unlock(&device_list_lock);
	return status;
}

static int rfm22b_dev_release(struct inode *inode, struct file *filp)
{
	struct rfm22b_dev_data	*rfm22b_dev;

	printk(KERN_DEBUG "RFM22B-DEV: RELEASE\n");
	mutex_lock(&device_list_lock);
	rfm22b_dev = filp->private_data;
	filp->private_data = NULL;

	/* last close? */
	rfm22b_dev->users--;
	if (!rfm22b_dev->users) {
		int		dofree;

		kfree(rfm22b_dev->tx_buffer);
		rfm22b_dev->tx_buffer = NULL;

		kfree(rfm22b_dev->rx_buffer);
		rfm22b_dev->rx_buffer = NULL;

		spin_lock_irq(&rfm22b_dev->spi_lock);
		if (rfm22b_dev->spi)
			rfm22b_dev->speed_hz = rfm22b_dev->spi->max_speed_hz;

		/* ... after we unbound from the underlying device? */
		dofree = (rfm22b_dev->spi == NULL);
		spin_unlock_irq(&rfm22b_dev->spi_lock);

		if (dofree)
			kfree(rfm22b_dev);
	}
	mutex_unlock(&device_list_lock);

	return 0;
}

static const struct file_operations rfm22b_dev_fops = {
	.owner =	THIS_MODULE,
	/* REVISIT switch to aio primitives, so that userspace
	 * gets more complete API coverage.  It'll simplify things
	 * too, except for the locking.
	 */
	.write =	rfm22b_dev_write,
	.read =		rfm22b_dev_read,
	.unlocked_ioctl = rfm22b_dev_ioctl,
	.compat_ioctl = rfm22b_dev_compat_ioctl,
	.open =		rfm22b_dev_open,
	.release =	rfm22b_dev_release,
	.llseek =	no_llseek,
};

/*-------------------------------------------------------------------------*/

/* The main reason to have this class is to make mdev/udev create the
 * /dev/rfm22b_devB.C character device nodes exposing our userspace API.
 * It also simplifies memory management.
 */

static struct class *rfm22b_dev_class;

#ifdef CONFIG_OF
static const struct of_device_id rfm22b_dev_dt_ids[] = {
	{ .compatible = "hoperf,rfm22b" },
	{ .compatible = "rfm22b-dev" },
	{ .compatible = "spidev" },
	{},
};
MODULE_DEVICE_TABLE(of, rfm22b_dev_dt_ids);
#endif

#ifdef CONFIG_ACPI

/* Dummy SPI devices not to be used in production systems */
#define SPIDEV_ACPI_DUMMY	1

static const struct acpi_device_id rfm22b_dev_acpi_ids[] = {
	/*
	 * The ACPI SPT000* devices are only meant for development and
	 * testing. Systems used in production should have a proper ACPI
	 * description of the connected peripheral and they should also use
	 * a proper driver instead of poking directly to the SPI bus.
	 */
	{ "SPT0001", SPIDEV_ACPI_DUMMY },
	{ "SPT0002", SPIDEV_ACPI_DUMMY },
	{ "SPT0003", SPIDEV_ACPI_DUMMY },
	{},
};
MODULE_DEVICE_TABLE(acpi, rfm22b_dev_acpi_ids);

static void rfm22b_dev_probe_acpi(struct spi_device *spi)
{
	const struct acpi_device_id *id;

	if (!has_acpi_companion(&spi->dev))
		return;

	id = acpi_match_device(rfm22b_dev_acpi_ids, &spi->dev);
	if (WARN_ON(!id))
		return;

	if (id->driver_data == SPIDEV_ACPI_DUMMY)
		dev_warn(&spi->dev, "do not use this driver in production systems!\n");
}
#else
static inline void rfm22b_dev_probe_acpi(struct spi_device *spi) {}
#endif

/*-------------------------------------------------------------------------*/

static int rfm22b_dev_probe(struct spi_device *spi)
{
	struct rfm22b_dev_data	*rfm22b_dev;
	int			status;
	unsigned long		minor;

	printk(KERN_DEBUG "RFM22B-DEV: PROBE\n");
	/*
	 * rfm22b_dev should never be referenced in DT without a specific
	 * compatible string, it is a Linux implementation thing
	 * rather than a description of the hardware.
	 */
	if (spi->dev.of_node && !of_match_device(rfm22b_dev_dt_ids, &spi->dev)) {
		dev_err(&spi->dev, "buggy DT: rfm22b_dev listed directly in DT\n");
		WARN_ON(spi->dev.of_node &&
			!of_match_device(rfm22b_dev_dt_ids, &spi->dev));
	}

	rfm22b_dev_probe_acpi(spi);

	/* Allocate driver data */
	rfm22b_dev = kzalloc(sizeof(*rfm22b_dev), GFP_KERNEL);
	if (!rfm22b_dev)
		return -ENOMEM;

	/* Initialize the driver data */
	rfm22b_dev->spi = spi;
	spin_lock_init(&rfm22b_dev->spi_lock);
	mutex_init(&rfm22b_dev->buf_lock);

	INIT_LIST_HEAD(&rfm22b_dev->device_entry);

	/* If we can allocate a minor number, hook up this device.
	 * Reusing minors is fine so long as udev or mdev is working.
	 */
	mutex_lock(&device_list_lock);
	minor = find_first_zero_bit(minors, N_SPI_MINORS);
	if (minor < N_SPI_MINORS) {
		struct device *dev;

		rfm22b_dev->devt = MKDEV(SPIDEV_MAJOR, minor);
		dev = device_create(rfm22b_dev_class, &spi->dev, rfm22b_dev->devt,
				    rfm22b_dev, "spidev%d.%d",
				    spi->master->bus_num, spi->chip_select);
		status = PTR_ERR_OR_ZERO(dev);
	} else {
		dev_dbg(&spi->dev, "no minor number available!\n");
		status = -ENODEV;
	}
	if (status == 0) {
		set_bit(minor, minors);
		list_add(&rfm22b_dev->device_entry, &device_list);
	}
	mutex_unlock(&device_list_lock);

	rfm22b_dev->speed_hz = spi->max_speed_hz;

	if (status == 0)
		spi_set_drvdata(spi, rfm22b_dev);
	else
		kfree(rfm22b_dev);

	return status;
}

static int rfm22b_dev_remove(struct spi_device *spi)
{
	struct rfm22b_dev_data	*rfm22b_dev = spi_get_drvdata(spi);

	printk(KERN_DEBUG "RFM22B-DEV: REMOVE\n");
	/* make sure ops on existing fds can abort cleanly */
	spin_lock_irq(&rfm22b_dev->spi_lock);
	rfm22b_dev->spi = NULL;
	spin_unlock_irq(&rfm22b_dev->spi_lock);

	/* prevent new opens */
	mutex_lock(&device_list_lock);
	list_del(&rfm22b_dev->device_entry);
	device_destroy(rfm22b_dev_class, rfm22b_dev->devt);
	clear_bit(MINOR(rfm22b_dev->devt), minors);
	if (rfm22b_dev->users == 0)
		kfree(rfm22b_dev);
	mutex_unlock(&device_list_lock);

	return 0;
}

static struct spi_driver rfm22b_dev_spi_driver = {
	.driver = {
		.name =		"rfm22b-dev",
		.of_match_table = of_match_ptr(rfm22b_dev_dt_ids),
		.acpi_match_table = ACPI_PTR(rfm22b_dev_acpi_ids),
	},
	.probe =	rfm22b_dev_probe,
	.remove =	rfm22b_dev_remove,

	/* NOTE:  suspend/resume methods are not necessary here.
	 * We don't do anything except pass the requests to/from
	 * the underlying controller.  The refrigerator handles
	 * most issues; the controller driver handles the rest.
	 */
};

/*-------------------------------------------------------------------------*/

static int __init rfm22b_dev_init(void)
{
	int status;

	printk(KERN_DEBUG "RFM22B-DEV: INIT\n");
	/* Claim our 256 reserved device numbers.  Then register a class
	 * that will key udev/mdev to add/remove /dev nodes.  Last, register
	 * the driver which manages those device numbers.
	 */
	BUILD_BUG_ON(N_SPI_MINORS > 256);
	status = register_chrdev(SPIDEV_MAJOR, "rfm22b", &rfm22b_dev_fops);
	if (status < 0)
		return status;

	rfm22b_dev_class = class_create(THIS_MODULE, "rfm22b-dev");
	if (IS_ERR(rfm22b_dev_class)) {
		unregister_chrdev(SPIDEV_MAJOR, rfm22b_dev_spi_driver.driver.name);
		return PTR_ERR(rfm22b_dev_class);
	}

	status = spi_register_driver(&rfm22b_dev_spi_driver);
	if (status < 0) {
		class_destroy(rfm22b_dev_class);
		unregister_chrdev(SPIDEV_MAJOR, rfm22b_dev_spi_driver.driver.name);
	}
	return status;
}
module_init(rfm22b_dev_init);

static void __exit rfm22b_dev_exit(void)
{
	printk(KERN_DEBUG "RFM22B-DEV: EXIT\n");
	spi_unregister_driver(&rfm22b_dev_spi_driver);
	class_destroy(rfm22b_dev_class);
	unregister_chrdev(SPIDEV_MAJOR, rfm22b_dev_spi_driver.driver.name);
}
module_exit(rfm22b_dev_exit);

MODULE_AUTHOR("Tania Hagn <Tania@DF9RY.de> - Template:"
		"Andrea Paterniani, <a.paterniani@swapp-eng.it>");
MODULE_DESCRIPTION("User mode SPI device interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:rfm22b-dev");
