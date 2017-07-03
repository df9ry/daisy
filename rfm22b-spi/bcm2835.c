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

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/ioport.h>
#include <asm/io.h>

#include "bcm2835_hw.h"
#include "fileio.h"

/*
 * Memory lock to BCM2835 SPI area
 */
static struct resource *spi_memory_lock = NULL;

/*
 * Physical address and size of the peripherals block
 */
static uint32_t *bcm2835_peripherals_base = MAP_FAILED;
static uint32_t bcm2835_peripherals_size = 0;

/* Virtual memory address of the mapped peripherals block
 */
static uint32_t *bcm2835_peripherals = (uint32_t *)MAP_FAILED;

/* And the register bases within the peripherals block
 */
static volatile uint32_t *bcm2835_gpio        = (uint32_t *)MAP_FAILED;
static volatile uint32_t *bcm2835_pwm         = (uint32_t *)MAP_FAILED;
static volatile uint32_t *bcm2835_clk         = (uint32_t *)MAP_FAILED;
static volatile uint32_t *bcm2835_pads        = (uint32_t *)MAP_FAILED;
static volatile uint32_t *bcm2835_spi0        = (uint32_t *)MAP_FAILED;
static volatile uint32_t *bcm2835_bsc0        = (uint32_t *)MAP_FAILED;
static volatile uint32_t *bcm2835_bsc1        = (uint32_t *)MAP_FAILED;
static volatile uint32_t *bcm2835_st	      = (uint32_t *)MAP_FAILED;

/*
 * Read with memory barriers from peripheral
 */
uint32_t bcm2835_peri_read(volatile uint32_t* paddr) {
    uint32_t ret;

   __sync_synchronize();
   ret = *paddr;
   __sync_synchronize();
   return ret;
}

/* read from peripheral without the read barrier
 * This can only be used if more reads to THE SAME peripheral
 * will follow.  The sequence must terminate with memory barrier
 * before any read or write to another peripheral can occur.
 * The MB can be explicit, or one of the barrier read/write calls.
 */
static uint32_t bcm2835_peri_read_nb(volatile uint32_t* paddr) {
	return *paddr;
}

/*
 * Write with memory barriers to peripheral
 */
static void bcm2835_peri_write(volatile uint32_t* paddr, uint32_t value) {
	__sync_synchronize();
	*paddr = value;
	__sync_synchronize();
}

/* write to peripheral without the write barrier */
static void bcm2835_peri_write_nb(volatile uint32_t* paddr, uint32_t value) {
	*paddr = value;
}

/* Set/clear only the bits in value covered by the mask
 * This is not atomic - can be interrupted.
 */
static void bcm2835_peri_set_bits(volatile uint32_t* paddr, uint32_t value,
		uint32_t mask)
{
    uint32_t v = bcm2835_peri_read(paddr);
    v = (v & ~mask) | (value & mask);
    bcm2835_peri_write(paddr, v);
}

/* Function select
// pin is a BCM2835 GPIO pin number NOT RPi pin number
//      There are 6 control registers, each control the functions of a block
//      of 10 pins.
//      Each control register has 10 sets of 3 bits per GPIO pin:
//
//      000 = GPIO Pin X is an input
//      001 = GPIO Pin X is an output
//      100 = GPIO Pin X takes alternate function 0
//      101 = GPIO Pin X takes alternate function 1
//      110 = GPIO Pin X takes alternate function 2
//      111 = GPIO Pin X takes alternate function 3
//      011 = GPIO Pin X takes alternate function 4
//      010 = GPIO Pin X takes alternate function 5
//
// So the 3 bits for port X are:
//      X / 10 + ((X % 10) * 3)
*/
static void bcm2835_gpio_fsel(uint8_t pin, uint8_t mode) {
    /* Function selects are 10 pins per 32 bit word, 3 bits per pin */
    volatile uint32_t* paddr = bcm2835_gpio + BCM2835_GPFSEL0/4 + (pin/10);
    uint8_t   shift = (pin % 10) * 3;
    uint32_t  mask = BCM2835_GPIO_FSEL_MASK << shift;
    uint32_t  value = mode << shift;
    bcm2835_peri_set_bits(paddr, value, mask);
}

int bcm2835_spi_begin(void) {
    volatile uint32_t* paddr;

    if (bcm2835_spi0 == MAP_FAILED)
      return 0; /* bcm2835_init() failed, or not root */

    /* Set the SPI0 pins to the Alt 0 function to enable SPI0 access on them */
    bcm2835_gpio_fsel(RPI_GPIO_P1_26, BCM2835_GPIO_FSEL_ALT0); /* CE1  */
    bcm2835_gpio_fsel(RPI_GPIO_P1_24, BCM2835_GPIO_FSEL_ALT0); /* CE0  */
    bcm2835_gpio_fsel(RPI_GPIO_P1_21, BCM2835_GPIO_FSEL_ALT0); /* MISO */
    bcm2835_gpio_fsel(RPI_GPIO_P1_19, BCM2835_GPIO_FSEL_ALT0); /* MOSI */
    bcm2835_gpio_fsel(RPI_GPIO_P1_23, BCM2835_GPIO_FSEL_ALT0); /* CLK  */

    /* Set the SPI CS register to the some sensible defaults */
    paddr = bcm2835_spi0 + BCM2835_SPI0_CS/4;
    bcm2835_peri_write(paddr, 0); /* All 0s */

    /* Clear TX and RX fifos */
    bcm2835_peri_write_nb(paddr, BCM2835_SPI0_CS_CLEAR);

    return 1; // OK
}

void bcm2835_spi_end(void) {
    if (bcm2835_spi0 == MAP_FAILED)
      return; /* bcm2835_init() failed, or not root */

    /* Set all the SPI0 pins back to input */
    bcm2835_gpio_fsel(RPI_GPIO_P1_26, BCM2835_GPIO_FSEL_INPT); /* CE1  */
    bcm2835_gpio_fsel(RPI_GPIO_P1_24, BCM2835_GPIO_FSEL_INPT); /* CE0  */
    bcm2835_gpio_fsel(RPI_GPIO_P1_21, BCM2835_GPIO_FSEL_INPT); /* MISO */
    bcm2835_gpio_fsel(RPI_GPIO_P1_19, BCM2835_GPIO_FSEL_INPT); /* MOSI */
    bcm2835_gpio_fsel(RPI_GPIO_P1_23, BCM2835_GPIO_FSEL_INPT); /* CLK  */
}

/*
 * Initialize the BCM2835.
 */
int bcm2835_initialize(void) {
	struct file  *fp;
    unsigned char buf[4];

	/* Figure out the base and size of the peripheral address block
    // using the device-tree. Required for RPi2, optional for RPi 1
    */
    fp = file_open(BMC2835_RPI2_DT_FILENAME, O_RDONLY, 0);
    if (fp == NULL) {
    	printk(KERN_ERR "rfm22b: Unable to open file %s\n",
    			BMC2835_RPI2_DT_FILENAME);
    	return -EBADF;
    }
    if (file_read(fp, BMC2835_RPI2_DT_PERI_BASE_ADDRESS_OFFSET, buf, 4) == -1) {
    	printk(KERN_ERR "rfm22b: Unable to read bcm2835_peripherals_base\n");
    	return -EBADF;
    }
    bcm2835_peripherals_base =(uint32_t *)
    		(buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3] << 0);
    if (file_read(fp, BMC2835_RPI2_DT_PERI_SIZE_OFFSET, buf, 4) == -1) {
    	printk(KERN_ERR "rfm22b: Unable to read bcm2835_peripherals_base\n");
    	return -EBADF;
    }
    bcm2835_peripherals_size =
    		(buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3] << 0);
	file_close(fp);
	printk(KERN_DEBUG "rfm22b: BCM2835 is at %x-%x\n",
			(uint32_t)bcm2835_peripherals_base,
			(uint32_t)bcm2835_peripherals_base + bcm2835_peripherals_size);

    /*
     * Reserve IO memory region for BCM2835 SPI
     */
	spi_memory_lock = request_mem_region(
			(uint32_t)bcm2835_peripherals_base + BCM2835_SPI0_BASE,
			BCM2835_SPI0_SIZE, "rfm22b");
	if (!spi_memory_lock) {
    	printk(KERN_ERR "rfm22b: Unable to lock BCM2835_SPI0\n");
    	return -EPERM;
	}

	// Map BCM2835 into virtual memory:
	bcm2835_peripherals = ioremap(
			(uint32_t)bcm2835_peripherals_base, bcm2835_peripherals_size);
	if (!bcm2835_peripherals) {
    	printk(KERN_ERR "rfm22b: Unable to map memory\n");
    	return -EPERM;
	}
	printk(KERN_DEBUG "rfm22b: BCM2835 mapped to %x\n",
			(uint32_t)bcm2835_peripherals);

    /* Now compute the base addresses of various peripherals,
    // which are at fixed offsets within the mapped peripherals block
    // Caution: bcm2835_peripherals is uint32_t*, so divide offsets by 4
    */
    bcm2835_gpio = bcm2835_peripherals + BCM2835_GPIO_BASE/4;
    bcm2835_pwm  = bcm2835_peripherals + BCM2835_GPIO_PWM/4;
    bcm2835_clk  = bcm2835_peripherals + BCM2835_CLOCK_BASE/4;
    bcm2835_pads = bcm2835_peripherals + BCM2835_GPIO_PADS/4;
    bcm2835_spi0 = bcm2835_peripherals + BCM2835_SPI0_BASE/4;
    bcm2835_bsc0 = bcm2835_peripherals + BCM2835_BSC0_BASE/4; /* I2C */
    bcm2835_bsc1 = bcm2835_peripherals + BCM2835_BSC1_BASE/4; /* I2C */
    bcm2835_st   = bcm2835_peripherals + BCM2835_ST_BASE/4;

	return 0;
}

/*
 * Release the BCM2835.
 */
extern void bcm2835_release(void) {
    if (spi_memory_lock)
    	release_mem_region(
    			(uint32_t)bcm2835_peripherals_base + BCM2835_SPI0_BASE,
    			BCM2835_SPI0_SIZE);
    spi_memory_lock = NULL;
    bcm2835_peripherals = MAP_FAILED;
    bcm2835_gpio = MAP_FAILED;
    bcm2835_pwm  = MAP_FAILED;
    bcm2835_clk  = MAP_FAILED;
    bcm2835_pads = MAP_FAILED;
    bcm2835_spi0 = MAP_FAILED;
    bcm2835_bsc0 = MAP_FAILED;
    bcm2835_bsc1 = MAP_FAILED;
    bcm2835_st   = MAP_FAILED;
}

/* defaults to 0, which means a divider of 65536.
// The divisor must be a power of 2. Odd numbers
// rounded down. The maximum SPI clock rate is
// of the APB clock
*/
void bcm2835_spi_setClockDivider(uint16_t divider) {
    volatile uint32_t* paddr = bcm2835_spi0 + BCM2835_SPI0_CLK/4;
    bcm2835_peri_write(paddr, divider);
}

/* Writes (and reads) an number of bytes to SPI */
void bcm2835_spi_transfernb(const uint8_t* tbuf, uint8_t* rbuf, size_t len) {
    volatile uint32_t* paddr = bcm2835_spi0 + BCM2835_SPI0_CS/4;
    volatile uint32_t* fifo = bcm2835_spi0 + BCM2835_SPI0_FIFO/4;
    uint32_t TXCnt=0;
    uint32_t RXCnt=0;

    /*
     * This is Polled transfer as per section 10.6.1
     */

    /* Clear TX and RX fifos */
    bcm2835_peri_set_bits(paddr, BCM2835_SPI0_CS_CLEAR, BCM2835_SPI0_CS_CLEAR);

    /* Set TA = 1 */
    bcm2835_peri_set_bits(paddr, BCM2835_SPI0_CS_TA, BCM2835_SPI0_CS_TA);

    /* Use the FIFO's to reduce the interbyte times */
    while((TXCnt < len)||(RXCnt < len))
    {
        /* TX fifo not full, so add some more bytes */
        while(((bcm2835_peri_read(paddr) & BCM2835_SPI0_CS_TXD))&&(TXCnt < len ))
        {
           bcm2835_peri_write_nb(fifo, tbuf[TXCnt]);
           TXCnt++;
        }
        /* Rx fifo not empty, so get the next received bytes */
        while(((bcm2835_peri_read(paddr) & BCM2835_SPI0_CS_RXD))&&( RXCnt < len ))
        {
           rbuf[RXCnt] = bcm2835_peri_read_nb(fifo);
           RXCnt++;
        }
    }
    /* Wait for DONE to be set */
    while (!(bcm2835_peri_read_nb(paddr) & BCM2835_SPI0_CS_DONE));

    /* Set TA = 0, and also set the barrier */
    bcm2835_peri_set_bits(paddr, 0, BCM2835_SPI0_CS_TA);
}

