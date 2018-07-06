# Resetting USB on a Linux machine
echo "start to transfer:"
counter=1
while true
do
pictureFile="/home/dfyuan/dfyuan/image44/640x480_$counter.yuv" 
echo "$pictureFile"
if [ -f "$pictureFile" ]; then 
	echo "teststset"
	/usr/bin/ffmpeg -s	640x480 -f rawvideo -pix_fmt nv12 -i $pictureFile /home/dfyuan/dfyuan/image44/640x480_$counter.bmp 
fi 
let "counter=counter+1" 
done
