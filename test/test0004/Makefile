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
all: $(CONFIGURATION)/test0004

# Tool invocations
$(CONFIGURATION)/test0004: *.cpp
	@echo 'Building target: $@'
ifeq ($(wildcard $(CONFIGURATION)),)
	-@$(MKDIR) $(CONFIGURATION)
endif
	@echo 'Invoking: Cross GCC Compiler and Linker'
	$(GXX) -std=c++11 -o $@ test0004.cpp
	@echo 'Finished building target: $@'
	-@echo ' '

# Other Targets
clean:
ifneq ($(wildcard $(CONFIGURATION)),)
	$(RMDIR) $(CONFIGURATION)
endif
	-@echo ' '

.PHONY: all clean
