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
#include <thread>
#include <functional>

#include "defaults.h"

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

		// Open
		bool open(const std::string& filename);

		// Close
		void close();

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
		void rx_packages(unsigned int timeout);

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
		RFM22B_Operating_Mode getOperatingMode();

		// Set or get squelch level
		void setSquelch(uint8_t level);
		uint8_t getSquelch();

		// Manually enable RX or TX modes
		void enableRXMode();
		void disableRXMode();
		void enableTXMode();
		void disableTXMode();

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
		bool getDebug() { return debug; }

		// Control the verbose flag
		void setVerbose(bool f) { verbose = f; }
		bool getVerbose() { return verbose; }

		// Send data
		void send(std::function<int(uint8_t *pb, size_t cb)> output);

		// Receive data (blocking with timeout). Returns number of bytes received
		void receive(
			std::function<void(uint8_t*,size_t)> input,
			unsigned int                         timeout = DEFAULT_RX_TIMEOUT);

		// Transfer
		void transfer(uint8_t *tx, uint8_t *rx, size_t size);

		// Helper functions for getting and getting individual registers
		uint8_t getRegister(RFM22B_Register reg);
		uint16_t get16BitRegister(RFM22B_Register reg);
		uint32_t get32BitRegister(RFM22B_Register reg);
		void setRegister(RFM22B_Register reg, uint8_t value);
		void set16BitRegister(RFM22B_Register reg, uint16_t value);
		void set32BitRegister(RFM22B_Register reg, uint32_t value);

#ifdef SIMULATE_INTERRUPTS
		void intrtask(); // Int simulation task
		uint16_t waitforinterrupt() {
			if (!intrpending)
				intoccurred.lock();
			intrpending = false;
			return rising_edges;
		}
		int32_t try_waitforinterrupt() {
			if (intrpending) {
				intrpending = false;
				return rising_edges;
			}
			if (intoccurred.try_lock()) {
				intrpending = false;
				return rising_edges;
			}
			return -1;
		}
		void eoi() {
			rising_edges = 0x0000;
			ei();
		}
#endif

		static const uint8_t MAX_PACKET_LENGTH = 64;
	private:
		void setFIFOThreshold(RFM22B_Register reg, uint8_t thresh);
		void init(struct register_value rg_rv[]);

		int                  spidev = -1;
		uint8_t              spimode;
		uint8_t              spibits;
		uint32_t             spispeed;
		std::mutex           transfer_lock;
		std::vector<uint8_t> addr {};
		bool                 debug = false;
		bool                 verbose = false;

#ifdef SIMULATE_INTERRUPTS
		// Support for simulated interrupts:
		std::thread          intrthread;
		volatile bool        intrstop     = false;  // Terminate int
		volatile bool        intrpending  = false;  // Interrupt pending
		std::mutex           intlocked;             // Avoid interrupts
		std::mutex           intoccurred;           // Wait for interrupt
		volatile uint16_t    intrmask     = 0x0000; // Enable mask
		volatile uint16_t    intrhold     = 0x0000; // Old int status
		volatile uint16_t    rising_edges = 0x0000; // Contains rising edges
		void di() { intlocked.lock();   }           // Disable interrupts
		void ei() { intlocked.unlock(); }           // Enable interrupts
#endif
	};

} // end namespace //

#endif
