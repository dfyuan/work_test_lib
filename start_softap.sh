#!/system/bin/sh

ssid=xxh_hotspot
passwd=12345678

LOGCAT_TAG=softap
interface=wlan0

stop(){
      ndc netd 6001 tether stop
      ndc netd 6002 softap stopap
}

power_on(){
      echo 149 > /sys/class/gpio/export
      echo out >  /sys/class/gpio/gpio149/direction
      echo 1 >  /sys/class/gpio/gpio149/value
      sleep 3
}

start(){
       busybox ifconfig $interface down
       ndc netd 5000 softap fwreload $interface AP
       busybox ifconfig $interface up
	   # cat /sys/class/net/wap0/address | busybox awk -F':' '{print "-"$5$6}'
	   ssid_suffix=`cat /sys/class/net/$interface/address | busybox awk -F':' '{print "-"$5$6}'`
	   ssid+=$ssid_suffix
       log -t $LOGCAT_TAG "Start wifi softap: name=$ssid, pwd=$passwd"
       echo "Start wifi softap: name=$ssid, pwd=$passwd"

       ndc netd 5001 softap set $interface $ssid broadcast 153 wpa2-psk $passwd
       ndc netd 5002 softap startap

       busybox ifconfig $interface 192.168.43.1                                                                          
       ndc netd 5003 tether start 192.168.43.2 192.168.43.254
       
       echo 1 > /proc/sys/net/ipv4/ip_forward
       ndc netd 5004 nat enable $interface 1 192.168.43.1/24
       
       ip rule add from all lookup main pref 9999
	   
	   SYSTEM_DNS=$(getprop net.dns1)
	   iptables -t nat -I PREROUTING -i $interface  -p udp --dport 53 -j DNAT --to-destination $SYSTEM_DNS
}

stop
start
