#!/bin/bash
# BeagleBone Black services startup script

if [ "$(id -u)" != "0" ]; then
    echo "This script must be run as root" 1>&2
    exit 1
fi

# Sync time
echo "Syncing time..."
ntpdate ntp.ubuntu.com

# Check for wifi adapter
if lsusb | grep -q "USB-N10"; then
    echo "Found USB wifi adapter"
    if ifconfig wlan0 | grep -q "inet addr"; then
        echo "wlan0 IP address is assigned: $(hostname -I | awk '{print $1}')"
    else
        echo -ne "wlan0 does not have an IP address assigned, attempting DHCP...\t"
        if dhclient wlan0; then
            echo "Success."
        else
            echo "Failed."
        fi
    fi
else
    echo "USB wifi adapter not found!"
    exit 1
fi

# Enable UART4
echo -ne "Enabling UART4...\t"
echo BB-UART4 > /sys/devices/bone_capemgr.*/slots && echo "Done."

# Enable ADC pins
echo -ne "Enabling ADC pins...\t"
echo BB-ADC > /sys/devices/bone_capemgr.*/slots && echo "Done."

# Start joystick daemon in background
echo -ne "Starting joystick daemon...\t"
if lsusb | grep -q "F710 Wireless Gamepad"; then
    xboxdrv --silent --quiet &
    echo "Started."
else
    echo "Logitech gamepad not found!"
    echo "joystick input device not initialized."
fi

# Start camera streaming in background
echo -ne "Starting camera stream...\t"
if lsusb | grep -q "HD Webcam C510"; then
    /home/ubuntu/mjpg-streamer/mjpg_streamer -b \
    -i "/home/ubuntu/mjpg-streamer/input_uvc.so -d /dev/video0 -r 640x480 -f 5" \
    -o "/home/ubuntu/mjpg-streamer/output_http.so -w /home/ubuntu/mjpg-streamer/www -p 8080"
    echo "Web camera streaming started."
else
    echo "Web camera not found!"
    echo "Streaming has not started."
fi

# Run joystick program using UART4 and js0 device
if [ -c /dev/input/js0 ]; then
    echo -ne "Starting joystick program...\t"
    /home/ubuntu/joystick /dev/ttyO4
else
    echo "Cannot start joystick program, Logitech gamepad not initialized."
fi

echo "Startup script complete."
exit
