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

include ../environment.mk

# Value 'kernel'  for Pi 1, Pi 0, Pi 0 W or ComputeModule
# Value *kernel7' for Pi 2, Pi 3 or ComputeModule 3
KERNEL_SPEC ?= kernel

# Value 'bcmrpi_defconfig'     for Pi 1, Pi 0, Pi 0 W or ComputeModule
# Value *bcmrpi2709_defconfig' for Pi 2, Pi 3 or ComputeModule 3
DEF_CONFIG  ?= bcmrpi_defconfig

# All Target
all:
	@echo 'Building kernel'
	$(MAKE) ARCH=arm CROSS_COMPILE=$(TOOLSET_PREFIX) \
		-C $(KERNEL_SRC) $(DEF_CONFIG)
	$(MAKE) ARCH=arm CROSS_COMPILE=$(TOOLSET_PREFIX) \
		-C $(KERNEL_SRC) zImage modules dtbs
	@echo 'Finished building kernel'
	-@echo ' '
	
# Clean Target
clean:
	@echo 'Cleaning kernel'
	$(MAKE) M=$(PWD) -C $(KERNEL_SRC) clean
	@echo 'Finished cleaning kernel'
	-@echo ' '
	

.PHONY: all clean
