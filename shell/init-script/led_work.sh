#!/bin/sh
export GPIO=13
cd /sys/class/gpio
echo $GPIO > export

cd /sys/class/gpio/gpio$GPIO
echo "out" > direction

for i in `seq 10`
do
if [ $(($i%2)) -eq 0 ];then
        #echo "even $i"
        echo 1 > /sys/class/gpio/gpio13/value
        sleep 0.005
else
        #echo "odd $i"
        echo 0 > /sys/class/gpio/gpio13/value
        sleep 0.020
fi
done

