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

#ifndef _RFM22B_REGISTERS_H
#define _RFM22B_REGISTERS_H

namespace RFM22B_NS {

	enum class RFM22B_Register {
		DEVICE_TYPE 								= 0x00,
		DEVICE_VERSION								= 0x01,
		DEVICE_STATUS								= 0x02,
		INTERRUPT_STATUS_1							= 0x03,
		INTERRUPT_STATUS_2							= 0x04,
		INTERRUPT_ENABLE_1							= 0x05,
		INTERRUPT_ENABLE_2							= 0x06,
		OPERATING_MODE_AND_FUNCTION_CONTROL_1		= 0x07,
		OPERATING_MODE_AND_FUNCTION_CONTROL_2		= 0x08,
		CRYSTAL_OSCILLATOR_LOAD_CAPACITANCE			= 0x09,
		MICROCONTROLLER_OUTPUT_CLOCK				= 0x0A,
		GPIO0_CONFIGURATION							= 0x0B,
		GPIO1_CONFIGURATION							= 0x0C,
		GPIO2_CONFIGURATION							= 0x0D,
		IO_PORT_CONFIGURATION						= 0x0E,
		ADC_CONFIGURATION							= 0x0F,
		ADC_SENSOR_AMPLIFIER_OFFSET					= 0x10,
		ADC_VALUE									= 0x11,
		TEMPERATURE_SENSOR_CONTROL					= 0x12,
		TEMPERATURE_VALUE_OFFSET					= 0x13,
		WAKE_UP_TIMER_PERIOD_1						= 0x14,
		WAKE_UP_TIMER_PERIOD_2						= 0x15,
		WAKE_UP_TIMER_PERIOD_3						= 0x16,
		WAKE_UP_TIMER_VALUE_1						= 0x17,
		WAKE_UP_TIMER_VALUE_2						= 0x18,
		LOW_DUTY_CYCLE_MODE_DURATION				= 0x19,
		LOW_BATTERY_DETECTOR_THRESHOLD				= 0x1A,
		BATTERY_VOLTAGE_LEVEL						= 0x1B,
		IF_FILTER_BANDWIDTH							= 0x1C,
		AFC_LOOP_GEARSHIFT_OVERRIDE					= 0x1D,
		AFC_TIMING_CONTROL							= 0x1E,
		CLOCK_RECOVERY_GEARSHIFT_OVERRIDE			= 0x1F,
		CLOCK_RECOVERY_OVERSAMPLING_RATIO			= 0x20,
		CLOCK_RECOVERY_OFFSET_2						= 0x21,
		CLOCK_RECOVERY_OFFSET_1						= 0x22,
		CLOCK_RECOVERY_OFFSET_0						= 0x23,
		CLOCK_RECOVERT_TIMING_LOOP_GAIN_1			= 0x24,
		CLOCK_RECOVERT_TIMING_LOOP_GAIN_0			= 0x25,
		RECEIVED_SIGNAL_STRENGTH_INDICATOR			= 0x26,
		RSSI_THRESHOLD_FOR_CLEAR_CHANNEL_INDICATOR	= 0x27,
		ANTENNA_DIVERSITY_REGISTER_1				= 0x28,
		ANTENNA_DIVERSITY_REGISTER_2				= 0x29,
		AFC_LIMITER									= 0x2A,
		AFC_CORRECTION_READ							= 0x2B,
		OOK_COUNTER_VALUE_1							= 0x2C,
		OOK_COUNTER_VALUE_2							= 0x2D,
		SLICER_PEAK_HOLD							= 0x2E,
	//	RESERVED 									= 0x2F,
		DATA_ACCESS_CONTROL							= 0x30,
		EXMAC_STATUS								= 0x31,
		HEADER_CONTROL_1							= 0x32,
		HEADER_CONTROL_2							= 0x33,
		PREAMBLE_LENGTH								= 0x34,
		PREAMBLE_DETECTION_CONTROL					= 0x35,
		SYNC_WORD_3									= 0x36,
		SYNC_WORD_2									= 0x37,
		SYNC_WORD_1									= 0x38,
		SYNC_WORD_0									= 0x39,
		TRANSMIT_HEADER_3							= 0x3A,
		TRANSMIT_HEADER_2							= 0x3B,
		TRANSMIT_HEADER_1							= 0x3C,
		TRANSMIT_HEADER_0							= 0x3D,
		TRANSMIT_PACKET_LENGTH						= 0x3E,
		CHECK_HEADER_3								= 0x3F,
		CHECK_HEADER_2								= 0x40,
		CHECK_HEADER_1								= 0x41,
		CHECK_HEADER_0								= 0x42,
		HEADER_ENABLE_3								= 0x43,
		HEADER_ENABLE_2								= 0x44,
		HEADER_ENABLE_1								= 0x45,
		HEADER_ENABLE_0								= 0x46,
		RECEIVED_HEADER_3							= 0x47,
		RECEIVED_HEADER_2							= 0x48,
		RECEIVED_HEADER_1							= 0x49,
		RECEIVED_HEADER_0							= 0x4A,
		RECEIVED_PACKET_LENGTH						= 0x4B,
	//	RESERVED									= 0x4C,
	//	RESERVED									= 0x4D,
	//	RESERVED									= 0x4E,
		ADC8_CONTROL								= 0x4F,
	//	RESERVED									= 0x50,
	//	RESERVED									= 0x51,
	//	RESERVED									= 0x52,
	//	RESERVED									= 0x53,
	//	RESERVED									= 0x54,
	//	RESERVED									= 0x55,
	//	RESERVED									= 0x56,
	//	RESERVED									= 0x57,
	//	RESERVED									= 0x58,
	//	RESERVED									= 0x59,
	//	RESERVED									= 0x5A,
	//	RESERVED									= 0x5B,
	//	RESERVED									= 0x5C,
	//	RESERVED									= 0x5D,
	//	RESERVED									= 0x5E,
	//	RESERVED									= 0x5F,
		CHANNEL_FILTER_COEFFICIENT_ADDRESS			= 0x60,
	//	RESERVED									= 0x61,
		CRYSTAL_OSCILLATOR_CONTROL_TES				= 0x62,
	//	RESERVED									= 0x63,
	//	RESERVED									= 0x64,
	//	RESERVED									= 0x65,
	//	RESERVED									= 0x66,
	//	RESERVED									= 0x67,
	//	RESERVED									= 0x68,
		AGC_OVERRIDE_1								= 0x69,
	//	RESERVED									= 0x6A,
	//	RESERVED									= 0x6B,
	//	RESERVED									= 0x6C,
		TX_POWER									= 0x6D,
		TX_DATA_RATE_1								= 0x6E,
		TX_DATA_RATE_0								= 0x6F,
		MODULATION_MODE_CONTROL_1					= 0x70,
		MODULATION_MODE_CONTROL_2					= 0x71,
		FREQUENCY_DEVIATION							= 0x72,
		FREQUENCY_OFFSET_1							= 0x73,
		FREQUENCY_OFFSET_2							= 0x74,
		FREQUENCY_BAND_SELECT						= 0x75,
		NOMINAL_CARRIER_FREQUENCY_1					= 0x76,
		NOMINAL_CARRIER_FREQUENCY_0					= 0x77,
	//	RESERVED									= 0x78,
		FREQUENCY_HOPPING_CHANNEL_SELECT			= 0x79,
		FREQUENCY_HOPPING_STEP_SIZE					= 0x7A,
	//	RESERVED									= 0x7B,
		TX_FIFO_CONTROL_1							= 0x7C,
		TX_FIFO_CONTROL_2							= 0x7D,
		RX_FIFO_CONTROL								= 0x7E,
		FIFO_ACCESS									= 0x7F
	};

	enum class RFM22B_GPIO {
		GPIO0										= RFM22B_Register::GPIO0_CONFIGURATION,
		GPIO1										= RFM22B_Register::GPIO1_CONFIGURATION,
		GPIO2										= RFM22B_Register::GPIO2_CONFIGURATION
	};

} // end namespace //

#endif
