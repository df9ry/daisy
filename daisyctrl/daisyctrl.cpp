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
#include <functional>

#include "rfm22b.h"
#include "daisy_exception.h"
#include "defaults.h"
#include "utility.h"

using namespace std;
using namespace RFM22B_NS;
using namespace DaisyUtils;

static bool f_verbose = true;

typedef function<void(RFM22B&, const string&)>
	command_handler_type;

struct command {
	const string          operand;
	const string          description;
	command_handler_type  handler;
};

typedef map<const string, command> command_map_type;
typedef command_map_type::const_iterator     command_map_iter;

static const command_map_type command_map = {
	{ "verbose=", command {
	  "<on|off>",  "Set verbose flag",
			[](RFM22B& chip, const string& arg)
			{ f_verbose = decode_bool(arg); }}},
		{ "verbose?", command {
		  "",  "Get verbose flag",
				[](RFM22B& chip, const string& arg)
				{ noarg(arg);
				  cout << "verbose=" << print(f_verbose) << endl; }}},
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
	  "<n>", "Set frequency in Hz",
			[](RFM22B& chip, const string& arg)
			{ chip.setCarrierFrequency(decode_uint(arg)); }}},
	{ "qrg?", command {
	  "", "Get frequency in Hz",
			[](RFM22B& chip, const string& arg)
			{ noarg(arg);
			  cout << "qrg=" << print(chip.getCarrierFrequency()) << endl; }}},
	{ "tune=", command {
	  "<d>", "Send unmodulated carrier for d milliseconds",
			[](RFM22B& chip, const string& arg)
			{ chip.tune(decode_uint(arg)); }}},
	{ "tune", command {
	  "", "Send unmodulated carrier for 10 seconds",
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
};

static void help(ostream &os, bool f_quiet = false) {
	if (f_quiet)
		return;

	os << "Usage: daisyctrl [device=<device>]" << endl;

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

int main(int argc, char* argv[]) {

	try {
		if (argc == 1) {
			help(cout);
			return EXIT_SUCCESS;
		}
		RFM22B chip;
		for (int iarg = 1; iarg < argc; ++iarg) {
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
		help(cerr, !f_verbose);
	}
	catch (std::exception &ex) {
		cerr << "Error: " << ex.what() << endl;
	}
	catch (...) {
		cerr << "Unspecified exception." << endl;
	}
	return EXIT_FAILURE;
}
