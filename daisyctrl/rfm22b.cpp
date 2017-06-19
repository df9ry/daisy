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

#include <cstdlib>
#include <sstream>
#include <iostream>

#include <sys/time.h>

#include "rfm22b.h"
#include "rfm22b_types.h"
#include "rfm22b_registers.h"
#include "daisy_exception.h"
#include "defaults.h"
#include "utility.h"
#include "bcm2835.h"

using namespace std;

static int init_state = 0;

namespace RFM22B_NS {

	// Constructor:
	RFM22B::RFM22B() {
		if (init_state != 0)
			throw daisy_exception("Invalid init state", to_string(init_state));
		if (!bcm2835_init())
			throw daisy_exception("Error initializing BCM2835 chip");
		init_state = 1;
		if (!bcm2835_spi_begin())
			throw daisy_exception("Error initializing SPI system");
		init_state = 2;
	}
	
	// Destructor:
	RFM22B::~RFM22B() {
		if (init_state > 1)
			bcm2835_spi_end();
		if (init_state > 0)
			bcm2835_close();
		init_state = 0;
	}

	// Set the header address.
	void RFM22B::setAddress(const vector<uint8_t>& _addr) {
		addr = _addr;
	}
	
	std::vector<uint8_t> RFM22B::getAddress() {
		return addr;
	}
	
	// Tune for some seconds:
	void RFM22B::tune(unsigned int seconds) {
	
	}
	
	// Set the frequency of the carrier wave
	//	This function calculates the values of the registers 0x75-0x77 to achieve the
	//	desired carrier wave frequency (without any hopping set)
	//	Frequency should be passed in integer Hertz
	void RFM22B::setCarrierFrequency(unsigned int frequency) {
		// Don't set a frequency outside the range specified in the datasheet
		if (frequency < 240E6 || frequency > 960E6) {
			ostringstream oss;
			oss << "Cannot set carrier frequency to " << frequency/1E6f <<
					"MHz, out of range!";
			throw daisy_exception(oss.str());
		}

		// The following determines the register values, see Section 3.5.1 of the datasheet

		// Are we in the 'High Band'? (i.e. is hbsel == 1)
		uint8_t hbsel = (frequency >= 480E6);

		// What is the integer part of the frequency
		uint8_t fb = frequency/10E6/(hbsel+1) - 24;

		// Calculate register 0x75 from hbsel and fb. sbsel (bit 6) is always set
		uint8_t fbs = (1<<6) | (hbsel<<5) | fb;

		// Calculate the fractional part of the frequency
		uint16_t fc = (frequency/(10E6f*(hbsel+1)) - fb - 24) * 64000;

		// Split the fractional part in to most and least significant bits
		// (Registers 0x76 and 0x77 respectively)
		uint8_t ncf1 = (fc >> 8);
		uint8_t ncf0 = fc & 0xff;
	
		// Write the registers to the device
		this->setRegister(RFM22B_Register::FREQUENCY_BAND_SELECT, fbs);
		this->setRegister(RFM22B_Register::NOMINAL_CARRIER_FREQUENCY_1, ncf1);
		this->setRegister(RFM22B_Register::NOMINAL_CARRIER_FREQUENCY_0, ncf0);
	}
	
	// Get the frequency of the carrier wave in integer Hertz
	//	Without any frequency hopping
	unsigned int RFM22B::getCarrierFrequency() {
		// Read the register values
		uint8_t fbs = this->getRegister(RFM22B_Register::FREQUENCY_BAND_SELECT);
		uint8_t ncf1 = this->getRegister(RFM22B_Register::NOMINAL_CARRIER_FREQUENCY_1);
		uint8_t ncf0 = this->getRegister(RFM22B_Register::NOMINAL_CARRIER_FREQUENCY_0);
	
		// The following calculations ceom from Section 3.5.1 of the datasheet

		// Determine the integer part
		uint8_t fb = fbs & 0x1F;

		// Are we in the 'High Band'?
		uint8_t hbsel = (fbs >> 5) & 1;
	
		// Determine the fractional part
		uint16_t fc = (ncf1 << 8) | ncf0;
	
		// Return the frequency
		return 10E6*(hbsel+1)*(fb+24+fc/64000.0);
	}
	
	// Get and set the frequency hopping step size
	//	Values are in Hertz (to stay SI) but floored to the nearest 10kHz
	void RFM22B::setFrequencyHoppingStepSize(unsigned int step) {
		if (step > 2550000) {
			step = 2550000;
		}
		this->setRegister(RFM22B_Register::FREQUENCY_HOPPING_STEP_SIZE, step/10000);
	}
	unsigned int RFM22B::getFrequencyHoppingStepSize() {
		return this->getRegister(RFM22B_Register::FREQUENCY_HOPPING_STEP_SIZE)*10000;
	}
	
	// Get and set the frequency hopping channel
	void RFM22B::setChannel(uint8_t channel) {
		this->setRegister(RFM22B_Register::FREQUENCY_HOPPING_CHANNEL_SELECT, channel);
	}
	uint8_t RFM22B::getChannel() {
		return this->getRegister(RFM22B_Register::FREQUENCY_HOPPING_CHANNEL_SELECT);
	}
	
	// Set or get the frequency deviation (in Hz, but floored to the nearest 625Hz)
	void RFM22B::setFrequencyDeviation(unsigned int deviation) {
		if (deviation > 320000) {
			deviation = 320000;
		}
		this->setRegister(RFM22B_Register::FREQUENCY_DEVIATION, deviation/625);
	}
	unsigned int RFM22B::getFrequencyDeviation() {
		return this->getRegister(RFM22B_Register::FREQUENCY_DEVIATION)*625;
	}
	
	// Set or get the data rate (bps)
	void RFM22B::setDataRate(unsigned int rate) {
		// Get the Modulation Mode Control 1 register (for scaling bit)
		uint8_t mmc1 = this->getRegister(RFM22B_Register::MODULATION_MODE_CONTROL_1);

		uint16_t txdr;
		// Set the scale bit (5th bit of 0x70) high if data rate is below 30kbps
		// and calculate the value for txdr registers (0x6E and 0x6F)
		if (rate < 30000) {
			mmc1 |= (1<<5);
			txdr = rate * ((1 << (16 + 5)) / 1E6);
		} else {
			mmc1 &= ~(1<<5);
			txdr = rate * ((1 << (16)) / 1E6);
		}

		// Set the data rate bytes
		this->set16BitRegister(RFM22B_Register::TX_DATA_RATE_1, txdr);

		// Set the scaling byte
		this->setRegister(RFM22B_Register::MODULATION_MODE_CONTROL_1, mmc1);
	}
	unsigned int RFM22B::getDataRate() {
		// Get the data rate scaling value (5th bit of 0x70)
		uint8_t txdtrtscale = (this->getRegister(RFM22B_Register::MODULATION_MODE_CONTROL_1) >> 5) & 1;
	
		// Get the data rate registers
		uint16_t txdr = this->get16BitRegister(RFM22B_Register::TX_DATA_RATE_1);

		// Return the data rate (in bps, hence extra 1E3)
		return ((unsigned int) txdr * 1E6) / (1 << (16 + 5 * txdtrtscale));

	}
	
	// Set or get the modulation type
	void RFM22B::setModulationType(RFM22B_Modulation_Type modulation) {
		// Get the Modulation Mode Control 2 register
		uint8_t mmc2 = this->getRegister(RFM22B_Register::MODULATION_MODE_CONTROL_2);

		// Clear the modtyp bits
		mmc2 &= ~0x03;

		// Set the desired modulation
		mmc2 |= (uint8_t)modulation;
	
		// Set the register
		this->setRegister(RFM22B_Register::MODULATION_MODE_CONTROL_2, mmc2);
	}
	RFM22B_Modulation_Type RFM22B::getModulationType() {
		// Get the Modulation Mode Control 2 register
		uint8_t mmc2 = this->getRegister(RFM22B_Register::MODULATION_MODE_CONTROL_2);

		// Determine modtyp bits
		uint8_t modtyp = mmc2 & 0x03;

		// Ugly way to return correct enum
		switch (modtyp) {
			case 1:
				return RFM22B_Modulation_Type::OOK;
			case 2:
				return RFM22B_Modulation_Type::FSK;
			case 3:
				return RFM22B_Modulation_Type::GFSK;
			case 0:
			default:
				return RFM22B_Modulation_Type::UNMODULATED_CARRIER;
		}
	}
	
	void RFM22B::setModulationDataSource(RFM22B_Modulation_Data_Source source) {
		// Get the Modulation Mode Control 2 register
		uint8_t mmc2 = this->getRegister(RFM22B_Register::MODULATION_MODE_CONTROL_2);

		// Clear the dtmod bits
		mmc2 &= ~(0x03<<4);

		// Set the desired data source
		mmc2 |= (uint8_t)source << 4;
	
		// Set the register
		this->setRegister(RFM22B_Register::MODULATION_MODE_CONTROL_2, mmc2);
	}
	RFM22B_Modulation_Data_Source RFM22B::getModulationDataSource() {
		// Get the Modulation Mode Control 2 register
		uint8_t mmc2 = this->getRegister(RFM22B_Register::MODULATION_MODE_CONTROL_2);

		// Determine modtyp bits
		uint8_t dtmod = (mmc2 >> 4) & 0x03;

		// Ugly way to return correct enum
		switch (dtmod) {
			case 1:
				return RFM22B_Modulation_Data_Source::DIRECT_SDI;
			case 2:
				return RFM22B_Modulation_Data_Source::FIFO;
			case 3:
				return RFM22B_Modulation_Data_Source::PN9;
			case 0:
			default:
				return RFM22B_Modulation_Data_Source::DIRECT_GPIO;
		}
	}
	
	void RFM22B::setDataClockConfiguration(RFM22B_Data_Clock_Configuration clock) {
		// Get the Modulation Mode Control 2 register
		uint8_t mmc2 = this->getRegister(RFM22B_Register::MODULATION_MODE_CONTROL_2);

		// Clear the trclk bits
		mmc2 &= ~(0x03<<6);

		// Set the desired data source
		mmc2 |= (uint8_t)clock << 6;
	
		// Set the register
		this->setRegister(RFM22B_Register::MODULATION_MODE_CONTROL_2, mmc2);
	}
	RFM22B_Data_Clock_Configuration RFM22B::getDataClockConfiguration() {
		// Get the Modulation Mode Control 2 register
		uint8_t mmc2 = this->getRegister(RFM22B_Register::MODULATION_MODE_CONTROL_2);

		// Determine modtyp bits
		uint8_t dtmod = (mmc2 >> 6) & 0x03;

		// Ugly way to return correct enum
		switch (dtmod) {
			case 1:
				return RFM22B_Data_Clock_Configuration::GPIO;
			case 2:
				return RFM22B_Data_Clock_Configuration::SDO;
			case 3:
				return RFM22B_Data_Clock_Configuration::NIRQ;
			case 0:
			default:
				return RFM22B_Data_Clock_Configuration::NONE;
		}
	}
	
	// Set or get the transmission power
	void RFM22B::setTransmissionPower(uint8_t power) {
		// Saturate to maximum power
		if (power > 20) {
			power = 20;
		}
	
		// Get the TX power register
		uint8_t txp = this->getRegister(RFM22B_Register::TX_POWER);

		// Clear txpow bits
		txp &= ~(0x07);

		// Calculate txpow bits (See Section 5.7.1 of datasheet)
		uint8_t txpow = (power + 1) / 3;

		// Set txpow bits
		txp |= txpow;

		// Set the register
		this->setRegister(RFM22B_Register::TX_POWER, txp);
	}
	uint8_t RFM22B::getTransmissionPower() {
		// Get the TX power register
		uint8_t txp = this->getRegister(RFM22B_Register::TX_POWER);

		// Get the txpow bits
		uint8_t txpow = txp & 0x07;

		// Calculate power (see Section 5.7.1 of datasheet)
		if (txpow == 0) {
			return 1;
		} else {
			return txpow * 3 - 1;
		}
	}
	
	// Set or get the GPIO configuration
	void RFM22B::setGPIOFunction(RFM22B_GPIO gpio, RFM22B_GPIO_Function func) {
		// Get the GPIO register
		uint8_t gpioX = this->getRegister((RFM22B_Register)gpio);

		// Clear gpioX bits
		gpioX &= ~((1<<5)-1);

		// Set the gpioX bits
		gpioX |= (uint8_t)func;

		// Set the register
		this->setRegister((RFM22B_Register)gpio, gpioX);
	}
	
	uint8_t RFM22B::getGPIOFunction(RFM22B_GPIO gpio) {
		// Get the GPIO register
		uint8_t gpioX = this->getRegister((RFM22B_Register)gpio);
	
		// Return the gpioX bits
		// This should probably return an enum, but this needs a lot of cases
		return gpioX & ((1<<5)-1);
	}
	
	// Enable or disable interrupts
	void RFM22B::setInterruptEnable(RFM22B_Interrupt interrupt, bool enable) {
		// Get the (16 bit) register value
		uint16_t intEnable = this->get16BitRegister(RFM22B_Register::INTERRUPT_ENABLE_1);

		// Either enable or disable the interrupt
		if (enable) {
			intEnable |= (uint16_t)interrupt;
		} else {
			intEnable &= ~(uint16_t)interrupt;
		}

		// Set the (16 bit) register value
		this->set16BitRegister(RFM22B_Register::INTERRUPT_ENABLE_1, intEnable);
	}
	
	// Get the status of an interrupt
	bool RFM22B::getInterruptStatus(RFM22B_Interrupt interrupt) {
		// Get the (16 bit) register value
		uint16_t intStatus = this->get16BitRegister(RFM22B_Register::INTERRUPT_STATUS_1);

		// Determine if interrupt bit is set and return
		if ((intStatus & (uint16_t)interrupt) > 0) {
			return true;
		} else {
			return false;
		}
	}
	
	// Set the operating mode
	//	This function does not toggle individual pins as with other functions
	//	It expects a bitwise-ORed combination of the modes you want set
	void RFM22B::setOperatingMode(RFM22B_Operating_Mode mode) {
		this->set16BitRegister(RFM22B_Register::OPERATING_MODE_AND_FUNCTION_CONTROL_1, (uint16_t)mode);
	}

	// Get operating mode (bitwise-ORed)
	RFM22B_Operating_Mode RFM22B::getOperatingMode() {
		return (RFM22B_Operating_Mode)this->get16BitRegister(RFM22B_Register::OPERATING_MODE_AND_FUNCTION_CONTROL_1);
	}

	// Manuall enter RX or TX mode
	void RFM22B::enableRXMode() {
		this->setOperatingMode((RFM22B_Operating_Mode)
				((uint)RFM22B_Operating_Mode::READY_MODE | (uint)RFM22B_Operating_Mode::RX_MODE));
	}
	void RFM22B::enableTXMode() {
		this->setOperatingMode((RFM22B_Operating_Mode)
				((uint)RFM22B_Operating_Mode::READY_MODE | (uint)RFM22B_Operating_Mode::TX_MODE));
	}

	// Reset the device
	void RFM22B::reset() {
		this->setOperatingMode((RFM22B_Operating_Mode)
				((uint)RFM22B_Operating_Mode::READY_MODE | (uint)RFM22B_Operating_Mode::RESET));
	}
	
	// Set or get the trasmit header
	void RFM22B::setTransmitHeader(uint32_t header) {
		this->set32BitRegister(RFM22B_Register::TRANSMIT_HEADER_3, header);
	}
	uint32_t RFM22B::getTransmitHeader() {
		return this->get32BitRegister(RFM22B_Register::TRANSMIT_HEADER_3);
	}
	
	// Set or get the check header
	void RFM22B::setCheckHeader(uint32_t header) {
		this->set32BitRegister(RFM22B_Register::CHECK_HEADER_3, header);
	}
	uint32_t RFM22B::getCheckHeader() {
		return this->get32BitRegister(RFM22B_Register::CHECK_HEADER_3);
	}
	
	// Set or get the CRC mode
	void RFM22B::setCRCMode(RFM22B_CRC_Mode mode) {
		uint8_t dac = this->getRegister(RFM22B_Register::DATA_ACCESS_CONTROL);
	
		dac &= ~0x24;
	
		switch (mode) {
		case RFM22B_CRC_Mode::CRC_DISABLED:
			break;
		case RFM22B_CRC_Mode::CRC_DATA_ONLY:
			dac |= 0x24;
			break;
		case RFM22B_CRC_Mode::CRC_NORMAL:
		default:
			dac |= 0x04;
			break;
		}
	
		this->setRegister(RFM22B_Register::DATA_ACCESS_CONTROL, dac);
	}
	RFM22B_CRC_Mode RFM22B::getCRCMode() {
		uint8_t dac = this->getRegister(RFM22B_Register::DATA_ACCESS_CONTROL);
	
		if (! (dac & 0x04)) {
			return RFM22B_CRC_Mode::CRC_DISABLED;
		}
		if (dac & 0x20) {
			return RFM22B_CRC_Mode::CRC_DATA_ONLY;
		}
		return RFM22B_CRC_Mode::CRC_NORMAL;
	}
	
	// Set or get the CRC polynomial
	void RFM22B::setCRCPolynomial(RFM22B_CRC_Polynomial poly) {
		uint8_t dac = this->getRegister(RFM22B_Register::DATA_ACCESS_CONTROL);
	
		dac &= ~0x03;
	
		dac |= (uint8_t)poly;
	
		this->setRegister(RFM22B_Register::DATA_ACCESS_CONTROL, dac);
	}
	RFM22B_CRC_Polynomial RFM22B::getCRCPolynomial() {
		uint8_t dac = this->getRegister(RFM22B_Register::DATA_ACCESS_CONTROL);
	
		switch (dac & 0x03) {
		case 0:
			return RFM22B_CRC_Polynomial::CCITT;
		case 1:
			return RFM22B_CRC_Polynomial::CRC16;
		case 2:
			return RFM22B_CRC_Polynomial::IEC16;
		case 3:
			return RFM22B_CRC_Polynomial::BIACHEVA;
		}
		return RFM22B_CRC_Polynomial::CRC16;
	}
	
	// Get and set all the FIFO threshold
	void RFM22B::setTXFIFOAlmostFullThreshold(uint8_t thresh) {
		this->setFIFOThreshold(RFM22B_Register::TX_FIFO_CONTROL_1, thresh);
	}
	void RFM22B::setTXFIFOAlmostEmptyThreshold(uint8_t thresh) {
		this->setFIFOThreshold(RFM22B_Register::TX_FIFO_CONTROL_2, thresh);
	}
	void RFM22B::setRXFIFOAlmostFullThreshold(uint8_t thresh) {
		this->setFIFOThreshold(RFM22B_Register::RX_FIFO_CONTROL, thresh);
	}
	uint8_t RFM22B::getTXFIFOAlmostFullThreshold() {
		return this->getRegister(RFM22B_Register::TX_FIFO_CONTROL_1);
	}
	uint8_t RFM22B::getTXFIFOAlmostEmptyThreshold() {
		return this->getRegister(RFM22B_Register::TX_FIFO_CONTROL_2);
	}
	uint8_t RFM22B::getRXFIFOAlmostFullThreshold() {
		return this->getRegister(RFM22B_Register::RX_FIFO_CONTROL);
	}
	void RFM22B::setFIFOThreshold(RFM22B_Register reg, uint8_t thresh) {
		thresh &= ((1 << 6) - 1);
		this->setRegister(reg, thresh);
	}
	
	// Get RSSI value
	uint8_t RFM22B::getRSSI() {
		return this->getRegister(RFM22B_Register::RECEIVED_SIGNAL_STRENGTH_INDICATOR);
	}
	// Get input power (in dBm)
	//	Coefficients approximated from the graph in Section 8.10 of the datasheet
	int8_t RFM22B::getInputPower() {
		return 0.56*this->getRSSI()-128.8;
	}
	
	// Get length of last received packet
	uint8_t RFM22B::getReceivedPacketLength() {
		return this->getRegister(RFM22B_Register::RECEIVED_PACKET_LENGTH);
	}
	
	// Set length of packet to be transmitted
	void RFM22B::setTransmitPacketLength(uint8_t length) {
		return this->setRegister(RFM22B_Register::TRANSMIT_PACKET_LENGTH, length);
	}
	
	void RFM22B::clearRXFIFO() {
		//Toggle ffclrrx bit high and low to clear RX FIFO
		this->setRegister(RFM22B_Register::OPERATING_MODE_AND_FUNCTION_CONTROL_2, 2);
		this->setRegister(RFM22B_Register::OPERATING_MODE_AND_FUNCTION_CONTROL_2, 0);
	}
	
	void RFM22B::clearTXFIFO() {
		//Toggle ffclrtx bit high and low to clear TX FIFO
		this->setRegister(RFM22B_Register::OPERATING_MODE_AND_FUNCTION_CONTROL_2, 1);
		this->setRegister(RFM22B_Register::OPERATING_MODE_AND_FUNCTION_CONTROL_2, 0);
	}
	
	// Transfer data
	void RFM22B::transfer(uint8_t *tx, uint8_t *rx, size_t size) {
		if ((!tx) || (!rx))
			throw daisy_exception("NULL data");
		if (size > MAX_PACKET_LENGTH)
			throw daisy_exception(
					"Package too long (max 64)", to_string(size));
		transfer_lock.lock();
		cerr << "**TX: " << DaisyUtils::print(tx, size);
		bcm2835_spi_transfernb((char*)tx, (char*)rx, size);
		cerr << " - " << DaisyUtils::print(rx, size) << endl;
		transfer_lock.unlock();
	}

	// Send data
	void RFM22B::send(uint8_t *data, size_t length) {
		// Clear TX FIFO
		this->clearTXFIFO();

		// Initialise rx and tx arrays
		uint8_t tx[MAX_PACKET_LENGTH+1] = { 0 };
		uint8_t rx[MAX_PACKET_LENGTH+1] = { 0 };

		// Set FIFO register address (with write flag)
		tx[0] = (uint8_t)RFM22B_Register::FIFO_ACCESS | (1<<7);

		// Truncate data if its too long
		if (length > MAX_PACKET_LENGTH) {
			length = MAX_PACKET_LENGTH;
		}

		// Copy data from input array to tx array
		for (int i = 1; i <= length; i++) {
			tx[i] = data[i-1];
		}

		// Set the packet length
		this->setTransmitPacketLength(length);

		// Make the transfer
		this->transfer(tx,rx,length+1);

		// Enter TX mode
		this->enableTXMode();
	
		// Loop until packet has been sent (device has left TX mode)
		while (((this->getRegister(RFM22B_Register::OPERATING_MODE_AND_FUNCTION_CONTROL_1)>>3) & 1)) {}

		return;
	};
	
	// Receive data (blocking with timeout). Returns number of bytes received
	int RFM22B::receive(uint8_t *data, size_t length, int timeout) {
		// Make sure RX FIFO is empty, ready for new data
		this->clearRXFIFO();

		// Enter RX mode
		this->enableRXMode();

		// Initialise rx and tx arrays
		uint8_t tx[MAX_PACKET_LENGTH+1] = { 0 };
		uint8_t rx[MAX_PACKET_LENGTH+1] = { 0 };

		// Set FIFO register address
		tx[0] = (uint8_t)RFM22B_Register::FIFO_ACCESS;

		// Timing for the interrupt loop timeout
		struct timeval start, end;
		gettimeofday(&start, NULL);
		long elapsed = 0;

		// Loop endlessly on interrupt or timeout
		//	Don't use interrupt registers here as these don't seem to behave consistently
		//	Watch the operating mode register for the device leaving RX mode. This is indicitive
		//	of a valid packet being received
		while (((this->getRegister(RFM22B_Register::OPERATING_MODE_AND_FUNCTION_CONTROL_1)>>2) & 1) && elapsed < timeout) {
			// Determine elapsed time
			gettimeofday(&end, NULL);
			elapsed = (end.tv_usec - start.tv_usec)/1000 + (end.tv_sec - start.tv_sec)*1000;
		}

		// If timeout occured, return -1
		if (elapsed >= timeout) {
			return -1;
		}

		// Get length of packet received
		uint8_t rxLength = this->getReceivedPacketLength();
	
		if (rxLength > length) {
			rxLength = length;
		}

		// Make the transfer
		this->transfer(tx,rx,rxLength+1);

		// Copy the data to the output array
		for (int i = 1; i <= rxLength; i++) {
			data[i-1] = rx[i];
		}

		return rxLength;
	};
	
	// Helper function to read a single byte from the device
	uint8_t RFM22B::getRegister(RFM22B_Register reg) {
		// rx and tx arrays must be the same length
		// Must be 2 elements as the device only responds whilst it is being sent
		// data. tx[0] should be set to the requested register value and tx[1] left
		// clear. Once complete, rx[0] will be left clear (no data was returned whilst
		// the requested register was being sent), and rx[1] will contain the value
		uint8_t tx[] = {0x00, 0x00};
		uint8_t rx[] = {0x00, 0x00};

		tx[0] = (uint8_t)reg;

		this->transfer(tx,rx,2);

		return rx[1];
	}
	
	// Similar to function above, but for readying 2 consequtive registers as one
	uint16_t RFM22B::get16BitRegister(RFM22B_Register reg) {
		uint8_t tx[] = {0x00, 0x00, 0x00};
		uint8_t rx[] = {0x00, 0x00, 0x00};

		tx[0] = (uint8_t)reg;

		this->transfer(tx,rx,3);
	
		return (rx[1] << 8) | rx[2];
	}
	
	// Similar to function above, but for readying 4 consequtive registers as one
	uint32_t RFM22B::get32BitRegister(RFM22B_Register reg) {
		uint8_t tx[] = {0x00, 0x00, 0x00, 0x00, 0x00};
		uint8_t rx[] = {0x00, 0x00, 0x00, 0x00, 0x00};

		tx[0] = (uint8_t)reg;

		this->transfer(tx,rx,5);
	
		return (rx[1] << 24) | (rx[2] << 16) | (rx[3] << 8) | rx[4];
	}
	
	// Helper function to write a single byte to a register
	void RFM22B::setRegister(RFM22B_Register reg, uint8_t value) {
		// tx and rx arrays required even though we aren't receiving anything
		uint8_t tx[] = {0x00, 0x00};
		uint8_t rx[] = {0x00, 0x00};

		// tx[0] is the requested register with the final bit set high to indicate
		// a write operation (see Section 3.1 of the datasheet)
		tx[0] = (uint8_t)reg | (1<<7);

		// tx[1] is the value to be set
		tx[1] = value;

		this->transfer(tx,rx,2);
	}
	
	// As above, but for 2 consequitive registers
	void RFM22B::set16BitRegister(RFM22B_Register reg, uint16_t value) {
		// tx and rx arrays required even though we aren't receiving anything
		uint8_t tx[] = {0x00, 0x00, 0x00};
		uint8_t rx[] = {0x00, 0x00, 0x00};

		// tx[0] is the requested register with the final bit set high to indicate
		// a write operation (see Section 3.1 of the datasheet)
		tx[0] = (uint8_t)reg | (1<<7);

		// tx[1-2] is the value to be set
		tx[1] = (value >> 8);
		tx[2] = (value) & 0xFF;

		this->transfer(tx,rx,3);
	}
	
	// As above, but for 4 consequitive registers
	void RFM22B::set32BitRegister(RFM22B_Register reg, uint32_t value) {
		// tx and rx arrays required even though we aren't receiving anything
		uint8_t tx[] = {0x00, 0x00, 0x00, 0x00, 0x00};
		uint8_t rx[] = {0x00, 0x00, 0x00, 0x00, 0x00};

		// tx[0] is the requested register with the final bit set high to indicate
		// a write operation (see Section 3.1 of the datasheet)
		tx[0] = (uint8_t)reg | (1<<7);

		// tx[1-4] is the value to be set
		tx[1] = (value >> 24);
		tx[2] = (value >> 16) & 0xFF;
		tx[3] = (value >> 8) & 0xFF;
		tx[4] = (value) & 0xFF;

		this->transfer(tx,rx,5);
	}

} // end namespace //
