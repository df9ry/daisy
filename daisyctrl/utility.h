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


#ifndef UTILITY_H_
#define UTILITY_H_

#include <string>
#include <vector>

#include <stdint.h>

namespace DaisyUtils {

	std::string          decode_string(const std::string&          s);
	unsigned int         decode_uint  (const std::string&          s);
	std::vector<uint8_t> decode_call  (const std::string&          s);
	bool                 decode_bool  (const std::string&          s);

	void                 noarg        (const std::string&          s);

	std::string          print        (const std::string&          v);
	std::string          print        (unsigned char               v);
	std::string          print        (unsigned short              v);
	std::string          print        (unsigned int                v);
	std::string          print        (signed char                 v);
	std::string          print        (signed short                v);
	std::string          print        (signed int                  v);
	std::string          print_call   (const std::vector<uint8_t>& v);
	std::string          print        (bool                        v);
	std::string          print        (const uint8_t* pb, size_t cb );

} // end namespace //

#endif /* UTILITY_H_ */
