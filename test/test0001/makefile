# Copyright 2016 Tania Hagn

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

ifdef ComSpec
    RM := del /F /Q
else
    RM := rm -f
endif

# All Target
all: $(TARGET_LOC)/test0001

# Tool invocations
$(TARGET_LOC)/test0001:
	@echo 'Building target: $@'
	@echo 'Invoking: Cross GCC Linker'
	arm-linux-gnueabihf-gcc -o $@ test0001.c
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) "$(TARGET_LOC)/test0001"
	-@echo ' '

.PHONY: all clean
