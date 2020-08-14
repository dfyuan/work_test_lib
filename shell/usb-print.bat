adb root
adb wait-for-device
adb shell "echo -n 'file drivers/usb/core/* +p' > /sys/kernel/debug/dynamic_debug/control"
adb shell "echo -n 'file drivers/usb/phy/* +p' > /sys/kernel/debug/dynamic_debug/control"
adb shell "echo -n 'file drivers/usb/host/* +p' > /sys/kernel/debug/dynamic_debug/control"
adb shell "echo -n 'file drivers/usb/dwc3/* +p' > /sys/kernel/debug/dynamic_debug/control"
adb shell "echo 8 > /proc/sys/kernel/printk"
pause