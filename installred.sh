#!/bin/bash

scp ./driver/Debug/daisy.ko        pi@raspberryred:~
scp ./spi-daisy/Debug/spi-daisy.ko pi@raspberryred:~
scp ./spi-daisy/*.sh               pi@raspberryred:~
