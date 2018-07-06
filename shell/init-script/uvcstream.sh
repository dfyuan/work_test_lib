#!/bin/sh
#
# Init uvc stream test
#
start()
{
	echo "start uvcstream"
	insmod /lib/modules/3.10.73/extra/ezy_buf.ko
	sleep 1
	insmod /lib/modules/3.10.73/extra/gdt_uvcvideo.ko
	sleep 2
	insmod /lib/modules/3.10.73/extra/motor.ko
	sleep 3
	mknod /dev/g_media c 250 0
	mknod /dev/g_uvc_ctrl c 251 0
	mknod /dev/g_uvc_mjpeg c 251 1
	sleep 1
	init.sh --imx290
	sleep 2
	uvcstream_app &
    echo "mount finished!"
}
stop()
{
     echo "stop uvc stream !"
}

restart()
{
	stop
	start
}

case "$1" in
	start)
	start;;
  stop)
	stop
	;;
	restart|reload)
	restart
	;;
*)
	echo "Usage: $0 {start|stop|restart}"
	exit 1
	esac
	exit $?

