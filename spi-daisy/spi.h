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

#define GPIO_SLOT0_PIN     RPI_GPIO_P1_15
#define GPIO_SLOT0_DESC    "DAISY Interrupt line"

#define IO_MAX 64

#define daisy_SPI_MODE_BITS	(SPI_CPOL | SPI_CPHA | SPI_CS_HIGH \
				| SPI_NO_CS | SPI_3WIRE)

#define RFM22B_REG_DEVICE_STATUS    0x02

#define RFM22B_REG_INTERRUPT_STATUS 0x03
#define RFM22B_IPOR                (1<<0)
#define RFM22B_ICHIPRDY            (1<<1)
#define RFM22B_ILBD                (1<<2)
#define RFM22B_IWUT                (1<<3)
#define RFM22B_IRSSI               (1<<4)
#define RFM22B_IPREAINVAL          (1<<5)
#define RFM22B_IPREAVAL            (1<<6)
#define RFM22B_ISWDET              (1<<7)
#define RFM22B_ICRCERROR           (1<<8)
#define RFM22B_IPKVALID            (1<<9)
#define RFM22B_IPKSENT             (1<<10)
#define RFM22B_IEXT                (1<<11)
#define RFM22B_IRXFFAFUL           (1<<12)
#define RFM22B_ITXFFAEM            (1<<13)
#define RFM22B_ITXFFAFULL          (1<<14)
#define RFM22B_IFFERR              (1<<15)

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
#define RFM22B_ENRXFFAFULL         (1<<12)
#define RFM22B_ENTXFFAEM           (1<<13)
#define RFM22B_ENTXFFAFULL         (1<<14)
#define RFM22B_ENFFERR             (1<<15)

#define RFM22B_REG_OP_MODE_1        0x07
#define RFM22B_XTON                (1<<0)
#define RFM22B_PLLON               (1<<1)
#define RFM22B_RXON                (1<<2)
#define RFM22B_TXON                (1<<3)
#define RFM22B_X32KSEL             (1<<4)
#define RFM22B_ENWT                (1<<5)
#define RFM22B_ENLBD2              (1<<6)
#define RFM22B_SWRES               (1<<7)

#define RFM22B_REG_OP_MODE_2        0x08
#define RFM22B_FFCLRTX             (1<<0)
#define RFM22B_FFCLRRX             (1<<1)
#define RFM22B_ENLDM               (1<<2)
#define RFM22B_AUTOTX              (1<<3)
#define RFM22B_RXMPK               (1<<4)
#define RFM22B_ANTDIV              (1<<5)

#define RFM22B_REG_RSSI             0x26

#define RFM22B_REG_RSSI_TH          0x27

#define RFM22B_DATA_ACCESS_CONTROL  0x30
#define RFM22B_CRC_NONE             0x00
#define RFM22B_CRC_CCITT            0x04
#define RFM22B_CRC_CRC16            0x05
#define RFM22B_CRC_IEC16            0x06
#define RFM22B_CRC_BIACHEVA         0x07
#define RFM22B_ENPACTX             (1<<3)
#define RFM22B_SKIP2PH             (1<<4)
#define RFM22B_CRCDONLY            (1<<5)
#define RFM22B_LSBFRST             (1<<6)
#define RFM22B_ENPACRX             (1<<7)

#define RFM22B_TXPKLEN              0x3e

#define RFM22B_REG_MOD_MODE_2       0x71
#define RFM22B_MODTYP_MASK          0x03
#define RFM22B_MODTYP_UNMODULATED   0x00
#define RFM22B_MODTYP_OOK           0x01
#define RFM22B_MODTYP_FSK           0x02
#define RFM22B_MODTYP_GFSK          0x03
#define RFM22B_FD_8                (1<<2)
#define RFM22B_EINVERT             (1<<3)
#define RFM22B_DTMOD_MASK           0x30
#define RFM22B_DTMOD_DIRECT_GPIO    0x00
#define RFM22B_DTMOD_DIRECT_SDI     0x10
#define RFM22B_DTMOD_FIFO           0x20
#define RFM22B_DTMOD_PN9            0x30
#define RFM22B_TRCLK_TX_DCLK_MASK   0xc0
#define RFM22B_TRCLK_TX_DCLK_NONE   0x00
#define RFM22B_TRCLK_TX_DCLK_GPIO   0x40
#define RFM22B_TRCLK_TX_DCLK_SDO    0x80
#define RFM22B_TRCLK_TX_DCLK_NIRQ   0xc0

#define RFM22B_REG_FIFO             0x7f
#define RFM22B_WRITE_FLAG           0x80

#define RFM22B_ENINTR \
	(RFM22B_ENPOR\
	|RFM22B_ENCHIPRDY\
	|RFM22B_ENSWDET\
	|RFM22B_ENCRCERROR\
	|RFM22B_ENPKVALID\
	|RFM22B_ENPKSENT\
	|RFM22B_ENRXFFAFULL\
	|RFM22B_ENTXFFAEM\
	|RFM22B_ENTXFFAFULL\
	|RFM22B_ENFFERR)

	//RFM22B_ENRSSI      |
	//RFM22B_ENPREAINVAL |
	//RFM22B_ENPREAVAL   |

struct net_device_stats;

extern u8 tx_buffer[IO_MAX+2];
extern u8 rx_buffer[IO_MAX+2];

enum automaton_state {
	STATUS_IDLE,
	STATUS_SEND
};

struct daisy_spi {
	struct platform_device  *pdev;
	spinlock_t               transfer_lock;
	struct clk              *clk;
	uint32_t                 spi_hz;
	bool                     speed_lock;
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
	volatile unsigned long   timeout;
	union {
		struct tx_entry         *tx_entry;
		struct rx_entry         *rx_entry;
	};
	int                      pkg_len;
	int                      pkg_idx;
	u8                      *pkg_ptr;
};

extern irqreturn_t irq_handler(int irq, void *_dd, struct pt_regs *regs);
extern void tasklet(unsigned long _dd);
extern void watchdog(unsigned long _dd);

#endif //_SPI_H_//
