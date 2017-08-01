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

static const uint8_t x5b6b[] = {
	/*00*/ 0xa7, // 10100111:D.00
	/*01*/ 0x9d, // 10011101:D.01
	/*02*/ 0xad, // 10101101:D.02
	/*03*/ 0x31, // 00110001:D.03
	/*04*/ 0xb5, // 10110101:D.04
	/*05*/ 0x29, // 00101001:D.05
	/*06*/ 0x19, // 00011001:D.06
	/*07*/ 0xb8, // 10111000:D.07
	/*08*/ 0xb9, // 10111001:D.08
	/*09*/ 0x25, // 00100101:D.09
	/*0a*/ 0x15, // 00010101:D.10
	/*0b*/ 0x34, // 00110100:D.11
	/*0c*/ 0x0d, // 00001101:D.12
	/*0d*/ 0x2c, // 00101100:D.13
	/*0e*/ 0x1c, // 00011100:D.14
	/*0f*/ 0x97, // 10010111:D.15
	/*10*/ 0x9b, // 10011011:D.16
	/*11*/ 0x23, // 00100011:D.17
	/*12*/ 0x13, // 00010011:D.18
	/*13*/ 0x32, // 00110010:D.19
	/*14*/ 0x0b, // 00001011:D.20
	/*15*/ 0x2a, // 00101010:D.21
	/*16*/ 0x1a, // 00011010:D.22
	/*17*/ 0xba, // 10111010:D.23
	/*18*/ 0xb3, // 10110011:D.24
	/*19*/ 0x26, // 00100110:D.25
	/*1a*/ 0x16, // 00010110:D.26
	/*1b*/ 0xb6, // 10110110:D.27
	/*1c*/ 0x0e, // 00001110:D.28
	/*1d*/ 0xae, // 10101110:D.29
	/*1e*/ 0x9e, // 10011110:D.30
	/*1f*/ 0xab, // 10101011:D.31
	/*20*/ 0xcf, // 11001111:K.28
};

static const uint8_t rev_x5b6b[] = {
	/*00*/ 0xff, // ________:Not assigned
	/*01*/ 0xff, // ________:Not assigned
	/*02*/ 0xff, // ________:Not assigned
	/*03*/ 0xff, // ________:Not assigned
	/*04*/ 0xff, // ________:Not assigned
	/*05*/ 0xff, // ________:Not assigned
	/*06*/ 0xff, // ________:Not assigned
	/*07*/ 0xff, // ________:Not assigned
	/*08*/ 0xff, // ________:Not assigned
	/*09*/ 0xff, // ________:Not assigned
	/*0a*/ 0xff, // ________:Not assigned
	/*0b*/ 0x14, // 00001011:D.20
	/*0c*/ 0xff, // ________:Not assigned
	/*0d*/ 0x0c, // 00001101:D.12
	/*0e*/ 0x1c, // 00001110:D.28
	/*0f*/ 0x20, // 11001111:K.28
	/*10*/ 0xff, // ________:Not assigned
	/*11*/ 0xff, // ________:Not assigned
	/*12*/ 0xff, // ________:Not assigned
	/*13*/ 0x12, // 00010011:D.18
	/*14*/ 0xff, // ________:Not assigned
	/*15*/ 0x0a, // 00010101:D.10
	/*16*/ 0x1a, // 00010110:D.26
	/*17*/ 0x0f, // 10010111:D.15
	/*18*/ 0xff, // ________:Not assigned
	/*19*/ 0x06, // 00011001:D.06
	/*1a*/ 0x16, // 00011010:D.22
	/*1b*/ 0x10, // 10011011:D.16
	/*1c*/ 0x0e, // 00011100:D.14
	/*1d*/ 0x01, // 10011101:D.01
	/*1e*/ 0x1e, // 10011110:D.30
	/*1f*/ 0xff, // ________:Not assigned
	/*20*/ 0xff, // ________:Not assigned
	/*21*/ 0xff, // ________:Not assigned
	/*22*/ 0xff, // ________:Not assigned
	/*23*/ 0x11, // 00100011:D.17
	/*24*/ 0xff, // ________:Not assigned
	/*25*/ 0x09, // 00100101:D.09
	/*26*/ 0x19, // 00100110:D.25
	/*27*/ 0x00, // 10100111:D.00
	/*28*/ 0xff, // ________:Not assigned
	/*29*/ 0x05, // 00101001:D.05
	/*2a*/ 0x15, // 00101010:D.21
	/*2b*/ 0x1f, // 10101011:D.31
	/*2c*/ 0x0d, // 00101100:D.13
	/*2d*/ 0x02, // 10101101:D.02
	/*2e*/ 0x1d, // 10101110:D.29
	/*2f*/ 0xff, // ________:Not assigned
	/*30*/ 0xff, // ________:Not assigned
	/*31*/ 0x03, // 00110001:D.03
	/*32*/ 0x13, // 00110010:D.19
	/*33*/ 0x18, // 10110011:D.24
	/*34*/ 0x0b, // 00110100:D.11
	/*35*/ 0x04, // 10110101:D.04
	/*36*/ 0x1b, // 10110110:D.27
	/*37*/ 0xff, // ________:Not assigned
	/*38*/ 0x07, // 10111000:D.07
	/*39*/ 0x08, // 10111001:D.08
	/*3a*/ 0x17, // 10111010:D.23
	/*3b*/ 0xff, // ________:Not assigned
	/*3c*/ 0xff, // ________:Not assigned
	/*3d*/ 0xff, // ________:Not assigned
	/*3e*/ 0xff, // ________:Not assigned
	/*3f*/ 0xff, // ________:Not assigned
};

static const uint8_t x3b4b[] = {
	/*00*/ 0x8b, // 10001011:D.x.0
	/*01*/ 0x09, // 00001001:D.x.1
	/*02*/ 0x05, // 00000101:D.x.2
	/*03*/ 0x8c, // 10001100:D.x.3
	/*04*/ 0x8d, // 10001101:D.x.4
	/*05*/ 0x0a, // 00001010:D.x.5
	/*06*/ 0x06, // 00000110:D.x.6
	/*07*/ 0xee, // 11101110:D.x.P7
	/*08*/ 0xf7, // 11110111:D.x.A7
};

static const uint8_t rev_x3b4b[] = {
	/*00*/ 0xff, // ________:Not assigned
	/*01*/ 0xff, // ________:Not assigned
	/*02*/ 0xff, // ________:Not assigned
	/*03*/ 0xff, // ________:Not assigned
	/*04*/ 0xff, // ________:Not assigned
	/*05*/ 0x02, // 00000101:D.x.2
	/*06*/ 0x06, // 00000110:D.x.6
	/*07*/ 0x08, // 11110111:D.x.A7
	/*08*/ 0xff, // ________:Not assigned
	/*09*/ 0x01, // 00001001:D.x.1
	/*0a*/ 0x05, // 00001010:D.x.5
	/*0b*/ 0x00, // 10001011:D.x.0
	/*0c*/ 0x03, // 10001100:D.x.3
	/*0d*/ 0x04, // 10001101:D.x.4
	/*0e*/ 0x07, // 11101110:D.x.P7
	/*0f*/ 0xff, // ________:Not assigned
};

