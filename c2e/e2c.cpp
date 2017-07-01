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
	os << "Usage: e2c" << endl;
	os << "\tmac=<MAC> ... // MAC to decode"        << endl;
	os << "\tdev=<dsyN> .. // Read MAC from device" << endl;
	os << "\thelp ........ // Print help text"      << endl;
}

static uint8_t hex(char ch) {
	if ((ch >= '0') && (ch <= '9'))
		return ch - '0';
	if ((ch >= 'a') && (ch <= 'f'))
		return ch - 'a' + 10;
	if ((ch >= 'A') && (ch <= 'F'))
		return ch - 'A' + 10;
	throw logic_error("Invalid MAC address");
}

static vector<uint8_t> decode_mac(const string& addr) {
	if (addr.length() != 3 * IFHWADDRLEN - 1)
		throw logic_error("Invalid MAC address");		
	vector<uint8_t> result(IFHWADDRLEN);
	int j = 0;
	for (int i = 0; i < IFHWADDRLEN; ++i, j+= 3) {
		if ((j > 0) && (addr[j-1] != ':'))
			throw logic_error("Invalid MAC address");		
		result[i] = hex(addr[j]) * 16 + hex(addr[j+1]);
	} // end for //
	return result;
}

int main(int argc, char* argv[]) {
	try {
		vector<uint8_t> mac(0);
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
			} else if ((cmd == "mac") && (mac.size() == 0) &&
					(dev.length() == 0))
			{
				mac = decode_mac(arg);
			} else if ((cmd == "dev") && (dev.length() == 0) &&
					(mac.size() == 0))
			{
				mac = getaddr(arg);
			} else {
				help(cerr);
				return EXIT_FAILURE;
			}
		} // end for //
		
		if (mac.size() == 0) {
			string addr;
			cin >> addr;
			mac = decode_mac(addr);
		}
		
		if (mac.size() != IFHWADDRLEN)
			throw logic_error("Invalid address lenght");
			
		for (int i = 0; i < IFHWADDRLEN; ++i)
		{
			char ch = ((( mac[i] & 0xfc ) >> 2 )+ 0x20 );
			if (ch != ' ')
				cout << ch;
		}
		int ssid = (( mac[5] & 0x03 ) << 6 ) |
				   (( mac[4] & 0x03 ) << 4 ) |
				   (( mac[3] & 0x03 ) << 2 ) |
				   (( mac[2] & 0x03 ) << 0 );
		if (ssid > 0)
			cout << "-" << ssid;
		if (mac[0] & 0x01)
			cout << " (broadcast)";
		cout << endl;
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
