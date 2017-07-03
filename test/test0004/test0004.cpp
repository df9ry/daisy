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

/*
 * Check connectivity to the RFM22B SPI driver.
 */

#include <cstdlib>
#include <iostream>
#include <exception>
#include <stdexcept>

#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include <sys/ioctl.h>

#include "spidev.h"

#define RFM22B_ID 8
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

using namespace std;

int main()
{
	cout << "Test0004" << endl;
	
	int spidev = -1;
	
	try {
		spidev = open(SPI_DEVICE, O_RDWR);
		if (spidev == -1)
			throw logic_error("Unable to open " SPI_DEVICE);
		
		uint8_t mode = SPI_MODE_0 | SPI_NO_CS;
		
		if (ioctl(spidev, SPI_IOC_WR_MODE, &mode) == -1)
			throw logic_error("Unable to set mode");
		if (ioctl(spidev, SPI_IOC_RD_MODE, &mode) == -1)
			throw logic_error("Unable to get mode");
		cout << "Mode is 0x" << (hex) << (int)mode << endl;
		
		uint8_t bits = 8;
		if (ioctl(spidev, SPI_IOC_WR_BITS_PER_WORD, &bits) == -1)
			throw logic_error("Unable to set mode");
		if (ioctl(spidev, SPI_IOC_RD_BITS_PER_WORD, &bits) == -1)
			throw logic_error("Unable to get mode");
		cout << "Bits is " << (dec) << (int)bits << endl;
		
		uint32_t speed = 5000000;
		if (ioctl(spidev, SPI_IOC_RD_MAX_SPEED_HZ, &speed) == -1)
			throw logic_error("Unable to set speed");
		if (ioctl(spidev, SPI_IOC_RD_MAX_SPEED_HZ, &speed) == -1)
			throw logic_error("Unable to get mode");
		cout << "Speed is " << (dec) << speed/1000 << " kHz" << endl;
		
		// Check to see, if the chip can be reached:
		uint16_t delay;
		uint8_t  tx[] { 0x80, 0x00, 0x00 };
		uint8_t  rx[] { 0x00, 0x00, 0x00 };
		
		// In C this is much much more easy ;-(
		struct spi_ioc_transfer tr;
		memset(&tr, 0x00, sizeof(tr));
		tr.tx_buf        = (unsigned long)tx;
		tr.rx_buf        = (unsigned long)rx;
		tr.delay_usecs   = delay;
		tr.len           = ARRAY_SIZE(tx);
		tr.speed_hz      = speed;
		tr.bits_per_word = bits;
		
		if (ioctl(spidev, SPI_IOC_MESSAGE(1), &tr) == -1)
			throw logic_error("Unable to read");
		
		if (rx[1] != RFM22B_ID)
			throw logic_error("RFM22B not found: " + to_string((int)rx[1]));
		
		cout << "Found RFM22B chip version " << (int)rx[2] << endl;		
	}
	catch (exception& ex) {
		cerr << "Error: " << ex.what() << endl;
	}
	catch (...) {
		cerr << "Error: Unspecified" << endl;
	}
	
	if (spidev != -1)
		close(spidev);
	
	return(EXIT_SUCCESS);
}
