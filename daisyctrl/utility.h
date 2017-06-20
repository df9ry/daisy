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

#include "rfm22b_types.h"

namespace DaisyUtils {

	std::string toupper(const std::string& s);
	std::string tolower(const std::string& s);
	std::vector<std::string> split(const std::string& s, char delim);
	std::string ltrim(const std::string& s);
	std::string rtrim(const std::string& s);
	std::string trim(const std::string& s);

	std::string          decode_string   (const std::string&          s);
	uint8_t              decode_uint8    (const std::string&          s);
	uint16_t             decode_uint16   (const std::string&          s);
	uint32_t             decode_uint32   (const std::string&          s);
	std::vector<uint8_t> decode_call     (const std::string&          s);
	bool                 decode_bool     (const std::string&          s);
	RFM22B_NS::RFM22B_Modulation_Type
	                     decode_modtype  (const std::string&          s);
	RFM22B_NS::RFM22B_Modulation_Data_Source
	                     decode_mds      (const std::string&          s);
	RFM22B_NS::RFM22B_Data_Clock_Configuration
	                     decode_dcc      (const std::string&          s);
	RFM22B_NS::RFM22B_GPIO_Function
	                     decode_gpiofunc (const std::string&          s);
	RFM22B_NS::RFM22B_Interrupt
	                     decode_interrupt(const std::string&          s);
	RFM22B_NS::RFM22B_Operating_Mode
	                     decode_opmodes  (const std::string&          s);
	RFM22B_NS::RFM22B_CRC_Mode
	                     decode_crcmode  (const std::string&          s);
	RFM22B_NS::RFM22B_CRC_Polynomial
	                     decode_crcpoly  (const std::string&          s);

	void                 noarg           (const std::string&          s);

	std::string          print           (const std::string&          v);
	std::string          print           (uint8_t                     v);
	std::string          print           (uint16_t                    v);
	std::string          print           (uint32_t                    v);
	std::string          print           (int8_t                      v);
	std::string          print           (int16_t                     v);
	std::string          print           (int32_t                     v);
	std::string          print_call      (const std::vector<uint8_t>& v);
	std::string          print           (bool                        v);
	std::string          print           (const uint8_t* pb, size_t cb );
	std::string          print           (
			RFM22B_NS::RFM22B_Modulation_Type                         v);
	std::string          print           (
			RFM22B_NS::RFM22B_Modulation_Data_Source                  v);
	std::string          print           (
			RFM22B_NS::RFM22B_Data_Clock_Configuration                v);
	std::string          print           (
			RFM22B_NS::RFM22B_GPIO_Function                           v);
	std::string          print           (
			RFM22B_NS::RFM22B_Interrupt                               v);
	std::string          print           (
			RFM22B_NS::RFM22B_Operating_Mode                          v);
	std::string          print           (
			RFM22B_NS::RFM22B_CRC_Mode                                v);
	std::string          print           (
			RFM22B_NS::RFM22B_CRC_Polynomial                          v);

	std::string          print_help      (const std::string&        cmd);
} // end namespace //

#endif /* UTILITY_H_ */
