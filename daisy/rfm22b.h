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

#ifndef _RFM22B_H
#define _RFM22B_H

#include <vector>
#include <mutex>

namespace RFM22B_NS {

	enum class RFM22B_CRC_Mode;
	enum class RFM22B_CRC_Polynominal;
	enum class RFM22B_Data_Clock_Configuration;
	enum class RFM22B_GPIO;
	enum class RFM22B_GPIO_Function;
	enum class RFM22B_Interrupt;
	enum class RFM22B_Modulation_Data_Source;
	enum class RFM22B_Modulation_Type;
	enum class RFM22B_Modulation_Mode;
	enum class RFM22B_Operating_Mode;
	enum class RFM22B_CRC_Polynomial;
	enum class RFM22B_Register;

	struct register_value { uint8_t setting[2]; };

	class RFM22B {
	public:
		// Constructor
		RFM22B();

		// Destructor
		~RFM22B();

		// Reset the device
		void reset();

		// Get device type.
		uint8_t getDeviceType();

		// Get device version.
		uint8_t getDeviceVersion();

		// Get device status.
		uint8_t getDeviceStatus();

		// Set the header address.
		void setAddress(const std::vector<uint8_t>& addr);

		// Get the header address.
		std::vector<uint8_t> getAddress();

		// Set standard modes:
		void setNarrowMode();
		void setMediumMode();
		void setWideMode();

		// Tune for some seconds:
		void tune(unsigned int seconds);

		// Send packages:
		void tx_packages(unsigned int numpkts);

		// Receive packages:
		size_t rx_packages(uint8_t* pb, size_t cb, unsigned int timeout);

		// Set or get the carrier frequency (in Hz);
		void setCarrierFrequency(unsigned int frequency);
		unsigned int getCarrierFrequency();

		// Set or get the frequency hopping step size (in Hz, but it is floored to nearest 10kHz)
		void setFrequencyHoppingStepSize(unsigned int step);
		unsigned int getFrequencyHoppingStepSize();

		// Set or get the frequency hopping channel
		void setChannel(uint8_t channel);
		uint8_t getChannel();

		// Set or get the frequency deviation (in Hz, but floored to the nearest 625Hz)
		void setFrequencyDeviation(unsigned int deviation);
		unsigned int getFrequencyDeviation();

		// Set or get the TX data rate (bps)
		// NOTE: This does NOT configure the receive data rate! To properly set
		// up the device for receiving, use the magic register values
		// calculated using the Si443x-Register-Settings_RevB1.xls Excel sheet.
		void setDataRate(unsigned int rate);
		unsigned int getDataRate();

		// Set or get the bandwidth in hz.
		void setBandwidth(unsigned int bw);
		unsigned int getBandwidth();

		// Set or get the modulation type
		void setModulationType(RFM22B_Modulation_Type modulation);
		RFM22B_Modulation_Type getModulationType();

		// Set or get the modulation mode
		void setModulationMode(RFM22B_Modulation_Mode mode);
		RFM22B_Modulation_Mode getModulationMode();

		// Set or get the modulation data source
		void setModulationDataSource(RFM22B_Modulation_Data_Source source);
		RFM22B_Modulation_Data_Source getModulationDataSource();

		// Set or get the data clock source
		void setDataClockConfiguration(RFM22B_Data_Clock_Configuration clock);
		RFM22B_Data_Clock_Configuration getDataClockConfiguration();

		// Set or get the transmission power
		void setTransmissionPower(uint8_t power);
		uint8_t getTransmissionPower();

		// Set or get the preamble length
		void setPreambleLength(uint32_t len);
		uint32_t getPreambleLength();

		// Set or get the GPIO configuration
		void setGPIOFunction(RFM22B_GPIO gpio, RFM22B_GPIO_Function funct);
		// This should probably return enum, but this needs a lot of cases
		RFM22B_GPIO_Function getGPIOFunction(RFM22B_GPIO gpio);

		// Enable or disable interrupts
		// No ability to get interrupt enable status as this would need a lot of case statements
		void setInterruptEnable(RFM22B_Interrupt interrupt, bool enable);

		// Get the status of an interrupt
		bool getInterruptStatus(RFM22B_Interrupt interrupt);

		// Set the operating mode
		//	This function does not toggle individual pins as with other functions
		//	It expects a bitwise-ORed combination of the modes you want set
		void setOperatingMode(RFM22B_Operating_Mode mode);

		// Get operating mode (bitwise-ORed)
		RFM22B_Operating_Mode getOperatingMode();

		// Manually enable RX or TX modes
		void enableRXMode();
		void enableTXMode();

		// Set or get the transmit header
		void setTransmitHeader(uint32_t header);
		uint32_t getTransmitHeader();

		// Set or get the check header
		void setCheckHeader(uint32_t header);
		uint32_t getCheckHeader();
	
		// Set or get the CRC mode
		void setCRCMode(RFM22B_CRC_Mode mode);
		RFM22B_CRC_Mode getCRCMode();

		// Set or get the CRC polynominal
		void setCRCPolynomial(RFM22B_CRC_Polynomial poly);
		RFM22B_CRC_Polynomial getCRCPolynomial();

		// Get and set all the FIFO threshold
		void setTXFIFOAlmostFullThreshold(uint8_t thresh);
		void setTXFIFOAlmostEmptyThreshold(uint8_t thresh);
		void setRXFIFOAlmostFullThreshold(uint8_t thresh);
		uint8_t getTXFIFOAlmostFullThreshold();
		uint8_t getTXFIFOAlmostEmptyThreshold();
		uint8_t getRXFIFOAlmostFullThreshold();

		// Get RSSI value or input dBm
		uint8_t getRSSI();
		int8_t getInputPower();

		// Get length of last received packet
		uint8_t getReceivedPacketLength();

		// Set length of packet to be transmitted
		void setTransmitPacketLength(uint8_t length);

		// Clear the FIFOs
		void clearRXFIFO();
		void clearTXFIFO();

		// Control the debug flag
		void setDebug(bool f) { debug = f; }

		// Get the debug flag
		bool getDebug() { return debug; }

		// Send data
		void send(uint8_t *data, size_t length);

		// Receive data (blocking with timeout). Returns number of bytes received
		size_t receive(uint8_t *data, size_t length, int timeout=30000);

		// Transfer
		void transfer(uint8_t *tx, uint8_t *rx, size_t size);

		// Helper functions for getting and getting individual registers
		uint8_t getRegister(RFM22B_Register reg);
		uint16_t get16BitRegister(RFM22B_Register reg);
		uint32_t get32BitRegister(RFM22B_Register reg);
		void setRegister(RFM22B_Register reg, uint8_t value);
		void set16BitRegister(RFM22B_Register reg, uint16_t value);
		void set32BitRegister(RFM22B_Register reg, uint32_t value);

		static const uint8_t MAX_PACKET_LENGTH = 64;
	private:
		void setFIFOThreshold(RFM22B_Register reg, uint8_t thresh);
		void init(struct register_value rg_rv[]);

		std::mutex           transfer_lock;
		std::vector<uint8_t> addr {};
		bool                 debug = false;
		uint8_t              tx[MAX_PACKET_LENGTH+1];
		uint8_t              rx[MAX_PACKET_LENGTH+1];
	};

} // end namespace //

#endif