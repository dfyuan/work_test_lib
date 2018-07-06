#!/bin/sh 
#for power up the wifi module
export GPIO=33
cd /sys/class/gpio
echo $GPIO > export

cd /sys/class/gpio/gpio$GPIO
echo "out" > direction
echo 1 > /sys/class/gpio/gpio33/value
sleep 1
while true
do
idProduct=$(cat /sys/bus/usb/devices/1-1/idProduct)
if [ 1022 -eq  $idProduct]; then
	echo "wifi power is up! "
else
	echo 0 > /sys/class/gpio/gpio33/value
	sleep 1
	echo 1 > /sys/class/gpio/gpio33/value
	sleep 1
fi
done