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

#ifndef _SPI_DAISY_EXCEPTION_H
#define _SPI_DAISY_EXCEPTION_H

#include <exception>
#include <string>

class daisy_exception: public std::exception {
private:
	std::string msg;
public:
	daisy_exception(const std::string& _msg) {
		msg = _msg;
	}
	daisy_exception(const std::string& _msg, const std::string& _arg) {
		msg = _msg + ": " + _arg;
	}
	virtual const char* what() const throw() {
		return msg.c_str();
	}
};

#endif
