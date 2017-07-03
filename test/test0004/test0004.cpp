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

/*
 * Check connectivity to the RFM22B SPI driver.
 */

#include <cstdlib>
#include <iostream>
#include <exception>
#include <stdexcept>

using namespace std;

int main()
{
	cout << "Test0004" << endl;
	
	try {
		
	}
	catch (exception& ex) {
		cerr << "Error: " << ex.what() << endl;
	}
	catch (...) {
		cerr << "Error: Unspecified" << endl;
	}
	
	return(EXIT_SUCCESS);
}
