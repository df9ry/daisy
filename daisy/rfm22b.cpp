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
#include <iomanip>
#include <cstring>
#include <chrono>
#include <random>
#include <algorithm>

#include <unistd.h>
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

class Timer {
public:
    Timer() : beg_(clock_::now()) {}
    void reset() { beg_ = clock_::now(); }
    double elapsed() const {
        return std::chrono::duration_cast<second_>
            (clock_::now() - beg_).count(); }
private:
    typedef std::chrono::high_resolution_clock clock_;
    typedef std::chrono::duration<double, std::ratio<1> > second_;
    std::chrono::time_point<clock_> beg_;
};

class Random {
public:
    Random() = default;
    Random(std::mt19937::result_type seed) : eng(seed) {}
    uint8_t get() {
    	return std::uniform_int_distribution<uint8_t>{0, 255}(eng);
    }
private:
    std::mt19937 eng{std::random_device{}()};
};

class RandomPackageGenerator {
public:
	RandomPackageGenerator(size_t n): n_packages{n}	{}

	int get(uint8_t *pb_buf, size_t cb_buf) {
		if (i_packages++ >= n_packages) {
			elapsed = timer.elapsed();
			return -1;
		}
		size_t cb = rnd.get() % cb_buf;
		for (int i = 0; i < cb; ++i)
			pb_buf[i] = rnd.get();
		n_octets += cb;
		return cb;
	}

	void printStatistics() {
		double ms_per_octet =  1000 * ( elapsed / (double)n_octets );
		double rate = 8.0 / ms_per_octet;
		cout << n_octets << " octets" << endl;
		cout << setprecision(3);
		cout << elapsed << "s total" << endl;
		cout << ms_per_octet << " ms/octet" << endl;
		cout << rate << " kbps" << endl;
	}

private:
	Random   rnd;
	Timer    timer;
	size_t   n_packages;
	size_t   i_packages  = 0;
	uint64_t n_octets    = 0;
	double   elapsed     = 0.0;
};

namespace RFM22B_NS {

	// Settings for GFSK 10kbps, 50kHz deviation on 434.150 MHz:
	static struct register_value init_narrow[] = {
		 { 0x1c|0x80, 0x2B }, // IF Filter Bandwidth
		 { 0x1d|0x80, 0x40 }, // AFC Loop Gearshift Overide
		 { 0x20|0x80, 0xF4 }, // Clock Recovery Oversampling Rate
		 { 0x21|0x80, 0x20 }, // Clock Recovery Offset 2
		 { 0x22|0x80, 0x20 }, // Clock Recovery Offset 1
		 { 0x23|0x80, 0xC5 }, // Clock Recovery Offset 0
		 { 0x24|0x80, 0x00 }, // Clock Recovery Timing Loop Gain 1
		 { 0x25|0x80, 0x26 }, // Clock Recovery Timing Loop Gain 0
		 { 0x2a|0x80, 0x1D }, // AFC Limiter
		 { 0x30|0x80, 0x8F }, // Data Access Control
		 { 0x32|0x80, 0x8C }, // Header Control 1
		 { 0x33|0x80, 0x02 }, // Header Control 2
		 { 0x34|0x80, 0x08 }, // Preamble Length
		 { 0x35|0x80, 0x2A }, // Preamble Detection Control 1
		 { 0x36|0x80, 0x2D }, // Synchronization Word 3
		 { 0x37|0x80, 0xD4 }, // Synchronization Word 2
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
		 { 0x43|0x80, 0xFF }, // Header Enable 3
		 { 0x44|0x80, 0xFF }, // Header Enable 2
		 { 0x45|0x80, 0xFF }, // Header Enable 1
		 { 0x46|0x80, 0xFF }, // Header Enable 0
		 { 0x6e|0x80, 0x08 }, // TX Data Rate 1
		 { 0x6f|0x80, 0x31 }, // TX Data Rate 0
		 { 0x70|0x80, 0x2E }, // Modulation Mode Control 1
		 { 0x71|0x80, 0x23 }, // Modulation Mode Control 2
		 { 0x72|0x80, 0x08 }, // Frequency Deviation
		 { 0x75|0x80, 0x53 }, // Frequency Band Select
		 { 0x76|0x80, 0x67 }, // Nominal Carrier Frequency
		 { 0x77|0x80, 0xC0 }, // Nominal Carrier Frequency
		 { 0x00,      0x00 }  // Termination NULL Entry
	};

	// Settings for GFSK 100kbps, 50kHz deviation on 434.150 MHz:
	static struct register_value init_medium[] = {
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

	// Settings for GFSK 250kbps, 50kHz deviation on 434.150 MHz:
	static struct register_value init_wide[] = {
		 { 0x1c|0x80, 0x89 }, // IF Filter Bandwidth
		 { 0x1d|0x80, 0x40 }, // AFC Loop Gearshift Overide
		 { 0x20|0x80, 0x30 }, // Clock Recovery Oversampling Rate
		 { 0x21|0x80, 0x02 }, // Clock Recovery Offset 2
		 { 0x22|0x80, 0xaa }, // Clock Recovery Offset 1
		 { 0x23|0x80, 0xab }, // Clock Recovery Offset 0
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
		 { 0x6e|0x80, 0x40 }, // TX Data Rate 1
		 { 0x6f|0x80, 0x00 }, // TX Data Rate 0
		 { 0x70|0x80, 0x0d }, // Modulation Mode Control 1
		 { 0x71|0x80, 0x23 }, // Modulation Mode Control 2
		 { 0x72|0x80, 0x50 }, // Frequency Deviation
		 { 0x75|0x80, 0x53 }, // Frequency Band Select
		 { 0x76|0x80, 0x67 }, // Nominal Carrier Frequency
		 { 0x77|0x80, 0xc0 }, // Nominal Carrier Frequency
		 { 0x00,      0x00 }  // Termination NULL Entry
	};

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
		bcm2835_spi_setClockDivider(BUS_CLOCK_DIVIDER);
		unsigned int device = getRegister(RFM22B_Register::DEVICE_TYPE);
		if (device != 8)
			throw daisy_exception("Invalid device", to_string(device));
		setNarrowMode();
	}
	
	// Destructor:
	RFM22B::~RFM22B() {
		if (init_state > 1)
			bcm2835_spi_end();
		if (init_state > 0)
			bcm2835_close();
		init_state = 0;
	}

	uint8_t RFM22B::getDeviceType() {
		return getRegister(RFM22B_Register::DEVICE_TYPE);
	}

	uint8_t RFM22B::getDeviceVersion() {
		return getRegister(RFM22B_Register::DEVICE_VERSION);
	}

	uint8_t RFM22B::getDeviceStatus() {
		return getRegister(RFM22B_Register::DEVICE_STATUS);
	}

	// Set the header address.
	void RFM22B::setAddress(const vector<uint8_t>& _addr) {
		addr = _addr;
	}
	
	std::vector<uint8_t> RFM22B::getAddress() {
		return addr;
	}
	
	void RFM22B::setNarrowMode() {
		reset();
		init(init_narrow);
		sleep(1);
	}

	void RFM22B::setMediumMode() {
		reset();
		init(init_medium);
		sleep(1);
	}

	void RFM22B::setWideMode() {
		reset();
		init(init_wide);
		sleep(1);
	}

	// Tune for some seconds:
	void RFM22B::tune(unsigned int seconds) {
		RFM22B_Modulation_Data_Source mds_save = getModulationDataSource();
		setModulationDataSource(RFM22B_Modulation_Data_Source::PN9);
		enableTXMode();
		sleep(seconds);
		setModulationDataSource(mds_save);
	}
	
	void RFM22B::tx_packages(unsigned int numpkts) {
		try {
			cout << "Now sending " << numpkts << " packages ... ";
			if (verbose)
				cout << endl;
			else
				cout.flush();
			RandomPackageGenerator rpg(numpkts);
			send([&rpg](uint8_t *pb, size_t cb) -> int{ return rpg.get(pb, cb); });
			rpg.printStatistics();
		}
		catch (exception& ex) {
			cerr << "Exception caught: " << ex.what() << endl;
		}
		catch (...) {
			cerr << "Unknown exception caught!" << endl;
		}
	}

	void RFM22B::rx_packages(unsigned int timeout) {
		try {
		/*
		if (verbose)
			receive([](const uint8_t *pb, size_t cb) {
				DaisyUtils::dump(cout, pb, cb);
			}, timeout);
		else
		*/
			receive(NULL, timeout);
		}
		catch (exception& ex) {
			cerr << "Exception caught: " << ex.what() << endl;
		}
		catch (...) {
			cerr << "Unknown exception caught!" << endl;
		}
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
		if (deviation >= 320000)
			throw daisy_exception("Deviation too large (0..319_999)", to_string(deviation));
		uint16_t v = deviation/625;
		setRegister(RFM22B_Register::FREQUENCY_DEVIATION, v & 0x00ff);
		uint8_t mmc2 = getRegister(RFM22B_Register::MODULATION_MODE_CONTROL_2);
		mmc2 = (v & 0x0100) ? mmc2 | (1<<2) : mmc2 & ~(1<<2);
		setRegister(RFM22B_Register::MODULATION_MODE_CONTROL_2, mmc2);
	}
	unsigned int RFM22B::getFrequencyDeviation() {
		uint16_t v = getRegister(RFM22B_Register::FREQUENCY_DEVIATION);
		if (getRegister(RFM22B_Register::MODULATION_MODE_CONTROL_2) & (1<<2))
			v |= 0x0100;
		return v * 625;
	}
	
	// Set or get the data rate (bps)
	void RFM22B::setDataRate(unsigned int rate) {
		if (rate < 123)
			throw daisy_exception("Data rate too low (123..256_000)", to_string(rate));
		if (rate > 256000)
			throw daisy_exception("Data rate too high (123..256_000)", to_string(rate));
		// Set the scale bit (5th bit of 0x70) high if data rate is below 30kbps
		// and calculate the value for txdr registers (0x6E and 0x6F)
		uint8_t mmc1 = getRegister(RFM22B_Register::MODULATION_MODE_CONTROL_1);
		double _rate = rate;
		if (_rate < 30000.0) {
			mmc1 |= (1<<5);
			_rate = ((_rate * (1<<21))) / 1.0E6;
		} else {
			mmc1 &= ~(1<<5);
			_rate = ((_rate * (1<<16))) / 1.0E6;
		}
		uint32_t x = round(_rate);
		setRegister(RFM22B_Register::MODULATION_MODE_CONTROL_1, mmc1);
		set16BitRegister(RFM22B_Register::TX_DATA_RATE_1, round(x));
		// Register 58 hint:
		setRegister(RFM22B_Register::FAST_TX, (rate > 100000)?0xc0:0x80);
	}

	unsigned int RFM22B::getDataRate() {
		double x = get16BitRegister(RFM22B_Register::TX_DATA_RATE_1);
		return round((getRegister(RFM22B_Register::MODULATION_MODE_CONTROL_1) & (1<<5))?
				     (1.0E6 * x) / (1<<21) : (1.0E6 * x) / (1<<16));
	}
	
	struct BW {
		unsigned int bw;
		uint8_t      ndec_exp;
		uint8_t      dwn3_bypass;
		uint8_t      filset;
	};

	static BW bwtable[] = {
			{   2600, 5, 0,  1 }, {   2800, 5, 0,  2 },
			{   3100, 5, 0,  3 }, {   3200, 5, 0,  4 },
			{   3700, 5, 0,  5 }, {   4200, 5, 0,  6 },
			{   4500, 5, 0,  7 }, {   4900, 4, 0,  1 },
			{   5400, 4, 0,  2 }, {   5900, 4, 0,  3 },
			{   6100, 4, 0,  4 }, {   7200, 4, 0,  5 },
			{   8200, 4, 0,  6 }, {   8800, 4, 0,  7 },
			{   9500, 3, 0,  1 }, {  10600, 3, 0,  2 },
			{  11500, 3, 0,  3 }, {  12100, 3, 0,  4 },
			{  14200, 3, 0,  5 }, {  16200, 3, 0,  6 },
			{  17500, 3, 0,  7 }, {  18900, 2, 0,  1 },
			{  21000, 2, 0,  2 }, {  22700, 2, 0,  3 },
			{  24000, 2, 0,  4 }, {  28200, 2, 0,  5 },
			{  32200, 2, 0,  6 }, {  34700, 2, 0,  7 },
			{  37700, 1, 0,  1 }, {  41700, 1, 0,  2 },
			{  45200, 1, 0,  3 }, {  47900, 1, 0,  4 },
			{  56200, 1, 0,  5 }, {  64100, 1, 0,  6 },
			{  69200, 1, 0,  7 }, {  75200, 0, 0,  1 },
			{  83200, 0, 0,  2 }, {  90000, 0, 0,  3 },
			{  95300, 0, 0,  4 }, { 112100, 0, 0,  5 },
			{ 127900, 0, 0,  6 }, { 137900, 0, 0,  7 },
			{ 142800, 1, 1,  4 }, { 167800, 1, 1,  5 },
			{ 181100, 1, 1,  9 }, { 191500, 0, 1, 15 },
			{ 225100, 0, 1,  1 }, { 248800, 0, 1,  2 },
			{ 269300, 0, 1,  3 }, { 284900, 0, 1,  4 },
			{ 269300, 0, 1,  3 }, { 284900, 0, 1,  4 },
			{ 335500, 0, 1,  8 }, { 361800, 0, 1,  9 },
			{ 420200, 0, 1, 10 }, { 468400, 0, 1, 11 },
			{ 518800, 0, 1, 12 }, { 577000, 0, 1, 13 },
			{ 620700, 0, 1, 14 }, {      0, 0, 1, 13 }
	};

	void RFM22B::setBandwidth(unsigned int bw) {
		BW *_bw;
		for (_bw = bwtable; _bw->bw != 0; ++_bw) {
			if (_bw->bw >= bw)
				break;
		} // end for //
		unsigned int x =
				(_bw->dwn3_bypass << 7) |
				(_bw->ndec_exp    << 4) |
				(_bw->filset          );
		setRegister(RFM22B_Register::IF_FILTER_BANDWIDTH, x);
	}

	unsigned int RFM22B::getBandwidth() {
		BW *_bw;
		unsigned int x = getRegister(RFM22B_Register::IF_FILTER_BANDWIDTH);
		uint8_t _dwn3_bypass, _ndec_exp, _filset;
		for (_bw = bwtable; _bw->bw != 0; ++_bw) {
			_dwn3_bypass = (x & 0x80) >> 7;
			_ndec_exp    = (x & 0x70) >> 4;
			_filset      = (x & 0x0f);
			if ((_bw->dwn3_bypass == _dwn3_bypass) &&
				(_bw->ndec_exp    == _ndec_exp   ) &&
				(_bw->filset      == _filset     ))
			{
				return _bw->bw;
			}
		} // end for //
		return 0; // Not a useful setting.
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
	
	// Set or get the modulation mode
	void RFM22B::setModulationMode(RFM22B_Modulation_Mode mode) {
		// Get the Modulation Mode Control 1 register
		uint8_t mmc1 = this->getRegister(RFM22B_Register::MODULATION_MODE_CONTROL_1);

		// Clear the modtyp bits
		mmc1 &= ~0x0f;

		// Set the desired modes
		mmc1 |= (uint8_t)mode;

		// Set the register
		this->setRegister(RFM22B_Register::MODULATION_MODE_CONTROL_1, mmc1);
	}

	RFM22B_Modulation_Mode RFM22B::getModulationMode() {
		// Get the Modulation Mode Control 1 register
		uint8_t mmc1 = this->getRegister(RFM22B_Register::MODULATION_MODE_CONTROL_1);

		// Determine modtyp bits
		return (RFM22B_Modulation_Mode) (mmc1 & 0x0f);
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
	
	// Set or get the preamble length
	void RFM22B::setPreambleLength(unsigned int len) {
		if (len >= 1024)
			throw daisy_exception("Preamble too long", to_string(len));
		if (len % 4 != 0)
			len += 4;
		setRegister(RFM22B_Register::PREAMBLE_LENGTH, len/4);
	}

	unsigned int RFM22B::getPreambleLength() {
		return getRegister(RFM22B_Register::PREAMBLE_LENGTH) * 4;
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
	
	RFM22B_GPIO_Function RFM22B::getGPIOFunction(RFM22B_GPIO gpio) {
		// Get the GPIO register
		uint8_t gpioX = this->getRegister((RFM22B_Register)gpio);
	
		// Return the gpioX bits
		// This should probably return an enum, but this needs a lot of cases
		return (RFM22B_GPIO_Function)(gpioX & ((1<<5)-1));
	}
	
#if SIMULATE_INTERRUPTS
	void RFM22B::intrtask() {
		try {
			while(!intrstop) {
				di();
				// Get interrupt status register
				uint16_t intrstat =
						get16BitRegister(RFM22B_Register::INTERRUPT_STATUS_1);
				// mask out all not enabled bits:
				intrstat &= intrmask;
#ifdef DETECT_ONLY_RISING_EDGES
				if (intrstat != intrhold) {
					rising_edges |= (intrstat^intrhold)&intrstat&intrmask;
					intrhold = intrstat;
				}
				if (rising_edges) {
					intrpending = true;
					intoccurred.unlock();
					continue;
				}
#else
				if (intrstat != intrhold) {
					rising_edges = intrhold = intrstat;
					intrpending = true;
					intoccurred.unlock();
					continue;
				}
#endif
				if (intrstop)
					break;
				ei();
				usleep(INTERRUPT_POLL_TIME);
			} // end while //
		}
		catch (exception& ex) {
			cerr << "Exception caught in interrupt thread: " 
				 << ex.what() << endl;
		}
		catch (...) {
			cerr << "Unknown exception caught in interrupt thread!" 
				 << endl;
		}

	}

	static void task(RFM22B* me) {
		try {
			me->intrtask();
		}
		catch (...) {
			cerr << "!!! Exception in interrupt thread" << endl;
		}
	}
#endif

	// Enable or disable interrupts
	void RFM22B::setInterruptEnable(RFM22B_Interrupt interrupt, bool enable) {
#if SIMULATE_INTERRUPTS
		if (enable) {
			intrhold &= ~(uint16_t) interrupt; // Initialize hold
			if (!intrmask) {
				intlocked.unlock();
				intoccurred.unlock();
				intrstop = false;
				intrhold = 0x0000;
				intoccurred.lock();
				intrhold = 0x0000;
				rising_edges = 0x0000;
				intrthread = move(thread(task, this));
			}
			intrmask |= (uint16_t) interrupt;
		} else if (intrmask & (uint16_t) interrupt) {
			intrhold &= ~(uint16_t) interrupt; // Initialize hold
			intrmask &= ~(uint16_t) interrupt;
			if (!intrmask) {
				intrstop = true;
				ei(); // To allow stop flag detection.
				intrthread.join();
				intrthread = move(thread());
				intrhold = 0x0000;
				rising_edges = 0x0000;
				intoccurred.unlock();
			}
		}
#else
		// Get the (16 bit) register value
		uint16_t intEnable = get16BitRegister(RFM22B_Register::INTERRUPT_ENABLE_1);
		// Either enable or disable the interrupt
		if (enable) {
			intEnable |= (uint16_t)interrupt;
		} else {
			intEnable &= ~(uint16_t)interrupt;
		}
		// Set the (16 bit) register value
		set16BitRegister(RFM22B_Register::INTERRUPT_ENABLE_1, intEnable);
#endif
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
		set16BitRegister(RFM22B_Register::OPERATING_MODE_AND_FUNCTION_CONTROL_1, (uint16_t)mode);
	}

	// Get operating mode (bitwise-ORed)
	RFM22B_Operating_Mode RFM22B::getOperatingMode() {
		return (RFM22B_Operating_Mode)this->get16BitRegister(RFM22B_Register::OPERATING_MODE_AND_FUNCTION_CONTROL_1);
	}

	// Manuall enter RX or TX mode
	void RFM22B::enableRXMode() {
		setOperatingMode((RFM22B_Operating_Mode)
			((uint)getOperatingMode() |
			 (uint)RFM22B_Operating_Mode::RX_MODE |
			 (uint)RFM22B_Operating_Mode::RX_MULTI_PACKET));
	}

	void RFM22B::disableRXMode() {
		setOperatingMode((RFM22B_Operating_Mode)
			((uint)getOperatingMode() &
			 ~(uint)RFM22B_Operating_Mode::RX_MODE &
			 ~(uint)RFM22B_Operating_Mode::RX_MULTI_PACKET));
	}

	void RFM22B::enableTXMode() {
		setOperatingMode((RFM22B_Operating_Mode)
			((uint)getOperatingMode() |
			 (uint)RFM22B_Operating_Mode::TX_MODE |
			 (uint)RFM22B_Operating_Mode::AUTOMATIC_TRANSMISSION));
	}

	void RFM22B::disableTXMode() {
		setOperatingMode((RFM22B_Operating_Mode)
			((uint)getOperatingMode() &
			~(uint)RFM22B_Operating_Mode::TX_MODE &
			~(uint)RFM22B_Operating_Mode::AUTOMATIC_TRANSMISSION));
	}

	// Reset the device
	void RFM22B::reset() {
		setOperatingMode((RFM22B_Operating_Mode)
				((uint)getOperatingMode() | (uint)RFM22B_Operating_Mode::RESET));
		while ((uint)getOperatingMode() & (uint)RFM22B_Operating_Mode::RESET)
			usleep(1);
		usleep(20000);
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
	
	// Set squelch
	void RFM22B::setSquelch(uint8_t level) {
		setRegister(RFM22B_Register::RSSI_THRESHOLD_FOR_CLEAR_CHANNEL_INDICATOR,
				level);
	}

	// Get squelch
	uint8_t RFM22B::getSquelch() {
		return getRegister(
				RFM22B_Register::RSSI_THRESHOLD_FOR_CLEAR_CHANNEL_INDICATOR);
	}

	// Get length of last received packet
	uint8_t RFM22B::getReceivedPacketLength() {
		return getRegister(RFM22B_Register::RECEIVED_PACKET_LENGTH);
	}
	
	// Set length of packet to be transmitted
	void RFM22B::setTransmitPacketLength(uint8_t length) {
		return this->setRegister(RFM22B_Register::TRANSMIT_PACKET_LENGTH, length);
	}
	
	void RFM22B::clearRXFIFO() {
		//Toggle ffclrrx bit high and low to clear RX FIFO
		uint8_t x = getRegister(
				RFM22B_Register::OPERATING_MODE_AND_FUNCTION_CONTROL_2);
		setRegister(RFM22B_Register::OPERATING_MODE_AND_FUNCTION_CONTROL_2,
				x | 0x02);
		usleep(10);
		setRegister(RFM22B_Register::OPERATING_MODE_AND_FUNCTION_CONTROL_2,
				x & ~0x02);
	}
	
	void RFM22B::clearTXFIFO() {
		//Toggle ffclrtx bit high and low to clear TX FIFO
		uint8_t x = getRegister(
				RFM22B_Register::OPERATING_MODE_AND_FUNCTION_CONTROL_2);
		setRegister(RFM22B_Register::OPERATING_MODE_AND_FUNCTION_CONTROL_2,
				x | 0x01);
		usleep(10);
		setRegister(RFM22B_Register::OPERATING_MODE_AND_FUNCTION_CONTROL_2,
				x & ~0x01);
	}
	
	// Transfer data
	void RFM22B::transfer(uint8_t *tx, uint8_t *rx, size_t size) {
		if ((!tx) || (!rx))
			throw daisy_exception("NULL data");
		if (size > MAX_PACKET_LENGTH+1)
			throw daisy_exception(
					"Package too long (max 65)", to_string(size));
		transfer_lock.lock();
		if (debug)
			cerr << "**TX: " << DaisyUtils::print(tx, size) << endl;
		bcm2835_spi_transfernb((char*)tx, (char*)rx, size);
		if (debug)
			cerr << "**RX: " << DaisyUtils::print(rx, size) << endl;
		transfer_lock.unlock();
	}

	// Send data
	void RFM22B::send(function<int(uint8_t *pb, size_t cb)> output) {
		if (!output)
			return;

		uint8_t   data[256];
		uint8_t   tx[MAX_PACKET_LENGTH+1];
		uint8_t   rx[MAX_PACKET_LENGTH+1];
		int       packageleft;
		int       indexinpackage;
		int       tosend;
		uint64_t  overflows = 0;
		uint64_t  underflows = 0;
		const int refillmax = MAX_PACKET_LENGTH - 
				getTXFIFOAlmostEmptyThreshold();
		Timer     timer;

		setInterruptEnable(RFM22B_Interrupt::TX_FIFO_ALMOST_EMPTY_INT, true);
		setInterruptEnable(RFM22B_Interrupt::FIFO_UNDERFLOW_OVERFLOW,  true);
		setInterruptEnable(RFM22B_Interrupt::PACKET_SENT,              true);
		
		RFM22B_Modulation_Data_Source mds_save = getModulationDataSource();
		setModulationDataSource(RFM22B_Modulation_Data_Source::FIFO);

		// Clear the TX fifo to get a clean start:
		clearTXFIFO();
		
		// Get first data block:
		packageleft = output(data, sizeof(data));
		if (packageleft < 0) // EOF
			return;
		if (verbose) {
			cout << "<<<Output " << packageleft << " octets>>>" << endl;
			DaisyUtils::dump(cout, data, packageleft);
		}
		setTransmitPacketLength(packageleft);
		enableTXMode();
		indexinpackage = 0;
		// Main loop:
		timer.reset();
		while (packageleft >= 0) {
			int32_t status = try_waitforinterrupt();
			if (status < 0) {
				usleep(INTERRUPT_POLL_TIME);
				if (timer.elapsed() < 2.00)
					continue;
				if (verbose)
					cout << "<##Transmission stalled##>" << endl;
				enableTXMode();
			} else {
				timer.reset();
			}
			eoi();
			
			/*** PACKET_SENT ***/
			if (status & (uint16_t) RFM22B_Interrupt::PACKET_SENT) {
				if (verbose)
					cout << "<<<Packet sent>>>" << endl;
				packageleft = output(data, sizeof(data));
				if (packageleft >= 0) {
					if (verbose) {
						cout << "<<<Output " << packageleft << " octets>>>"
							 << endl;
						DaisyUtils::dump(cout, data, packageleft);
					}
					setTransmitPacketLength(packageleft);
					enableTXMode();
				} else {
					if (verbose)
						cout << "<== End of file==>" << endl;
					break;
				}
				continue;
			}

			/*** TX_FIFO_ALMOST_EMPTY_INT ***/
			if (status & (uint16_t) RFM22B_Interrupt::TX_FIFO_ALMOST_EMPTY_INT) 
			{
				if (verbose)
					cout << "<<<FIFO almost empty>>>" << endl;
				if (packageleft > 0) {
					tosend = (packageleft > refillmax) ?
							refillmax : packageleft;
					if (debug)
						cout << "*indexinpackage=" << indexinpackage
							 << ",packageleft="    << packageleft
							 << ",tosend="         << tosend
							 << endl;
					tx[0] = 0xff;
					memcpy(&tx[1], &data[indexinpackage], tosend);
					transfer(tx, rx, tosend+1);
					indexinpackage += tosend;
					packageleft -= tosend;
				} else {
					if (verbose)
						cout << "<==End of data==>" << endl;
				}
				continue;
			}

			/*** FIFO_UNDERFLOW_OVERFLOW ***/
			if (status & (uint16_t) RFM22B_Interrupt::FIFO_UNDERFLOW_OVERFLOW) {
				if (verbose)
					cout << "<<<Over-/Underflow>>>" << endl;
				uint8_t x = getRegister(RFM22B_Register::DEVICE_STATUS);
				if (x & 0x80) {
					if (verbose)
						cout << "<==Overflow==>" << endl;
					++overflows;
				}
				if (x & 0x40) {
					if (verbose)
						cout << "<==Underflow==>" << endl;
					++underflows;
				}
				clearTXFIFO();
				
				packageleft = output(data, sizeof(data));
				if (packageleft >= 0) {
					if (verbose) {
						cout << "<<<Output " << packageleft << " octets>>>"
						     << endl;
						DaisyUtils::dump(cout, data, packageleft);
					}
					setTransmitPacketLength(packageleft);
					enableTXMode();
				} else {
					if (verbose)
						cout << "<==End of file==>" << endl;
					break;
				}
			}

		} // end while //
		if (verbose)
			cout << "<--Main loop left-->" << endl;
		disableTXMode();
		setInterruptEnable(RFM22B_Interrupt::FIFO_UNDERFLOW_OVERFLOW,  false);
		setInterruptEnable(RFM22B_Interrupt::TX_FIFO_ALMOST_EMPTY_INT, false);
		setInterruptEnable(RFM22B_Interrupt::PACKET_SENT,              false);
		// Wait for completion of transmission:
		while ((uint16_t)getOperatingMode() &
			   (uint16_t)RFM22B_Operating_Mode::TX_MODE)
			usleep(100);
		setModulationDataSource(mds_save);
		cout << "done" << endl;
		cout << overflows << " overflows, " << underflows << " underflows."
			 << endl;
	};
	
	// Receive data (blocking with timeout). Returns number of bytes received
	void RFM22B::receive(
			function<void(uint8_t*,size_t)> input, unsigned int timeout)
	{
		clearRXFIFO();
		setInterruptEnable(RFM22B_Interrupt::RSSI,                    true);
		setInterruptEnable(RFM22B_Interrupt::VALID_PREAMBLE,          true);
		setInterruptEnable(RFM22B_Interrupt::SYNC_WORD,               true);
		setInterruptEnable(RFM22B_Interrupt::CRC_ERROR,               true);
		setInterruptEnable(RFM22B_Interrupt::FIFO_UNDERFLOW_OVERFLOW, true);
		setInterruptEnable(RFM22B_Interrupt::VALID_PACKET_RECEIVED,   true);
		setInterruptEnable(RFM22B_Interrupt::RX_FIFO_ALMOST_FULL_INT, true);
		Timer timer;
		setRXFIFOAlmostFullThreshold(40);
		int rxbffaful = getRXFIFOAlmostFullThreshold();
		uint32_t crcerrors = 0, overflows = 0, underflows = 0, valids = 0;
		uint64_t octets = 0;
		uint8_t data[256];
		uint8_t txb[MAX_PACKET_LENGTH+1]; uint8_t rxb[MAX_PACKET_LENGTH+1];
		txb[0] = 0x7f; memset(&txb[1], 0x00, sizeof(txb)-1);
		int alreadyreceived = 0, packagelength = 0;
		bool sync = false;

		cout << "Now listening for " << timeout << "s ... ";
		cout.flush();
		enableRXMode();
		while (timer.elapsed() < timeout) {
			int32_t status = try_waitforinterrupt();
			if (status < 0) {
				usleep(INTERRUPT_POLL_TIME);
				continue;
			}
			eoi();

			/*** RSSI ***/
			if (status & (uint16_t) RFM22B_Interrupt::RSSI) {
				if (verbose)
					cout << "<<<Squelch open>>>" << endl;
			}
			
			if (status & (uint16_t) RFM22B_Interrupt::VALID_PREAMBLE) {
				if (verbose)
					cout << "<<<Valid preamble>>>" << endl;
				clearRXFIFO();
				sync = false;
			}

			/*** SYNC_WORD ***/
			if (status & (uint16_t) RFM22B_Interrupt::SYNC_WORD) {
				if (verbose)
					cout << "<<<Sync>>>" << endl;
				alreadyreceived = 0;
				sync = true;
			}

			/*** VALID_PACKET_RECEIVED ***/
			if (status & (uint16_t) RFM22B_Interrupt::VALID_PACKET_RECEIVED) {
				if (verbose)
					cout << "<<<Valid packet received>>>" << endl;
				if (sync) {
					if (alreadyreceived == 0) {
						transfer(txb, rxb, 2);
						packagelength = rxb[1];
						if (debug)
							cout << "---First of ";
					} else {
						if (debug)
							cout << "---"
							     << alreadyreceived << " of ";
					}
					int received = packagelength - alreadyreceived;
					if (debug)
						cout << packagelength << ", got:"
						     << received << "---" << endl;
					if ((received >= 0) && 
						(alreadyreceived + received == packagelength) &&
						(received <= MAX_PACKET_LENGTH))
					{
						transfer(txb, rxb, received+1);
						if (debug) {
							cout << "---store data---" << endl;
							DaisyUtils::dump(cout, &rxb[1], received);
						}
						memcpy(&data[alreadyreceived], &rxb[1], received);
						alreadyreceived += received;
						++valids;
						octets += packagelength;
						if (verbose) {
							cout << "---Deliver---" << endl;
							DaisyUtils::dump(cout, data, packagelength);
						}
						if (input)
							input(data, packagelength);
						sync = false;
					} else {
						if (verbose)
							cout << "!!!Inconsistency"
							     << ":received=" << received
							     << ",alreadyreceived=" << alreadyreceived
							     << ",packagelength=" << packagelength
							     << "!!!>" << endl;
						clearRXFIFO();
						sync = false;
					}
				} else {
					if (debug)
						cout << "<##RX outside sync##>" << endl;
					clearRXFIFO();
				}
			}

			/*** RX_FIFO_ALMOST_FULL_INT ***/
			if (status & (uint16_t) RFM22B_Interrupt::RX_FIFO_ALMOST_FULL_INT) {
				if (debug)
					cout << "<<<RX FIFO almost full: ";
				transfer(txb, rxb, rxbffaful+1);
				if (sync) {
					int received, startindex;
					if (alreadyreceived > 0) {
						received = rxbffaful;
						startindex = 1;
						if (debug)
							cout << alreadyreceived << " of "
							     << packagelength << ", got:"
							     << received << ">>>" << endl;
					} else {
						packagelength = rxb[1];
						received = rxbffaful - 1;
						startindex = 2;
						if (debug)
							cout << "First of "
							     << packagelength << ", got:"
							     << received << ">>>" << endl;
					}
					if (alreadyreceived + received <= sizeof(data)) {
						if (debug) {
							cout << "---store data---" << endl;
							DaisyUtils::dump(cout, &rxb[startindex], received);
						}
						memcpy(&data[alreadyreceived], &rxb[startindex],
								received);
						alreadyreceived += received;
					} else {
						if (debug)
							cout << "<<<Too much data:"
								 << alreadyreceived + rxbffaful << endl;
						clearRXFIFO();
						sync = false;
					}
				} else {
					if (debug)
						cout << " outside sync>>>" << endl;
					clearRXFIFO();
				}
			}

			/*** CRC_ERROR ***/
			if (status & (uint16_t) RFM22B_Interrupt::CRC_ERROR) {
				if (verbose)
					cout << "<<<CRC error>>>" << endl;
				++crcerrors;
				sync = false;
			}

			/*** FIFO_UNDERFLOW_OVERFLOW ***/
			if (status & (uint16_t) RFM22B_Interrupt::FIFO_UNDERFLOW_OVERFLOW) {
				if (sync) {
					if (debug)
						cout << "<<<Over-/Underflow>>>" << endl;
					clearRXFIFO();
					uint8_t x = getRegister(RFM22B_Register::DEVICE_STATUS);
					if (x & 0x80) {
						if (verbose)
							cout << "<==Overflow==>" << endl;
						++overflows;
						sync = false;
					}
					if (x & 0x40) {
						if (verbose)
							cout << "<==Underflow==>" << endl;
						++underflows;
						sync = false;
					}
				}
			}

		} // end while //
		disableRXMode();
		setInterruptEnable(RFM22B_Interrupt::RSSI,                    false);
		setInterruptEnable(RFM22B_Interrupt::VALID_PREAMBLE,          false);
		setInterruptEnable(RFM22B_Interrupt::SYNC_WORD,               false);
		setInterruptEnable(RFM22B_Interrupt::CRC_ERROR,               false);
		setInterruptEnable(RFM22B_Interrupt::FIFO_UNDERFLOW_OVERFLOW, false);
		setInterruptEnable(RFM22B_Interrupt::VALID_PACKET_RECEIVED,   false);
		setInterruptEnable(RFM22B_Interrupt::RX_FIFO_ALMOST_FULL_INT, false);
		cout << octets << " octets"
			 << endl;
		cout << "  "
			 << valids     << " pkgs, "
			 << crcerrors  << " CRC err, "
			 << overflows  << " overflows, "
			 << underflows << " underflows"
			 << endl;
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

	void RFM22B::init(struct register_value rg_rv[]) {
		uint8_t buf[2];
		for (; rg_rv->setting[0] != 0x00; ++rg_rv)
			transfer(rg_rv->setting, buf, 2);
		sleep(1);
	}

} // end namespace //
