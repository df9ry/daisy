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
	driver spi-daisy spi16 rfm22b hello hellodrv spitest \
	snull daisy_gui c2e tests test0004 clean kernel daisy kernel_clean

all: spi-daisy driver

daisy:
	$(MAKE) -C daisy all
	
spitest:
	$(MAKE) -C spitest all
	
driver:
	$(MAKE) -C driver all

rfm22b:
	$(MAKE) -C rfm22b all

spi-daisy:
	$(MAKE) -C spi-daisy all

spi16:
	$(MAKE) -C spi16 all

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
	$(MAKE) -C spitest    clean
	$(MAKE) -C spi-daisy  clean
	$(MAKE) -C spi16      clean
	$(MAKE) -C rfm22b     clean
	$(MAKE) -C daisy      clean
	$(MAKE) -C daisy_gui  clean
	$(MAKE) -C c2e        clean
	$(MAKE) -C test       clean

kernel:
	$(MAKE) -C kernel all
	
kernel_clean:
	$(MAKE) -C kernel clean

