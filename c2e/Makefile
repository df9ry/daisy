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
all: makedir $(CONFIGURATION)/c2e $(CONFIGURATION)/e2c 
	
makedir:
ifeq ($(wildcard $(CONFIGURATION)),)
	-@$(MKDIR) $(CONFIGURATION)
endif

# Tool invocations

$(CONFIGURATION)/c2e: c2e.cpp \
	$(CONFIGURATION)/utility.o
	
	@echo 'Building target: $@'
	@echo 'Invoking: Cross GCC Compiler and Linker'
	$(GXX) -std=c++11 -o $@ $(basename $(notdir $@)).cpp \
		$(CONFIGURATION)/utility.o
	@echo 'Finished building target: $@'
	-@echo ' '

$(CONFIGURATION)/e2c: e2c.cpp \
	$(CONFIGURATION)/utility.o
	
	@echo 'Building target: $@'
	@echo 'Invoking: Cross GCC Compiler and Linker'
	$(GXX) -std=c++11 -o $@ $(basename $(notdir $@)).cpp \
		$(CONFIGURATION)/utility.o
	@echo 'Finished building target: $@'
	-@echo ' '

$(CONFIGURATION)/%.o: %.cpp *.h
	@echo 'Building target: $@'
	@echo 'Invoking: Cross GCC Compiler and Linker'
	$(GXX) -std=c++11 -c -o $@ $(basename $(notdir $@)).cpp
	@echo 'Finished building target: $@'
	-@echo ' '

# Other Targets
clean:
ifneq ($(wildcard $(CONFIGURATION)),)
	$(RMDIR) $(CONFIGURATION)
endif
	-@echo ' '

.PHONY: all clean makedir
