#!/system/bin/sh
busybox ifconfig wlan0 192.168.1.1 up
wl down
wl apsta 0
wl ap 1
wl wsec 0
wl wpa_auth 0
wl channel 1
wl mpc 0
wl ssid –C 0 rk3399ap
wl up
wl bss –C 0 up
ndc netd 5003 tether start 192.168.1.2 192.168.1.254