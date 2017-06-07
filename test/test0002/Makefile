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

include ../../environment.mk

# All Target
all:
	@echo 'Invoking: Module hello build'
ifeq ($(wildcard $(CONFIGURATION)),)
	-@$(MKDIR) $(CONFIGURATION)
endif
	$(CP) *.c $(CONFIGURATION)
	@echo 'obj-m := hello.o' > $(CONFIGURATION)/Makefile
	@echo 'Invoking: Cross GCC module build'
	$(MAKE) ARCH=arm CROSS_COMPILE=$(TOOLSET_PREFIX) \
		-C $(KERNEL_SRC) \
		M=${CURDIR}/$(CONFIGURATION) modules
	$(RM) $(CONFIGURATION)/*.c
	@echo 'Finished building module hello'
	-@echo ' '

clean:
ifneq ($(wildcard $(CONFIGURATION)),)
	$(MAKE) -C $(KERNEL_SRC) \
		M=${CURDIR}/$(CONFIGURATION) clean
	$(RMDIR) $(CONFIGURATION)
endif
	-@echo ' '

.PHONY: all clean
