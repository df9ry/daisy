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
#include <string>
#include <cstring>
#include <vector>
#include <stdexcept>

#include <net/if.h>

#include "utility.h"

using namespace std;
using namespace Utility;

static void help(ostream &os) {
	os << "Usage: c2e" << endl;
	os << "\tdev=<dsyN> ...... // IO device - Default: write stdout" << endl;
	os << "\tcall=<callsign> . // Callsign  - Default: read stdin"   << endl;
	os << "\thelp ............ // Print help text"                   << endl;
}

static vector<uint8_t> decode_call(const string& s) {
	string _s{toupper(s)};
	
	unsigned long long ssid = 0;
	size_t pos = _s.find_last_of('-');
	if (pos != string::npos) {
		ssid = stoull(_s.substr(pos+1));
		if (ssid > 255)
			throw out_of_range("SSID too large (0..255)");
		_s = _s.substr(0, pos);
	}
	if (_s.length() > IFHWADDRLEN)
		throw invalid_argument("Callsign too long (max. 6)");
	
	vector<uint8_t> result(IFHWADDRLEN);
	for (int i = 0; i < IFHWADDRLEN; ++i) {
		uint8_t x;
		if (i < _s.length()) {
			x = (uint8_t)_s[i];
			if ((x < 0x20) || (x >= 0x60))
				throw invalid_argument(
						"Callsign contains invalid characters");
		} else {
			x = 0x20;
		}
		result[i] = ( x - 0x20 ) << 2;
	} // end for //
	result[2] |= (( ssid & 0x03 ) >> 0 );
	result[3] |= (( ssid & 0x0c ) >> 2 );
	result[4] |= (( ssid & 0x30 ) >> 4 );
	result[5] |= (( ssid & 0xc0 ) >> 6 );
	return result;
}

int main(int argc, char* argv[]) {
	try {
		vector<uint8_t> addr(0);
		string          dev;
		
		for (int iarg = 1; iarg < argc; ++iarg) {
			string arg(argv[iarg]);
			string cmd;
			
			{
				const char rgsep[] { '=' };
				size_t pos = arg.find_first_of(rgsep, 0, sizeof(rgsep));
				if (pos == string::npos) {
					cmd = tolower(arg);
					arg = "";
				} else {
					cmd = tolower(arg.substr(0, pos));
					arg = arg.substr(pos+1);
				}
			}
			
			if ((cmd == "help") && (iarg == 1)) {
				help(cout);
				return EXIT_SUCCESS;
			} else if ((cmd == "call") && (addr.size() == 0)) {
				addr = decode_call(arg);
			} else if ((cmd == "dev") && (dev.length() == 0) && 
					(arg.length() > 0))
			{
				dev = arg;
			} else {
				help(cerr);
				return EXIT_FAILURE;
			}
		} // end for //
		
		if (addr.size() == 0) {
			string call;
			cin >> call;
			addr = decode_call(call);
		}
		
		if (dev.length() > 0) {
			setaddr(dev, addr);
		} else {
			const char hex[] = "0123456789abcdef";
			bool first = true;
			for (int i = 0; i < IFHWADDRLEN; ++i) {
				if (first) first = false; else cout << ':';
				uint8_t x = addr[i];
				cout << hex[x/16] << hex[x%16];
			} // end for //
			cout.flush();
		}
		return EXIT_SUCCESS;
	}
	catch (exception &ex) {
		cerr << "Error: " << ex.what() << endl;
	}
	catch (...) {
		cerr << "Error: Unspecified exception." << endl;
	}
	return EXIT_FAILURE;
}
