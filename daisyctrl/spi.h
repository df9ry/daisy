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

/* Serial Peripheral Interface (SPI) bus Class
 *	This class allows you to take control of devices on the SPI bus from Linux
 *	It has been developed and tested on the BeagleBone Black but should work on
 *	any Linux system with spidev support (e.g. Raspberry Pi)
 *
 *	Usage is simple...
 *	To create an instance, point the constructor at your SPI bus
 *		SPI *myBus = new SPI("/dev/spidev1.0");
 *
 *	Set the speed of the bus (or leave it at the 100kHz default)
 *		myBus->setMaxSpeedHz(1000000);
 *
 *	Transfer some data
 *		uint8_t tx[] = {0x55, 0x00};
 *		uint8_t rx[] = {0x00, 0x00};
 *		myBus->transfer(tx, rx, 2);
 *	Note that tx and rx arrays must be the same size
 *	(the size is passed as the 3rd parameter to 'transfer')
 *
 *	Close the bus
 *		myBus->close();
 */
 
#ifndef _SPI_H
#define _SPI_H

#include <stdint.h>

namespace SPI_NS {

	class SPI {
	public:
		// Constructor
		SPI(): fd{-1}, bits{0}, speed{0}, mode{0}, delay{0} {}
		SPI(const char* device): SPI() { open(device); }
		~SPI() { close(); }

		// Open the device
		void open(const char* device);

		// Set or get the SPI mode
		void setMode(uint8_t mode);
		uint8_t getMode();

		// Set or get the bits per word
		void setBitsPerWord(uint8_t bits);
		uint8_t getBitsPerWord();

		// Set or get the SPI clock speed
		void setMaxSpeedHz(uint32_t speed);
		uint32_t getMaxSpeedHz();

		// Set or get the SPI delay
		void setDelayUsecs(uint16_t delay);
		uint16_t getDelayUsecs();

		// Transfer some data
		//	tx:	Array of bytes to be transmitted
		//	rx: Array of bytes to be received
		//	length:	Length of arrays (must be equal)
		// If you just want to send data you still need to pass in
		// an rx array, but you can safely ignore its output
		// Returns true if transfer was successful (false otherwise)
		bool transfer(uint8_t *tx, uint8_t *rx, int length);

		// Close the bus
		void close();
	private:
		int         fd;
		uint8_t     mode;
		uint8_t     bits;
		uint32_t    speed;
		uint16_t    delay;
	};

} // end namespace //

#endif
