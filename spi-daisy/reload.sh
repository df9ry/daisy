#!/bin/bash

sudo rmmod spi-bcm2835 2>/dev/null
sudo rmmod daisy       2>/dev/null
sudo rmmod spi-daisy   2>/dev/null

sudo insmod ./spi-daisy.ko && sudo insmod ./daisy.ko
sleep 1
./up.sh
sudo route -e
ping -c 2 44.130.60.100
