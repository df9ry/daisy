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

CONFIGURATION  ?= Debug
TOOLSET_PREFIX ?= arm-linux-gnueabihf-

GCC            := $(TOOLSET_PREFIX)gcc

ifdef ComSpec
    RM    := del /f /q
    RMDIR := rmdir /s /q
    MKDIR := mkdir
else
    RM    := rm -f
    RMDIR := rm -rf
    MKDIR := mkdir
endif
