#!/bin/sh

LED=17
BUZZER=27

# Export GPIOs
echo "$LED" > /sys/class/gpio/export 2>/dev/null
echo "$BUZZER" > /sys/class/gpio/export 2>/dev/null

# Set GPIO directions
echo "out" > /sys/class/gpio/gpio$LED/direction
echo "out" > /sys/class/gpio/gpio$BUZZER/direction

# Set initial GPIO values to 0
echo "0" > /sys/class/gpio/gpio$LED/value
echo "0" > /sys/class/gpio/gpio$BUZZER/value

# Bring up CAN interface
ip link set can0 up type can bitrate 125000

# Listen to CAN bus and react
candump can0 | while read -r iface canid bracket dlc data; do
    if [[ "$dlc" =~ ^[0-9A-Fa-f]+$ ]]; then
        val_dec=$((16#$dlc))
        echo "Proximity: $val_dec"

        if [ "$val_dec" -gt 100 ]; then
            echo "1" > /sys/class/gpio/gpio$LED/value
            echo "1" > /sys/class/gpio/gpio$BUZZER/value
        else
            echo "0" > /sys/class/gpio/gpio$LED/value
            echo "0" > /sys/class/gpio/gpio$BUZZER/value
        fi
    fi
done

