while true;do 
echo "capacity : `cat /sys/class/power_supply/battery/capacity`" >> pd_test.txt;
echo "current_now : `cat /sys/class/power_supply/battery/current_now`" >> pd_test.txt;
echo "voltage_now : `cat /sys/class/power_supply/battery/voltage_now`" >> pd_test.txt;
echo "status : `cat /sys/class/power_supply/battery/status`" >> pd_test.txt;
echo "temp : `cat /sys/class/power_supply/battery/temp`" >> pd_test.txt;
echo "chg_type : `cat /sys/class/power_supply/battery/charge_type`" >> pd_test.txt;
echo "in_temp_skin_temp_input : `cat /sys/bus/iio/devices/iio\:device0/in_temp_skin_temp_input`" >> pd_test.txt;
echo "in_temp_skin_temp_hot_input : `cat /sys/bus/iio/devices/iio\:device0/in_temp_skin_temp_hot_input`" >> pd_test.txt;
echo "date : `date`" >> pd_test.txt;sleep 10;echo =====;
done
