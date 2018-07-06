#!/bin/sh
echo "starting....."

make clean
make 

chmod 777 gdt_uvcvideo.ko
cp gdt_uvcvideo.ko ../../../fs/squashfs-root/drivers
echo "end......"
