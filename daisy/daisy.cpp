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

#include <iostream>
#include <cstdlib>
#include <map>

#include <signal.h>
#include <execinfo.h>

#include "rfm22b.h"
#include "daisy_exception.h"
#include "defaults.h"
#include "utility.h"
#include "rfm22b_registers.h"

using namespace std;
using namespace RFM22B_NS;
using namespace DaisyUtils;

static void shell(RFM22B& chip);
static void help(ostream &os, bool f_quiet = false);

typedef function<void(RFM22B&, const string&)>
	command_handler_type;

struct command {
	const string          operand;
	const string          description;
	command_handler_type  handler;
};

typedef map<const string, command>       command_map_type;
typedef command_map_type::const_iterator command_map_iter;

static const command_map_type command_map = {
	{ "verbose=", command {
	  "<on|off>",  "Set verbose flag",
			[](RFM22B& chip, const string& arg)
			{ chip.setVerbose(decode_bool(arg)); }}},
		{ "verbose?", command {
		  "",  "Get verbose flag",
				[](RFM22B& chip, const string& arg)
				{ noarg(arg);
				  cout << "verbose=" << print(chip.getVerbose()) << endl; }}},
	{ "call=", command {
	  "<call>{-<SSID>}",  "Set callsign, optional with SSID",
			[](RFM22B& chip, const string& arg)
			{ chip.setAddress(decode_call(arg)); }}},
	{ "call?", command {
	  "", "Get callsign with SSID",
			[](RFM22B& chip, const string& arg)
			{ noarg(arg);
			  cout << "call=" << print_call(chip.getAddress()) << endl; }}},
	{ "qrg=", command {
	  "<number>", "Set frequency in Hz",
			[](RFM22B& chip, const string& arg)
			{ chip.setCarrierFrequency(decode_uint32(arg)); }}},
	{ "qrg?", command {
	  "", "Get frequency in Hz",
			[](RFM22B& chip, const string& arg)
			{ noarg(arg);
			  cout << "qrg=" << print(chip.getCarrierFrequency()) << endl; }}},
	{ "tune=", command {
	  "<number>", "Send  carrier for d seconds",
			[](RFM22B& chip, const string& arg)
			{ chip.tune(decode_uint32(arg)); }}},
	{ "tune", command {
	  "", "Send carrier for 10 seconds",
			[](RFM22B& chip, const string& arg)
			{ noarg(arg); chip.tune(DEFAULT_TUNE_TIME); }}},
	{ "rssi?", command {
	  "", "Get RSSI value",
			[](RFM22B& chip, const string& arg)
			{ noarg(arg);
			  cout << "rssi=" << print(chip.getRSSI()) << endl; }}},
	{ "ip?", command {
	  "", "Get input power",
			[](RFM22B& chip, const string& arg)
			{ noarg(arg);
			  cout << "ip=" << print(chip.getInputPower()) << endl; }}},
	{ "debug=", command {
	  "<on|off>", "Set debug chip IO",
			[](RFM22B& chip, const string& arg)
			{ chip.setDebug(decode_bool(arg)); }}},
	{ "debug?", command {
	  "", "Get debug chip IO",
			[](RFM22B& chip, const string& arg)
			{ noarg(arg);
			  cout << "debug=" << print(chip.getDebug()) << endl; }}},
	{ "reset", command {
	  "", "Reset the chip",
			[](RFM22B& chip, const string& arg)
			{ noarg(arg); chip.reset(); }}},
	{ "channel=", command {
	  "<0..255>", "Set channel",
			[](RFM22B& chip, const string& arg)
			{ chip.setChannel(decode_uint8(arg)); }}},
	{ "channel?", command {
	  "", "Get debug chip IO",
			[](RFM22B& chip, const string& arg)
			{ noarg(arg);
			  cout << "channel=" << print(chip.getChannel()) << endl; }}},
	{ "deviation=", command {
	  "<number>", "Set frequency deviation",
			[](RFM22B& chip, const string& arg)
			{ chip.setFrequencyDeviation(decode_uint32(arg)); }}},
	{ "deviation?", command {
	  "", "Get frequency deviation",
			[](RFM22B& chip, const string& arg)
			{ noarg(arg);
			  cout << "deviation=" << print(chip.getFrequencyDeviation()) << endl; }}},
	{ "datarate=", command {
	  "<number>", "Set data rate",
			[](RFM22B& chip, const string& arg)
			{ chip.setDataRate(decode_uint32(arg)); }}},
	{ "datarate?", command {
	  "", "Get data rate",
			[](RFM22B& chip, const string& arg)
			{ noarg(arg);
			  cout << "datarate=" << print(chip.getDataRate()) << endl; }}},
	{ "modtype=", command {
	  "<mod. type>", "Set modulation type",
			[](RFM22B& chip, const string& arg)
			{ chip.setModulationType(decode_modtype(arg)); }}},
	{ "modtype?", command {
	  "", "Get modulation type",
			[](RFM22B& chip, const string& arg)
			{ noarg(arg);
			  cout << "modtype=" << print(chip.getModulationType()) << endl; }}},
	{ "mds=", command {
	  "<mod. data src>", "Set modulation data source",
			[](RFM22B& chip, const string& arg)
			{ chip.setModulationDataSource(decode_mds(arg)); }}},
	{ "mds?", command {
	  "", "Get modulation data source",
			[](RFM22B& chip, const string& arg)
			{ noarg(arg);
			  cout << "mds=" << print(chip.getModulationDataSource()) << endl; }}},
	{ "dcc=", command {
	  "<data clk conf>", "Set data clock configuration",
			[](RFM22B& chip, const string& arg)
			{ chip.setDataClockConfiguration(decode_dcc(arg)); }}},
	{ "dcc?", command {
	  "", "Get data clock configuration",
			[](RFM22B& chip, const string& arg)
			{ noarg(arg);
			  cout << "dcc=" << print(chip.getDataClockConfiguration()) << endl; }}},
	{ "txpower=", command {
	  "<0..255>", "Set transmit power",
			[](RFM22B& chip, const string& arg)
			{ chip.setTransmissionPower(decode_uint8(arg)); }}},
	{ "txpower?", command {
	  "", "Get transmit power",
			[](RFM22B& chip, const string& arg)
			{ noarg(arg);
			  cout << "txpower=" << print(chip.getTransmissionPower()) << endl; }}},
	{ "gpio0func=", command {
	  "<GPIO func.>", "Set GPIO0 function",
			[](RFM22B& chip, const string& arg)
			{ chip.setGPIOFunction(
					RFM22B_GPIO::GPIO0, decode_gpiofunc(arg)); }}},
	{ "gpio0func?", command {
	  "", "Get GPIO0 function",
			[](RFM22B& chip, const string& arg)
			{ noarg(arg);
			  cout << "gpio0func=" <<
					  print(chip.getGPIOFunction(RFM22B_GPIO::GPIO0)) << endl; }}},
	{ "gpio1func=", command {
	  "<GPIO func.>", "Set GPIO1 function",
			[](RFM22B& chip, const string& arg)
			{ chip.setGPIOFunction(
					RFM22B_GPIO::GPIO1, decode_gpiofunc(arg)); }}},
	{ "gpio1func?", command {
	  "", "Get GPIO1 function",
			[](RFM22B& chip, const string& arg)
			{ noarg(arg);
			  cout << "gpio1func=" <<
					  print(chip.getGPIOFunction(RFM22B_GPIO::GPIO1)) << endl; }}},
	{ "gpio2func=", command {
	  "<GPIO func.>", "Set GPIO2 function",
			[](RFM22B& chip, const string& arg)
			{ chip.setGPIOFunction(
					RFM22B_GPIO::GPIO2, decode_gpiofunc(arg)); }}},
	{ "gpio2func?", command {
	  "", "Get GPIO2 function",
			[](RFM22B& chip, const string& arg)
			{ noarg(arg);
			  cout << "gpio2func=" <<
					  print(chip.getGPIOFunction(RFM22B_GPIO::GPIO2)) << endl; }}},
	{ "inte=", command {
	  "<Interrupt>", "Enable interrupt",
			[](RFM22B& chip, const string& arg)
			{ chip.setInterruptEnable(decode_interrupt(arg), true); }}},
	{ "intd=", command {
	  "<Interrupt>", "Disable interrupt",
			[](RFM22B& chip, const string& arg)
			{ chip.setInterruptEnable(decode_interrupt(arg), false); }}},
	{ "ints=", command {
	  "<Interrupt>", "Get interrupt status",
			[](RFM22B& chip, const string& arg)
			{ cout << arg << " " <<	print(chip.getInterruptStatus(
							decode_interrupt(arg))); }}},
	{ "opmodes=", command {
	  "<OP modes>", "Set operating modes",
			[](RFM22B& chip, const string& arg)
			{ chip.setOperatingMode(decode_opmodes(arg)); }}},
	{ "opmodes?", command {
	  "", "Set operating modes",
			[](RFM22B& chip, const string& arg)
			{ noarg(arg);
			  cout << "opmodes=" << print(chip.getOperatingMode()) << endl; }}},
	{ "rxe", command {
	  "", "Enable RX",
	  	  	[](RFM22B& chip, const string& arg)
	  	  	{ noarg(arg); chip.enableRXMode(); }}},
	{ "txe", command {
	  "", "Enable TX",
			[](RFM22B& chip, const string& arg)
			{ noarg(arg); chip.enableTXMode(); }}},
	{ "txhdr=", command {
	  "<uint32>", "Set transmit header",
			[](RFM22B& chip, const string& arg)
			{ chip.setTransmitHeader(decode_uint32(arg)); }}},
	{ "txhdr?", command {
	  "", "Get transmit header",
			[](RFM22B& chip, const string& arg)
			{ noarg(arg);
			  cout << "txhdr=" << print(chip.getTransmitHeader()) << endl; }}},
	{ "crcmode=", command {
	  "<CRC mode>", "Set CRC mode",
			[](RFM22B& chip, const string& arg)
			{ chip.setCRCMode(decode_crcmode(arg)); }}},
	{ "crcmode?", command {
	  "", "Get CRC mdode",
			[](RFM22B& chip, const string& arg)
			{ noarg(arg);
			  cout << "crcmode=" << print(chip.getCRCMode()) << endl; }}},
	{ "crcpoly=", command {
	  "<CRC poly>", "Set CRC polynomial",
			[](RFM22B& chip, const string& arg)
			{ chip.setCRCPolynomial(decode_crcpoly(arg)); }}},
	{ "crcpoly?", command {
	  "", "Get CRC polynomial",
			[](RFM22B& chip, const string& arg)
			{ noarg(arg);
			  cout << "crcpoly=" << print(chip.getCRCPolynomial()) << endl; }}},
	{ "txamft=", command {
	  "<0..255>", "Set TX almost full threshold",
			[](RFM22B& chip, const string& arg)
			{ chip.setTXFIFOAlmostFullThreshold(decode_uint8(arg)); }}},
	{ "txamft?", command {
	  "", "Get TX almost full threshold",
			[](RFM22B& chip, const string& arg)
			{ noarg(arg);
			  cout << "txamft=" << print(chip.getTXFIFOAlmostFullThreshold()) << endl; }}},
	{ "txamet=", command {
	  "<0..255>", "Set TX almost empty threshold",
			[](RFM22B& chip, const string& arg)
			{ chip.setTXFIFOAlmostEmptyThreshold(decode_uint8(arg)); }}},
	{ "txamet?", command {
	  "", "Get TX almost empty threshold",
			[](RFM22B& chip, const string& arg)
			{ noarg(arg);
			  cout << "txamet=" << print(chip.getTXFIFOAlmostEmptyThreshold()) << endl; }}},
	{ "rxamft=", command {
	  "<0..255>", "Set RX almost full threshold",
			[](RFM22B& chip, const string& arg)
			{ chip.setRXFIFOAlmostFullThreshold(decode_uint8(arg)); }}},
	{ "rxamft?", command {
	  "", "Get RX almost full threshold",
			[](RFM22B& chip, const string& arg)
			{ noarg(arg);
			  cout << "rxamft=" << print(chip.getRXFIFOAlmostFullThreshold()) << endl; }}},
	{ "rxlen?", command {
	  "", "Get RX package length",
			[](RFM22B& chip, const string& arg)
			{ noarg(arg);
			  cout << "rxlen=" << print(chip.getReceivedPacketLength()) << endl; }}},
	{ "txlen=", command {
	  "<0..255>", "Set TX package length",
			[](RFM22B& chip, const string& arg)
			{ chip.setTransmitPacketLength(decode_uint8(arg)); }}},
	{ "rxclr", command {
	  "", "Clear RX FIFO",
			[](RFM22B& chip, const string& arg)
			{ noarg(arg); chip.clearRXFIFO(); }}},
	{ "txclr", command {
	  "", "Clear TX FIFO",
			[](RFM22B& chip, const string& arg)
			{ noarg(arg); chip.clearTXFIFO(); }}},
	{ "help", command {
	  "", "Print help",
			[](RFM22B& chip, const string& arg)
			{ noarg(arg); help(cout, !chip.getVerbose()); }}},
	{ "help=", command {
	  "<command>", "Print the possible values of the command",
			[](RFM22B& chip, const string& arg)
			{ cout << print_help(arg) << endl; }}},
	{ "shell", command {
		"", "Start interactive shell",
			[](RFM22B& chip, const string& arg)
			{ noarg(arg); shell(chip); }}},
	{ "devicetype?", command {
		"", "Get device type",
		[](RFM22B& chip, const string& arg)
		{ noarg(arg);
		  cout << "devicetype=" << print(chip.getDeviceType()) << endl; }}},
	{ "deviceversion?", command {
		"", "Get device version",
		[](RFM22B& chip, const string& arg)
		{ noarg(arg);
		  cout << "deviceversion=" << print(chip.getDeviceVersion()) << endl; }}},
	{ "devicestatus?", command {
		"", "Get device status",
		[](RFM22B& chip, const string& arg)
		{ noarg(arg);
		  cout << "devicestatus=" << print_status(chip.getDeviceStatus()) << endl; }}},
	{ "send", command {
		"", "Send 100 packages",
		[](RFM22B& chip, const string& arg)
		{ noarg(arg); chip.tx_packages(DEFAULT_NUM_PACKAGE); }}},
	{ "send=", command {
		"<number>", "Send <number> packages",
		[](RFM22B& chip, const string& arg)
		{ chip.tx_packages(decode_uint32(arg)); }}},
	{ "modmodes=", command {
		"<list of modes>", "Set modulation modes",
		[](RFM22B& chip, const string& arg)
		{ chip.setModulationMode(decode_modmodes(arg)); }}},
	{ "modmodes?", command {
		"", "Get modulation modes",
		[](RFM22B& chip, const string& arg)
		{ noarg(arg);
		  cout << "modmodes=" << print(chip.getModulationMode()) << endl; }}},
	{ "bandwidth=", command {
		"<number>", "Set IF bandwidth in Hz",
		[](RFM22B& chip, const string& arg)
		{ chip.setBandwidth(decode_uint32(arg)); }}},
	{ "bandwidth?", command {
		"", "Get IF bandwidth in Hz",
		[](RFM22B& chip, const string& arg)
		{ noarg(arg);
		  cout << "bandwidth=" << print(chip.getBandwidth()) << endl; }}},
	{ "preamble=", command {
		"<number>", "Set length of preamble in bits",
		[](RFM22B& chip, const string& arg)
		{ chip.setPreambleLength(decode_uint32(arg)); }}},
	{ "preamble?", command {
		"", "Get preamble length in bits",
		[](RFM22B& chip, const string& arg)
		{ noarg(arg);
		  cout << "preamble=" << print(chip.getPreambleLength()) << endl; }}},
	{ "receive", command {
		"", "Receive for 30s",
		[](RFM22B& chip, const string& arg)
		{ noarg(arg);
		  chip.rx_packages(DEFAULT_RX_TIMEOUT); }}},
	{ "receive=", command {
		"<number>", "Receive for <number>s",
		[](RFM22B& chip, const string& arg)
		{ chip.rx_packages(decode_uint32(arg)); }}},
	{ "narrow", command {
		"", "Set narrow mode",
		[](RFM22B& chip, const string& arg)
		{ noarg(arg);
		  chip.setNarrowMode(); }}},
	{ "medium", command {
		"", "Set medium mode",
		[](RFM22B& chip, const string& arg)
		{ noarg(arg);
		  chip.setMediumMode(); }}},
	{ "wide", command {
		"", "Set wide mode",
		[](RFM22B& chip, const string& arg)
		{ noarg(arg);
		  chip.setWideMode(); }}},
	{ "squelch=", command {
		"<0..255>", "Set squelch level",
		[](RFM22B& chip, const string& arg)
		{ chip.setSquelch(decode_uint8(arg)); }}},
	{ "squelch?", command {
		"", "Get squelch level",
		[](RFM22B& chip, const string& arg)
		{ noarg(arg);
		  cout << "squelch=" << print(chip.getSquelch()) << endl; }}},
};

static void help(ostream &os, bool f_quiet) {
	if (f_quiet)
		return;

	os << "Usage: daisy <spifile> [options]" << endl
	   << "  options:" << endl;

	int maxd = 0, maxo = 0;
	for (command_map_iter iter = command_map.begin();
			iter != command_map.end(); ++iter)
	{
		if (iter->first.length() > maxd)
			maxd = iter->first.length();
		if (iter->first.length() > maxo)
			maxo = iter->second.operand.length();
	} // end for //

	for (command_map_iter iter = command_map.begin();
			iter != command_map.end(); ++iter)
	{
		os << "\t";
		for (int i = 0; i < maxd - iter->first.length(); ++i)
			os << " ";
		bool single = (
				(iter->first[iter->first.length()-1] == '=') ||
				(iter->first[iter->first.length()-1] == '?'));
		if (single)
			os << ' ';
		os << iter->first << iter->second.operand;
		for (int i = 0; i < maxo - iter->second.operand.length(); ++i)
			os << " ";
		if (!single)
			os << " ";
		os << " -- " << iter->second.description << endl;
	} // end for //
}

static void shell(RFM22B& chip) {
	cout << "Daisy Shell started. End with \"quit\"" << endl;
	while (true) {
		cout << "daisy> ";
		cout.flush();
		string line;
		getline(cin, line);
		line = DaisyUtils::trim(line);
		if (line.length() == 0)
			continue;
		if (line == "quit")
			break;
		string cmd, arg;
		const char rgsep[] { '?', '=' };
		size_t pos = line.find_first_of(rgsep, 0, sizeof(rgsep));
		if (pos == string::npos) {
			cmd = line;
			arg = "";
		} else {
			cmd = line.substr(0, pos+1);
			arg = DaisyUtils::decode_string(line.substr(pos+1));
		}
		// Process command:
		command_map_iter iter = command_map.find(cmd);
		if (iter == command_map.end()) {
			cout << "Command not found: \"" << cmd << "\"" << endl;
			continue;
		}
		try {
			iter->second.handler(chip, arg);
		}
		catch (::daisy_exception &ex) {
			cerr << "Error: " << ex.what() << endl;
			help(cerr, false);
		}
		catch (std::exception &ex) {
			cerr << "Error: " << ex.what() << endl;
		}
		catch (...) {
			cerr << "Error: Unspecified exception." << endl;
		}
	} // end while //
	cout << "Daisy Shell exit" << endl;
}

static inline void stacktrace() {
	static const int   SIZE = 100;
	void              *buffer[SIZE];
	char             **strings;
	int                j, nptrs;
	
	nptrs = backtrace(buffer, SIZE);
	printf("backtrace() returned %d addresses\n", nptrs);
	strings = backtrace_symbols(buffer, nptrs);
	if (strings == NULL) {
		perror("backtrace_symbols");
		exit(EXIT_FAILURE);
	}
	for (j = 0; j < nptrs; j++)
		printf("%s\n", strings[j]);
	exit(EXIT_FAILURE);
}

static void handle_signal(int signal) {
    const char *signal_name;
    sigset_t pending;

    switch (signal) {
        case SIGHUP:
            printf("Caught SIGHUP, exiting now\n");
            exit(0);
            break;
        case SIGUSR1:
            printf("\nCaught SIGUSR1, exiting now\n");
            exit(0);
            break;
        case SIGINT:
            printf("\nCaught SIGINT, exiting now\n");
            exit(0);
            break;
        case SIGTERM:
            printf("\nCaught SIGTERM, exiting now\n");
            exit(0);
            break;
        case SIGSEGV:
            printf("\nCaught SIGSEGV, stacktrace\n");
            stacktrace();
            exit(0);
            break;
        default:
            fprintf(stderr, "\nCaught signal: %d\n", signal);
            return;
    } // end switch //
}

int main(int argc, char* argv[]) {
	struct sigaction sa;

	sa.sa_handler = &handle_signal;
	sa.sa_flags = SA_RESTART;
	sigfillset(&sa.sa_mask);
	if (sigaction(SIGHUP, &sa, NULL) == -1)
		perror("Error: cannot handle SIGHUP");
    if (sigaction(SIGUSR1, &sa, NULL) == -1)
        perror("Error: cannot handle SIGUSR1");
    if (sigaction(SIGINT, &sa, NULL) == -1)
        perror("Error: cannot handle SIGINT");
    if (sigaction(SIGTERM, &sa, NULL) == -1)
        perror("Error: cannot handle SIGTERM");
    if (sigaction(SIGSEGV, &sa, NULL) == -1)
        perror("Error: cannot handle SIGSEGV");

	try {
		if (argc == 1) {
			help(cout);
			return EXIT_SUCCESS;
		}
		RFM22B chip;
		if (!chip.open(argv[1]))
			throw daisy_exception(
					"Unable to open file \"" + string(argv[1]) + "\"");

		for (int iarg = 2; iarg < argc; ++iarg) {
			string arg(argv[iarg]);
			string cmd;
			const char rgsep[] { '?', '=' };
			size_t pos = arg.find_first_of(rgsep, 0, sizeof(rgsep));
			if (pos == string::npos) {
				cmd = arg;
				arg = "";
			} else {
				cmd = arg.substr(0, pos+1);
				arg = DaisyUtils::decode_string(arg.substr(pos+1));
			}
			// Process command:
			command_map_iter iter = command_map.find(cmd);
			if (iter == command_map.end())
				throw daisy_exception("Command not found", cmd);

			iter->second.handler(chip, arg);
		} // end for //
		return EXIT_SUCCESS;
	}
	catch (::daisy_exception &ex) {
		cerr << "Error: " << ex.what() << endl;
	}
	catch (std::exception &ex) {
		cerr << "Error: " << ex.what() << endl;
	}
	catch (...) {
		cerr << "Error: Unspecified exception." << endl;
	}
	return EXIT_FAILURE;
}

