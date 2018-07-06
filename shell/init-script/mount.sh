/sbin/ifconfig eth0 10.0.22.236 netmask 255.255.255.0
/bin/mount -t nfs -o nolock 10.0.22.93:/home/dfyuan/projects/ambarella/ambarella_nfs /mnt
insmod /mnt/ezy_buf.ko
insmod /mnt/gdt_uvcvideo.ko
