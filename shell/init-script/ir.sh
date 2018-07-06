#!/bin/sh 
#day gpio18=0,gpio17=1
#
export GPIO=18
cd /sys/class/gpio
echo $GPIO > export

cd /sys/class/gpio/gpio$GPIO
echo "out" > direction
echo 0 > /sys/class/gpio/gpio18/value

export GPIO=17
cd /sys/class/gpio
echo $GPIO > export

cd /sys/class/gpio/gpio$GPIO
echo "out" > direction
echo 1 > /sys/class/gpio/gpio17/value
sleep 3
echo 0 > /sys/class/gpio/gpio17/value
echo 0 > /sys/class/gpio/gpio18/value