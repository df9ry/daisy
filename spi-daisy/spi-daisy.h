/* Copyright 2017 Tania Hagn
 *
 * This file is part of Daisy.
 *
 *    Daisy is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    Daisy is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Daisy.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SPI_DAISY_H_
#define _SPI_DAISY_H_

#include <linux/module.h>

#define DRV_NAME	"spi-daisy"

#define MIN_SPEED_HZ         50000
#define MAX_SPEED_HZ       5000000
#define N_SLOTS                  2
#define MAX_PKG_LEN            256

#define DEFAULT_RX_QUEUE_SIZE   16
#define DEFAULT_TX_QUEUE_SIZE   16
#define DEFAULT_TX_LOW_WATER_DN  2
#define DEFAULT_TX_LOW_WATER_UP  6
#define DEFAULT_WATCHDOG        10

struct daisy_dev;
struct daisy_spi;
struct sk_buff;
struct net_device_stats;

/**
 * Open a daisy device. Use daisy_close_handle() to release the device.
 * @param slot SPI slot (chip select line) of the SPI device.
 */
extern struct daisy_dev *daisy_open_device(uint16_t slot);

/**
 * Start IO to the chip.
 * @param dd    Device to register the net_dev_stats for.
 */
extern void daisy_device_up(struct daisy_dev *dd);

/**
 * Suspend IO to the chip.
 * @param dd    Device to register the net_dev_stats for.
 */
extern void daisy_device_down(struct daisy_dev *dd);

/**
 * Register the net_device_stats for the device.
 * @param dd    Device to register the net_dev_stats for.
 * @param stats Struct net_device_stats for the device.
 */
extern void daisy_register_stats(struct daisy_dev *dd,
		struct net_device_stats *stats);

/**
 * Common utility for SPI IO.
 * @dd Daisy device for the transfer.
 * @tx SPI transmit buffer.
 * @rx SPI receive buffer.
 * @cb Size of tx and rx buffer.
 */
extern void daisy_transfer(struct daisy_dev *dd, const volatile uint8_t *tx,
						   volatile uint8_t *rx, size_t cb);

/**
 * Close a daisy device previously opened with daisy_open_device().
 */
extern void daisy_close_device(struct daisy_dev *bs);

/**
 * Synchronous read from the daisy device.
 * @param dd         Daisy device to read from.
 * @param pb         Pointer to a suitable receive buffer.
 * @param cb         Size of the receive buffer.
 * @return           Received sk_buff or NULL in the case of error.
 */
extern struct sk_buff *daisy_read(struct daisy_dev *dd);

/**
 * Synchronized write the daisy device.
 * @param dd         Daisy device to write to.
 * @param skb        Socket buffer to write. If daisy_write() succeeds
 *                   you take the responsibility of skb over to spi-daisy.
 * @param priority   When set send messages before messages sent without
 *                   priority.
 * @return           Number of bytes written or a negative error code on error.
 */
extern int daisy_write(struct daisy_dev *dd, struct sk_buff *skb,
					   bool priority);

/**
 * Check if a subsequent write would be successful.
 * @param dd         Daisy device to check.
 * @return           Value != 0, if a write would not block.
 */
extern bool daisy_can_write(struct daisy_dev *dd);

/**
 * Try write the daisy device.
 * @param dd         Daisy device to write to.
 * @param skb        Socket buffer to write. If daisy_write() succeeds
 *                   you take the responsibility of skb over to spi-daisy.
 * @param priority   When set send messages before messages sent without
 *                   priority.
 * @return           Number of bytes written, -EAGAIN when no write buffer
 *                   is available or another negative error code on error.
 */
extern int daisy_try_write(struct daisy_dev *dd, struct sk_buff *skb,
					       bool priority);

/**
 * Interrupt a pending read.
 * @param dd         Daisy device to interrupt.
 */
extern void daisy_interrupt_read(struct daisy_dev *dd);

/**
 * Get the controller for a daisy device.
 */
extern struct daisy_spi *daisy_get_controller(struct daisy_dev *dev);

/**
 * Set the speed of a controller. This circumvents the automatic speed lock
 * taken by the SPI subsystem.
 */
extern uint32_t daisy_set_speed(struct daisy_spi *spi,
									   uint32_t   spi_hz);

/**
 * Lock speed for a controller, so that no automatic adaption by the
 * SPI subsystem can be occur.
 */
extern void daisy_lock_speed(struct daisy_spi *spi);

/**
 * Unlock the automatic speed adaption.
 */
extern void daisy_unlock_speed(struct daisy_spi *spi);

/**
 * Set multiple bits in 8 bit register.
 */
static inline void daisy_set_mbits8(struct daisy_dev *dd,
								   	   	   uint8_t    reg,
										   uint8_t    mask,
										   uint8_t    val)
{
	uint8_t x1[2] = { reg,  0x00 };
	uint8_t x2[2] = { 0x00, 0x00 };
	daisy_transfer(dd, x1, x2, 2);
	x2[0] = reg | 0x80;
	x2[1] &= ~mask;
	x2[1] |= val;
	daisy_transfer(dd, x2, x1, 2);
}

/**
 * Set bits in 8 bit register.
 */
static inline void daisy_set_bits8(struct daisy_dev *dd,
								   	   	  uint8_t    reg,
										  uint8_t    mask)
{
	uint8_t x1[2] = { reg,  0x00 };
	uint8_t x2[2] = { 0x00, 0x00 };
	daisy_transfer(dd, x1, x2, 2);
	x2[0] = reg | 0x80;
	x2[1] |= mask;
	daisy_transfer(dd, x2, x1, 2);
}

/**
 * Clear bits in 8 bit register.
 */
static inline void daisy_clr_bits8(struct daisy_dev *dd,
								   	   	  uint8_t    reg,
										  uint8_t    mask)
{
	uint8_t x1[2] = { reg,  0x00 };
	uint8_t x2[2] = { 0x00, 0x00 };
	daisy_transfer(dd, x1, x2, 2);
	x2[0] = reg | 0x80;
	x2[1] &= ~mask;
	daisy_transfer(dd, x2, x1, 2);
}

/**
 * Direct write 8 bit register.
 */
static inline void daisy_set_register8(struct daisy_dev *dd,
								       	   	  uint8_t    reg,
											  uint8_t    val)
{
	uint8_t tx[2] = { reg | 0x80, val  };
	uint8_t rx[2] = { 0x00,       0x00 };
	daisy_transfer(dd, tx, rx, 2);
}

/**
 * Direct read 8 bit register.
 */
static inline uint8_t daisy_get_register8(struct daisy_dev *dd,
										  	  	 uint8_t    reg)
{
	uint8_t tx[2] = { reg,  0x00 };
	uint8_t rx[2] = { 0x00, 0x00 };
	daisy_transfer(dd, tx, rx, 2);
	return rx[1];
}

/**
 * Direct write 16 bit register.
 */
static inline void daisy_set_register16(struct daisy_dev *dd,
								        	   uint8_t    reg,
											   uint16_t   val)
{
	uint8_t tx[3] = { reg | 0x80, (val >> 8) & 0x00ff, val & 0x00ff  };
	uint8_t rx[3] = { 0x00,       0x00,                0x00          };
	daisy_transfer(dd, tx, rx, 3);
}

/**
 * Set multiple bits in 16 bit register.
 */
static inline void daisy_set_mbits16(struct daisy_dev *dd,
								   	   	    uint8_t    reg,
										    uint16_t   mask,
										    uint16_t   val)
{
	uint8_t x1[3] = { reg,  0x00, 0x00 };
	uint8_t x2[3] = { 0x00, 0x00, 0x00 };
	daisy_transfer(dd, x1, x2, 3);
	x2[0] = reg | 0x80;
	x2[1] &= ~((mask >> 8) & 0x00ff);
	x2[2] &= ~(mask & 0x00ff);
	x2[1] |= ((val >> 8) & 0x00ff);
	x2[2] |= (val & 0x00ff);
	daisy_transfer(dd, x2, x1, 3);
}

/**
 * Set bits in 16 bit register.
 */
static inline void daisy_set_bits16(struct daisy_dev *dd,
								       	   uint8_t    reg,
										   uint16_t   mask)
{
	uint8_t x1[3] = { reg,  0x00, 0x00 };
	uint8_t x2[3] = { 0x00, 0x00, 0x00 };
	daisy_transfer(dd, x1, x2, 3);
	x2[0] = reg | 0x80;
	x2[1] |= ((mask >> 8) & 0x00ff);
	x2[2] |= (mask & 0x00ff);
	daisy_transfer(dd, x2, x1, 3);
}

/**
 * Clear bits in 16 bit register.
 */
static inline void daisy_clr_bits16(struct daisy_dev *dd,
								       	   uint8_t    reg,
										   uint16_t   mask)
{
	uint8_t x1[3] = { reg,  0x00, 0x00 };
	uint8_t x2[3] = { 0x00, 0x00, 0x00 };
	daisy_transfer(dd, x1, x2, 3);
	x2[0] = reg | 0x80;
	x2[1] &= ~((mask >> 8) & 0x00ff);
	x2[2] &= ~(mask & 0x00ff);
	daisy_transfer(dd, x2, x1, 3);
}

/**
 * Direct read 16 bit register.
 */
static inline uint16_t daisy_get_register16(struct daisy_dev *dd,
								     	 	 	   uint8_t    reg)
{
	uint8_t tx[3] = { reg,  0x00, 0x00  };
	uint8_t rx[3] = { 0x00, 0x00, 0x00  };
	daisy_transfer(dd, tx, rx, 3);
	return (rx[1] << 8) | rx[2];
}

/**
 * Check if too less entries are available for transmit so
 * transmitting shall be stopped.
 * @param q Pointer to the tx_queue.
 * @return True if too less entries are available in the TX
 *         queue.
 */
extern bool tx_low_water_dn(struct daisy_dev *dd);

/**
 * Check if enough entries are available for transmit so
 * transmitting can be restarted.
 * @param q Pointer to the tx_queue.
 * @return True if enough entries are available in the TX
 *         queue.
 */
extern bool tx_low_water_up(struct daisy_dev *dd);

#endif /* _SPI_DAISY_H_ */
