#!/bin/bash

sudo ifconfig dsy0 up 
sudo route add -net 44.0.0.0 netmask 255.0.0.0 dsy0
sudo route -e
