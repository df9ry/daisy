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

#ifndef _SPI_H_
#define _SPI_H_

#include <linux/interrupt.h>
#include <linux/timer.h>

#include "bcm2835_hw.h"
#include "ev_queue.h"

#define GPIO_SLOT0_PIN     RPI_GPIO_P1_08
#define GPIO_SLOT0_DESC    "DAISY Interrupt line"

#define daisy_SPI_MODE_BITS	(SPI_CPOL | SPI_CPHA | SPI_CS_HIGH \
				| SPI_NO_CS | SPI_3WIRE)

#define RFM22B_REG_DEVICE_STATUS    0x02

#define RFM22B_REG_INTERRUPT_STATUS 0x03
#define RFM22B_POR                 (1<<0)
#define RFM22B_CHIPRDY             (1<<1)
#define RFM22B_LBD                 (1<<2)
#define RFM22B_WUT                 (1<<3)
#define RFM22B_RSSI                (1<<4)
#define RFM22B_PREAINVAL           (1<<5)
#define RFM22B_PREAVAL             (1<<6)
#define RFM22B_SWDET               (1<<7)
#define RFM22B_CRCERROR            (1<<8)
#define RFM22B_PKVALID             (1<<9)
#define RFM22B_PKSENT              (1<<10)
#define RFM22B_EXT                 (1<<11)
#define RFM22B_RXFFAFUL            (1<<12)
#define RFM22B_TXFFAEM             (1<<13)
#define RFM22B_TXFFAFULL           (1<<14)
#define RFM22B_FFERR               (1<<15)

#define RFM22B_REG_INTERRUPT_ENABLE 0x05
#define RFM22B_ENPOR               (1<<0)
#define RFM22B_ENCHIPRDY           (1<<1)
#define RFM22B_ENLBD1              (1<<2)
#define RFM22B_ENWUT               (1<<3)
#define RFM22B_ENRSSI              (1<<4)
#define RFM22B_ENPREAINVAL         (1<<5)
#define RFM22B_ENPREAVAL           (1<<6)
#define RFM22B_ENSWDET             (1<<7)
#define RFM22B_ENCRCERROR          (1<<8)
#define RFM22B_ENPKVALID           (1<<9)
#define RFM22B_ENPKSENT            (1<<10)
#define RFM22B_ENEXT               (1<<11)
#define RFM22B_ENRXFFAFUL          (1<<12)
#define RFM22B_ENTXFFAEM           (1<<13)
#define RFM22B_ENTXFFAFULL         (1<<14)
#define RFM22B_ENFFERR             (1<<15)

#define RFM22B_REG_OPERATING_MODE   0x07
#define RFM22B_FFCLRTX             (1<<0)
#define RFM22B_FFCLRRX             (1<<1)
#define RFM22B_ENLDM               (1<<2)
#define RFM22B_AUTOTX              (1<<3)
#define RFM22B_RXMPK               (1<<4)
#define RFM22B_ANTDIV              (1<<5)
#define RFM22B_XTON                (1<<8)
#define RFM22B_PLLON               (1<<9)
#define RFM22B_RXON                (1<<10)
#define RFM22B_TXON                (1<<11)
#define RFM22B_X32KSEL             (1<<12)
#define RFM22B_ENWT                (1<<13)
#define RFM22B_ENLBD2              (1<<14)
#define RFM22B_SWRES               (1<<15)

#define RFM22B_REG_MODULATION_MODE  0x70
#define RFM22B_DTMOD_MASK           0x0081
#define RFM22B_DTMOD_DIRECT_GPIO    0x0000
#define RFM22B_DTMOD_DIRECT_SDI     0x0001
#define RFM22B_DTMOD_FIFO           0x0080
#define RFM22B_DTMOD_PN9            0x0081

struct net_device_stats;

struct daisy_spi {
	struct platform_device  *pdev;
	spinlock_t               transfer_lock;
	struct clk              *clk;
	uint32_t                 spi_hz;
	bool                     speed_lock;
};

enum automaton_state {
	STATUS_IDLE = 0,
};

struct daisy_dev {
	struct kobject          *kobj;
	struct daisy_spi        *spi;
	struct spi_device       *dev;
	struct spi_master       *master;
	struct rx_queue         *rx_queue;
	struct tx_queue         *tx_queue;
	struct net_device_stats *stats;
	uint16_t                 slot;
	short int                irq;
	enum automaton_state     state;
	struct ev_queue          evq;
	struct tasklet_struct    tasklet;
	struct timer_list        watchdog;
	unsigned long            timeout;
};

extern irqreturn_t irq_handler(int irq, void *_dd, struct pt_regs *regs);
extern void tasklet(unsigned long _dd);
extern void watchdog(unsigned long _dd);

#endif //_SPI_H_//
