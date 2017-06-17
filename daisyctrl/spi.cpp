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

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

#include "spi.h"
#include "spi_exception.h"

namespace SPI_NS {

	//	Opens the SPI device and sets up some default values
	//	Default bits per word is 8
	//	Default clock speed is 10kHz
	void SPI::open(const char *device) {
		this->fd = ::open(device, O_RDWR);
		if (fd < 0) {
			throw SPI_exception("Unable to open SPI device");
		}
		this->setMode(0);
		this->setBitsPerWord(8);
		this->setMaxSpeedHz(100000);
		this->setDelayUsecs(0);
	}

	// Set the mode of the bus (see linux/spi/spidev.h)
	void SPI::setMode(uint8_t mode) {
		int ret = ioctl(this->fd, SPI_IOC_WR_MODE, &mode);
		if (ret == -1)
			throw SPI_exception("Unable to set SPI mode");

		ret = ioctl(this->fd, SPI_IOC_RD_MODE, &this->mode);
		if (ret == -1)
			throw SPI_exception("Unable to get SPI mode");
	}

	// Get the mode of the bus
	uint8_t SPI::getMode() {
		return this->mode;
	}

	// Set the number of bits per word
	void SPI::setBitsPerWord(uint8_t bits) {
		int ret = ioctl(this->fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
		if (ret == -1)
			throw SPI_exception("Unable to set bits per word");

		ret = ioctl(this->fd, SPI_IOC_RD_BITS_PER_WORD, &this->bits);
		if (ret == -1)
			throw SPI_exception("Unable to get bits per word");
	}

	// Get the number of bits per word
	uint8_t SPI::getBitsPerWord() {
		return this->bits;
	}

	// Set the bus clock speed
	void SPI::setMaxSpeedHz(uint32_t speed) {
		int ret = ioctl(this->fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
		if (ret == -1)
			throw SPI_exception("Unable to set max speed Hz");

		ret = ioctl(this->fd, SPI_IOC_RD_MAX_SPEED_HZ, &this->speed);
		if (ret == -1)
			throw SPI_exception("Unable to get max speed Hz");
	}

	// Get the bus clock speed
	uint32_t SPI::getMaxSpeedHz() {
		return this->speed;
	}

	// Set the bus delay
	void SPI::setDelayUsecs(uint16_t delay) {
		this->delay = delay;
	}

	// Get the bus delay
	uint16_t SPI::getDelayUsecs() {
		return this->delay;
	}

	// Transfer some data
	bool SPI::transfer(uint8_t *tx, uint8_t *rx, int length) {
	   struct spi_ioc_transfer tr;

	   tr.tx_buf = (unsigned long)tx;	//tx and rx MUST be the same length!
	   tr.rx_buf = (unsigned long)rx;
	   tr.len = length;
	   tr.delay_usecs = this->delay;
	   tr.speed_hz = this->speed;
	   tr.bits_per_word = this->bits;

	   int ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	   if (ret == 1) {
		  throw SPI_exception("Unable to send spi message");
	   }
	   return true;
	}

	// Close the bus
	void SPI::close() {
		if (this->fd < 0)
			::close(this->fd);
	}

} // end namespace //
