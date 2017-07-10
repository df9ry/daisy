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

#include "initdata.h"

// Settings for GFSK 100kbps, 50kHz deviation on 434.150 MHz:
const uint8_t __initdata const init_data[][2] = {
	{ 0x1c|0x80, 0x99 }, // IF Filter Bandwidth
	{ 0x1d|0x80, 0x40 }, // AFC Loop Gearshift Overide
	{ 0x20|0x80, 0x3c }, // Clock Recovery Oversampling Rate
	{ 0x21|0x80, 0x02 }, // Clock Recovery Offset 2
	{ 0x22|0x80, 0x22 }, // Clock Recovery Offset 1
	{ 0x23|0x80, 0x22 }, // Clock Recovery Offset 0
	{ 0x24|0x80, 0x07 }, // Clock Recovery Timing Loop Gain 1
	{ 0x25|0x80, 0xff }, // Clock Recovery Timing Loop Gain 0
	{ 0x2a|0x80, 0x48 }, // AFC Limiter
	{ 0x30|0x80, 0xaf }, // Data Access Control
	{ 0x32|0x80, 0x8c }, // Header Control 1
	{ 0x33|0x80, 0x02 }, // Header Control 2
	{ 0x34|0x80, 0x0a }, // Preamble Length
	{ 0x35|0x80, 0x2a }, // Preamble Detection Control 1
	{ 0x36|0x80, 0x2d }, // Synchronization Word 3
	{ 0x37|0x80, 0xd4 }, // Synchronization Word 2
	{ 0x38|0x80, 0x00 }, // Synchronization Word 1
	{ 0x39|0x80, 0x00 }, // Synchronization Word 0
	{ 0x3a|0x80, 0x00 }, // Transmit Header 3
	{ 0x3b|0x80, 0x00 }, // Transmit Header 2
	{ 0x3c|0x80, 0x00 }, // Transmit Header 1
	{ 0x3d|0x80, 0x00 }, // Transmit Header 0
	{ 0x3e|0x80, 0x00 }, // Packet Length
	{ 0x3f|0x80, 0x00 }, // Check Header 3
	{ 0x40|0x80, 0x00 }, // Check Header 2
	{ 0x41|0x80, 0x00 }, // Check Header 1
	{ 0x42|0x80, 0x00 }, // Check header 0
	{ 0x43|0x80, 0xff }, // Header Enable 3
	{ 0x44|0x80, 0xff }, // Header Enable 2
	{ 0x45|0x80, 0xff }, // Header Enable 1
	{ 0x46|0x80, 0xff }, // Header Enable 0
	{ 0x6e|0x80, 0x19 }, // TX Data Rate 1
	{ 0x6f|0x80, 0x9a }, // TX Data Rate 0
	{ 0x70|0x80, 0x0d }, // Modulation Mode Control 1
	{ 0x71|0x80, 0x23 }, // Modulation Mode Control 2
	{ 0x72|0x80, 0x50 }, // Frequency Deviation
	{ 0x75|0x80, 0x53 }, // Frequency Band Select
	{ 0x76|0x80, 0x67 }, // Nominal Carrier Frequency
	{ 0x77|0x80, 0xc0 }, // Nominal Carrier Frequency
	{ 0x00,      0x00 }  // Termination NULL Entry
};


