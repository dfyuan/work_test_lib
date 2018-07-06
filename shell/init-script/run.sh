#!/bin/sh
echo "init sensor and iav-mjep stream"

export PATH=$PATH:/usr/local/bin

/usr/local/bin/init.sh --ov4689 
/usr/local/bin/test_image -i 0 &
/usr/local/bin/test_encode -i0 -f 30 -V 480i --cvbs 
#/usr/local/bin/test_encode -i0 -X --btype off J  --btype off
/usr/local/bin/test_encode -A -m 720p  -e 
/mnt/socamapp -t 