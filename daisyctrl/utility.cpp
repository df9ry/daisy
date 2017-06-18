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

#include "utility.h"
#include "daisy_exception.h"

using namespace std;

namespace DaisyUtils {

	static string _decode(const string& s) {
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
			return _decode(s);
		if (s[s.length()-1] != '"')
			throw daisy_exception("Invalid string", s);
		return _decode(s.substr(1, s.length()-1));
	}

	unsigned int decode_uint(const std::string& s) {
		return 0;
	}

	std::vector<uint8_t> decode_call(const std::string& s) {
		return std::vector<uint8_t>({ });
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

	string print_call(const std::vector<uint8_t>& v) {
		return "not implemented";
	}

	string print(bool v) {
		return (v?"on":"off");
	}

	void noarg(const std::string& s) {
		if (s.length() > 0)
			throw daisy_exception("Misplaced argument", s);
	}

} // end namespace //
