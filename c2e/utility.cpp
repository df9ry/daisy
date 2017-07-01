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

#include <string>
#include <algorithm>
#include <exception>
#include <cerrno>
#include <cstring>

#include <fcntl.h>

#include <net/if.h>
#include <net/if_arp.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include "utility.h"

using namespace std;

namespace Utility {

	string toupper(const string& s) {
		string _s{s};
		transform(_s.begin(), _s.end(), _s.begin(), ::toupper);
		return _s;
	}

	string tolower(const string& s) {
		string _s{s};
		transform(_s.begin(), _s.end(), _s.begin(), ::tolower);
		return _s;
	}
	
	void setaddr(const string& ifname, const vector<uint8_t>& addr) {
		if (addr.size() != IFHWADDRLEN)
			throw logic_error("Invalid ethernet address size (max " +
					to_string(IFHWADDRLEN) + ")");
		
		if (ifname.length() >= IFNAMSIZ)
			throw logic_error("Interface name too long: (max " +
					to_string(IFNAMSIZ-1) + ")");
		struct ifreq ifr;
		strncpy(ifr.ifr_name, ifname.c_str(), IFNAMSIZ);
		
		ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
		memcpy(ifr.ifr_hwaddr.sa_data, &addr[0], IFHWADDRLEN);
		int skfd = socket(AF_LOCAL, SOCK_DGRAM, 0);
		if (skfd < 0)
			throw logic_error("Unable to open socket");
		if (ioctl(skfd, SIOCSIFHWADDR, &ifr) < 0) {
			if (errno == EBUSY) {
				throw logic_error(
						"Device is UP. You may down the interface before!");
			} else {
				throw logic_error("ioctl error: " + string(strerror(errno)));
			}
		}
	}

	vector<uint8_t> getaddr(const string& ifname) {
		int skfd = socket(AF_LOCAL, SOCK_DGRAM, 0);
		if (skfd < 0)
			throw logic_error("Unable to open socket");
		struct ifreq ifr;
		memset(&ifr, 0x00, sizeof(ifr));
		strncpy(ifr.ifr_name, ifname.c_str(), IFNAMSIZ);
		ioctl(skfd, SIOCGIFHWADDR, &ifr);
		vector<uint8_t> result(IFHWADDRLEN);
		memcpy(&result[0], ifr.ifr_hwaddr.sa_data, IFHWADDRLEN);
		return result;
	}

} // end namespace //
