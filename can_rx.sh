#!/bin/sh

LED=17
BUZZER=27

echo "$LED" > /sys/class/gpio/export 2>/dev/null
echo "$BUZZER" > /sys/class/gpio/export 2>/dev/null

echo "out" > /sys/class/gpio/gpio$LED/direction
echo "out" > /sys/class/gpio/gpio$BUZZER/direction

echo "0" > /sys/class/gpio/gpio$LED/value
echo "0" > /sys/class/gpio/gpio$BUZZER/value

ip link set can0 up type can bitrate 125000
candump can0 | while read -r iface canid bracket dlc byte1 byte2 byte3 byte4; do
    echo "$line"
    val_hex=$byte3
    val_dec=$((16#$val_hex))
    # echo "$val_hex"
    echo "Proximity: $val_dec"

    if [ "$val_dec" -gt 100 ]; then
        echo "inside if"
        echo "1" > /sys/class/gpio/gpio$LED/value
        echo "1" > /sys/class/gpio/gpio$BUZZER/value
    else
        echo "0" > /sys/class/gpio/gpio$LED/value
        echo "0" > /sys/class/gpio/gpio$BUZZER/value
    fi
done
