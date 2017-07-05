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

.PHONY: all \
	driver rfm22b-spi rfm22b-bus hello hellodrv snull daisy_gui c2e tests \
	test0004 clean kernel daisy kernel_clean

all: rfm22b-bus

daisy:
	$(MAKE) -C daisy all
	
driver:
	$(MAKE) -C driver all

rfm22b-spi:
	$(MAKE) -C rfm22b-spi all

rfm22b-bus:
	$(MAKE) -C rfm22b-bus all

hello:
	$(MAKE) -C test/test0001 all

hellodrv:
	$(MAKE) -C test/test0002 all

snull:
	$(MAKE) -C test/test0003 all

test0004:
	$(MAKE) -C test/test0004 all

daisy_gui:
	$(MAKE) -C daisy_gui all
	
c2e:
	$(MAKE) -C c2e all

tests:
	$(MAKE) -C test all

clean:
	$(MAKE) -C driver     clean
	$(MAKE) -C rfm22b-spi clean
	$(MAKE) -C rfm22b-dev clean
	$(MAKE) -C daisy      clean
	$(MAKE) -C daisy_gui  clean
	$(MAKE) -C c2e        clean
	$(MAKE) -C test       clean

kernel:
	$(MAKE) -C kernel all
	
kernel_clean:
	$(MAKE) -C kernel clean

