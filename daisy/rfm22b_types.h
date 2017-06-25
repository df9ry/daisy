/* Copyright 2017 Tania Hagn. This file is based on the work of Owen McAree.
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

#ifndef _RFM22B_TYPES_H
#define _RFM22B_TYPES_H

namespace RFM22B_NS {

	enum class RFM22B_Modulation_Type {
		UNMODULATED_CARRIER							= 0x00,
		OOK											= 0x01,
		FSK											= 0x02,
		GFSK										= 0x03
	};

	enum class RFM22B_Modulation_Data_Source {
		DIRECT_GPIO									= 0x00,
		DIRECT_SDI									= 0x01,
		FIFO										= 0x02,
		PN9											= 0x03
	};

	enum class RFM22B_CRC_Mode {
			CRC_DISABLED,
			CRC_DATA_ONLY,
			CRC_NORMAL,
	};

	enum class RFM22B_CRC_Polynomial {
		CCITT										= 0x00,
		CRC16										= 0x01,
		IEC16										= 0x02,
		BIACHEVA									= 0x03,
	};

	enum class RFM22B_Data_Clock_Configuration {
		NONE										= 0x00,
		GPIO										= 0x01,
		SDO											= 0x02,
		NIRQ										= 0x03
	};

	enum class RFM22B_GPIO_Function {
		POWER_ON_RESET								= 0x00,		// This depends on the GPIO!
		WAKE_UP_TIMER_1								= 0x01,
		LOW_BATTERY_DETECT_1						= 0x02,
		DIRECT_DIGITAL_INPUT						= 0x03,
		EXTERNAL_INTERRUPT_FALLING					= 0x04,
		EXTERNAL_INTERRUPT_RISING					= 0x05,
		EXTERNAL_INTERRUPT_CHANGE					= 0x06,
		ADC_ANALOGUE_INPUT							= 0x07,
	//	RESERVED									= 0x08,
	//	RESERVED									= 0x09,
		DIRECT_DIGITAL_OUTPUT						= 0x0A,
	//	RESERVED									= 0x0B,
	//	RESERVED									= 0x0C,
	//	RESERVED									= 0x0D,
		REFERENCE_VOLTAGE							= 0x0E,
		DATA_CLOCK_OUTPUT							= 0x0F,
		DATA_INPUT									= 0x10,
		EXTERNAL_RETRANSMISSION_REQUEST				= 0x11,
		TX_STATE									= 0x12,
		TX_FIFO_ALMOST_FULL							= 0x13,
		RX_DATA										= 0x14,
		RX_STATE									= 0x15,
		RX_FIFO_ALMOST_FULL							= 0x16,
		ANTENNA_1_SWITCH							= 0x17,
		ANTENNA_2_SWITCH							= 0x18,
		VALID_PREAMBLE_DETECTED						= 0x19,
		INVALID_PREAMBLE_DETECTED					= 0x1A,
		SYNC_WORD_DETECTED							= 0x1B,
		CLEAR_CHANNEL_ASSESSMENT					= 0x1C,
		VDD											= 0x1D,
		GND											= 0x1E
	};

	// Interrupt Enable spans 2 registers, but for the purpose of this enum they are treated as one (16 bit) register
	enum class RFM22B_Interrupt {
		POWER_ON_RESET_INT							= (1 << 0),
		CHIP_READY									= (1 << 1),
		LOW_BATTERY_DETECT							= (1 << 2),
		WAKE_UP_TIMER								= (1 << 3),
		RSSI										= (1 << 4),
		INVALID_PREAMBLE							= (1 << 5),
		VALID_PREAMBLE								= (1 << 6),
		SYNC_WORD									= (1 << 7),
		CRC_ERROR									= (1 << 8),
		VALID_PACKET_RECEIVED						= (1 << 9),
		PACKET_SENT									= (1 << 10),
		EXTERNAL									= (1 << 11),
		RX_FIFO_ALMOST_FULL_INT						= (1 << 12),
		TX_FIFO_ALMOST_EMPTY_INT					= (1 << 13),
		TX_FIFO_ALMOST_FULL_INT						= (1 << 14),
		FIFO_UNDERFLOW_OVERFLOW						= (1 << 15)
	};

	// Currently no support for antenna diversity settings
	// Treat registers 1 and 2 as a single 16 bit register (as above)
	enum class RFM22B_Operating_Mode {
		TX_FIFO_RESET								= (1 << 0),
		RX_FIFO_RESET								= (1 << 1),
		LOW_DUTY_CYCLE_MODE							= (1 << 2),
		AUTOMATIC_TRANSMISSION						= (1 << 3),
		RX_MULTI_PACKET								= (1 << 4),
	//	Antenna diversity (bits 5-7) not supported
		READY_MODE									= (1 << 8),
		TUNE_MODE									= (1 << 9),
		RX_MODE										= (1 << 10),
		TX_MODE										= (1 << 11),
		CRYSTAL_OSCILLATOR_SELECT					= (1 << 12),
		ENABLE_WAKE_UP_TIMER						= (1 << 13),
		ENABLE_LOW_BATTERY_DETECT					= (1 << 14),
		RESET										= (1 << 15)
	};

	enum class RFM22B_Modulation_Mode {
		WHITENING									= (1 << 0),
		MANCHESTER									= (1 << 1),
		MANCHESTER_INVERSION						= (1 << 2),
		MANCHESTER_POLARITY							= (1 << 3)
	};



} // end namespace //

#endif
