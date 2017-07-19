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

#define MIN_SPEED_HZ   50000
#define MAX_SPEED_HZ 5000000
#define N_SLOTS            2

struct daisy_dev;
struct daisy_spi;

/**
 * Open a daisy device. Use daisy_close_handle() to release the device.
 */
extern struct daisy_dev *daisy_open_device(uint16_t slot);

/**
 * Close a daisy device previously opened with daisy_open_device().
 */
extern void daisy_close_device(struct daisy_dev *bs);

/**
 * This is called by other drivers that need to bypass the SPI subsystem.
 * In daisy it is used by the network driver from within the interrupt handler.
 * Can be called from contexts that cannot sleep. Multithreading is supported
 * by the spinlock in the daisy_spi struct.
 */
extern void daisy_transfer(struct daisy_dev  *dd,
					  const volatile uint8_t *tx,
							volatile uint8_t *rx,
							size_t            cb);

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

#endif /* _SPI_DAISY_H_ */
