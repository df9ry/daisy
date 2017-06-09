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

namespace RFM22B_NS {

int main() {
	RFM22B *myRadio = new RFM22B("/dev/spidev0.0");
	myRadio->setMaxSpeedHz(100000);
	
	printf("Initial Settings...\n");
	printf("\tFrequency is %.3fMHz\n",myRadio->getCarrierFrequency()/1E6f);
	printf("\tFH Step is %.3fkHz\n",myRadio->getFrequencyHoppingStepSize()/1E3f);
	printf("\tChannel is %d\n",myRadio->getChannel());
	printf("\tFrequency deviation is %.3fkHz\n",myRadio->getFrequencyDeviation()/1E3f);
	printf("\tData rate is %.3fkbps\n",myRadio->getDataRate()/1E3f);
	printf("\tModulation Type %d\n",myRadio->getModulationType());
	printf("\tModulation Data Source %d\n",myRadio->getModulationDataSource());
	printf("\tData Clock Configuration %d\n",myRadio->getDataClockConfiguration());
	printf("\tTransmission Power is %ddBm\n",myRadio->getTransmissionPower());
	printf("\tGPIO0 Function is 0x%.2X\n",myRadio->getGPIOFunction(GPIO0));
	printf("\tGPIO1 Function is 0x%.2X\n",myRadio->getGPIOFunction(GPIO1));
	printf("\tGPIO2 Function is 0x%.2X\n",myRadio->getGPIOFunction(GPIO2));
	
	myRadio->setCarrierFrequency(869E6);
	myRadio->setFrequencyHoppingStepSize(20000);
	myRadio->setChannel(3);
	myRadio->setFrequencyDeviation(62500);
	myRadio->setDataRate(15000);
	myRadio->setModulationType(GFSK);
	myRadio->setModulationDataSource(FIFO);
	myRadio->setDataClockConfiguration(NONE);
	myRadio->setTransmissionPower(11);
	myRadio->setGPIOFunction(GPIO0, TX_STATE);
	myRadio->setGPIOFunction(GPIO1, RX_STATE);
	myRadio->setGPIOFunction(GPIO2, CLEAR_CHANNEL_ASSESSMENT);
	
	printf("\nNew Settings...\n");
	printf("\tFrequency is %.3fMHz\n",myRadio->getCarrierFrequency()/1E6f);
	printf("\tFH Step is %.3fkHz\n",myRadio->getFrequencyHoppingStepSize()/1E3f);
	printf("\tChannel is %d\n",myRadio->getChannel());
	printf("\tFrequency deviation is %.3fkHz\n",myRadio->getFrequencyDeviation()/1E3f);
	printf("\tData rate is %.3fkbps\n",myRadio->getDataRate()/1E3f);
	printf("\tModulation Type %d\n",myRadio->getModulationType());
	printf("\tModulation Data Source %d\n",myRadio->getModulationDataSource());
	printf("\tData Clock Configuration %d\n",myRadio->getDataClockConfiguration());
	printf("\tTransmission Power is %ddBm\n",myRadio->getTransmissionPower());
	printf("\tGPIO0 Function is 0x%.2X\n",myRadio->getGPIOFunction(GPIO0));
	printf("\tGPIO1 Function is 0x%.2X\n",myRadio->getGPIOFunction(GPIO1));
	printf("\tGPIO2 Function is 0x%.2X\n",myRadio->getGPIOFunction(GPIO2));
	
	myRadio->close();
}

} // end namespace //
