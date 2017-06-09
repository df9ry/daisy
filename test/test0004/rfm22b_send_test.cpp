/* Copyright (c) 2013 Owen McAree
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* This is a simple test program for the RFM22B class
 *	Currently all it can do it get and set the carrier frequency of the module
 *	as this is all the functionality available from the class. More coming soon...
 */
 
#include "rfm22b.h"

using namespace RFM22B_NS;

int main() {
	// Initialise the radio
	RFM22B *myRadio = new RFM22B("/dev/spidev0.0");
	
	// Set the bus speed
	myRadio->setMaxSpeedHz(1000000);
	
	// Radio configuration
	myRadio->reset();
	myRadio->setCarrierFrequency(434.5E6);
	myRadio->setModulationType(GFSK);
	myRadio->setModulationDataSource(FIFO);
	myRadio->setDataClockConfiguration(NONE);
	myRadio->setTransmissionPower(5);
	myRadio->setGPIOFunction(GPIO0, TX_STATE);
	myRadio->setGPIOFunction(GPIO1, RX_STATE);
	
	// What header are we broadcasting
	myRadio->setTransmitHeader(123456789);
	
	char output[RFM22B::MAX_PACKET_LENGTH] = "Hello World!";
	printf("Sending '%s'\n", output);
	myRadio->send((uint8_t*)output, RFM22B::MAX_PACKET_LENGTH);	
	myRadio->close();
}
