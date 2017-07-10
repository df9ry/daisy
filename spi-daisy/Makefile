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

# All Target
all:
	@echo 'Invoking: Module spi-daisy build'
ifeq ($(wildcard $(CONFIGURATION)),)
	-@$(MKDIR) $(CONFIGURATION)
endif
	$(LN) ${CURDIR}/*.[hc] $(CONFIGURATION)/
	$(LN) ${CURDIR}/Makefile_Template $(CONFIGURATION)/Makefile
	@echo 'Invoking: Cross GCC module build'
	$(MAKE) CONFIG_DEBUG_SECTION_MISMATCH=y \
			ARCH=arm CROSS_COMPILE=$(TOOLSET_PREFIX) \
		-C $(KERNEL_SRC) \
		M=${CURDIR}/$(CONFIGURATION) modules
	@echo 'Finished building module spi-daisy'
	-@echo ' '

clean:
ifneq ($(wildcard $(CONFIGURATION)),)
	$(RMDIR) $(CONFIGURATION)
endif
	-@echo ' '

.PHONY: all clean
