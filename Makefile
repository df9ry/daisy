# Copyright 2017 Tania Hagn

# This file is part of Daisy.
# 
#     Daisy is free software: you can redistribute it and/or modify
#     it under the terms of the GNU General Public License as published by
#     the Free Software Foundation, either version 3 of the License, or
#     (at your option) any later version.
# 
#     Daisy is distributed in the hope that it will be useful,
#     but WITHOUT ANY WARRANTY; without even the implied warranty of
#     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#     GNU General Public License for more details.
# 
#     You should have received a copy of the GNU General Public License
#     along with Daisy.  If not, see <http://www.gnu.org/licenses/>.

.PHONY: all rfm22b driver hello snull rfm22b_tests tests clean kernel \
	kernel_clean

all: driver

rfm22b:
	$(MAKE) -C rfm22b all
	
driver:
	$(MAKE) -C driver all

hello:
	$(MAKE) -C test/test0001 all

snull:
	$(MAKE) -C test/test0003 all

rfm22b_tests:
	$(MAKE) -C test/test0004 all

tests:
	$(MAKE) -C test all

clean:
	$(MAKE) -C driver clean
	$(MAKE) -C test   clean

kernel:
	$(MAKE) -C kernel all
	
kernel_clean:
	$(MAKE) -C kernel clean

