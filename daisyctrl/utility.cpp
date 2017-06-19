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
#include <sstream>
#include <iostream>
#include <cctype>
#include <climits>
#include <algorithm>

#include "utility.h"
#include "daisy_exception.h"

using namespace std;

namespace DaisyUtils {

	static string _decode_string(const string& s) {
		ostringstream oss;
		bool esc = false;
		for (string::const_iterator iter = s.begin();
				iter != s.end(); ++iter)
		{
			if (esc) {
				switch (*iter) {
				case 'b':
					oss << '\b';
					break;
				case 't':
					oss << '\t';
					break;
				case 'n':
					oss << '\n';
					break;
				case 'r':
					oss << '\r';
					break;
				default:
					oss << *iter;
					break;
				} // end switch //
				esc = false;
			} else if (*iter == '\\') {
				esc = true;
			} else {
				oss << *iter;
			}
		} // end for //
		if (esc)
			throw daisy_exception("Invalid string", s);
		return oss.str();
	}

	string decode_string(const string& s) {
		if ((s.length() < 2) || (s.find('"') == s.npos))
			return s;
		if (s[0] != '"')
			return _decode_string(s);
		if (s[s.length()-1] != '"')
			throw daisy_exception("Invalid string", s);
		return _decode_string(s.substr(1, s.length()-1));
	}

	static unsigned long long _decode_ulong(const std::string& s) {
		unsigned long long v = 0;
		for (string::const_iterator iter = s.begin();
				iter != s.end(); ++iter)
		{
			if (isdigit(*iter))
				v = 10 * v + (*iter - '0');
			else if (*iter != '_')
				throw daisy_exception("Invalid number value", s);
		} // end for //
		return v;
	}

	unsigned int decode_uint(const std::string& s) {
		unsigned long long l = _decode_ulong(s);
		if (l > UINT_MAX)
			throw daisy_exception("Value too large", s);
		return (unsigned int) l;
	}

	std::vector<uint8_t> decode_call(const std::string& s) {
		unsigned long long ssid = 0;
		std::string _s = s;
		std::transform(_s.begin(), _s.end(), _s.begin(), ::toupper);
		size_t pos = _s.find_last_of('-');
		if (pos != string::npos) {
			ssid = _decode_ulong(_s.substr(pos+1));
			if (ssid > 255)
				throw daisy_exception("SSID too large (0..255)", s);
			_s = _s.substr(0, pos);
		}
		if (_s.length() > 6)
			throw daisy_exception("Callsign too long (max. 6)", s);
		std::vector<uint8_t> result(6);
		for (int i = 0; i < 6; ++i) {
			uint8_t x;
			if (i < _s.length()) {
				x = (uint8_t)_s[i];
				if ((x < 0x20) || (x >= 0x60))
					throw daisy_exception(
							"Callsign contains invalid characters ", s);
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

	bool decode_bool(const std::string& s) {
		if ((s == "true") || (s == "on") || (s == "1"))
			return true;
		if ((s == "false") || (s == "off") || (s == "0"))
			return false;
		throw daisy_exception("Invalid operand", s);
	}

	string print(const std::string& s) {
		ostringstream oss;
		oss << '"';
		for (string::const_iterator iter = s.begin();
				iter != s.end(); ++iter)
		{
			switch (*iter) {
			case '"' :
				oss << "\\\"";
				break;
			case '\b' :
				oss << "\\b";
				break;
			case '\t' :
				oss << "\\t";
				break;
			case '\n' :
				oss << "\\n";
				break;
			case '\r' :
				oss << "\\r";
				break;
			default :
				oss << *iter;
				break;
			} // end switch //
			oss << '"';
		} // end for //
		return oss.str();
	}

	string print(unsigned int v) {
		return to_string(v);
	}

	string print(unsigned short v) {
		return to_string(v);
	}

	string print(unsigned char v) {
		return to_string(v);
	}

	string print(signed int v) {
		return to_string(v);
	}

	string print(signed short v) {
		return to_string(v);
	}

	string print(signed char v) {
		return to_string(v);
	}

	static const char hex[17] = "0123456789abcdef";

	static string _print_hex(const std::vector<uint8_t>& v) {
		ostringstream oss;
		bool first = true;
		for (vector<uint8_t>::const_iterator iter = v.begin();
				iter != v.end(); ++iter)
		{
			if (first)
				first = false;
			else
				oss << ':';
			oss << hex[(*iter) / 16] << hex[(*iter) % 16];
		}
		return oss.str();
	}

	string print_call(const std::vector<uint8_t>& v) {
		ostringstream oss;
		if (v.size() == 6) {
			for (int i = 0; i < 6; ++i)
			{
				char ch = ((( v[i] & 0xfc ) >> 2 )+ 0x20 );
				if (ch != ' ')
					oss << ch;
			}
			int ssid = (( v[5] & 0x03 ) << 6 ) |
					   (( v[4] & 0x03 ) << 4 ) |
					   (( v[3] & 0x03 ) << 2 ) |
					   (( v[2] & 0x03 ) << 0 );
			oss << "-" << ssid;
			if (v[0] & 0x01)
				oss << " (broadcast)";
			oss << " ";
		}
		oss << "[" <<_print_hex(v) << "]";
		return oss.str();
	}

	string print(bool v) {
		return (v?"on":"off");
	}

	string print(const uint8_t* pb, size_t cb ) {
		if (!pb)
			return "NULL";
		ostringstream oss;
		oss << "[";
		bool first = true;
		while (cb > 0)
		{
			if (first)
				first = false;
			else
				oss << ':';
			oss << hex[(*pb) / 16] << hex[(*pb) % 16];
			++pb; --cb;
		}
		oss << "]";
		return oss.str();
	}

	void noarg(const std::string& s) {
		if (s.length() > 0)
			throw daisy_exception("Misplaced argument", s);
	}

} // end namespace //
