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


#ifndef _UTILITY_H_
#define _UTILITY_H_

#include <string>
#include <vector>

namespace Utility {

	// Uppercase a string:
	std::string toupper(const std::string& s);

	// Lowercase a string:
	std::string tolower(const std::string& s);

	// Set ethernet address of interface:
	void setaddr(const std::string& ifname, const std::vector<uint8_t>& addr);

	// Get ethernet address of interface:
	std::vector<uint8_t> getaddr(const std::string& ifname);

} // end namespace //

#endif /* _UTILITY_H_ */
