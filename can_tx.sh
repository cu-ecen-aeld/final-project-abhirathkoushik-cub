#!/bin/sh

ip link set can0 up type can bitrate 125000

i2cset -y 1 0x39 0x80 0x05
sleep 1

echo "Reading proximity sensor data ...."

while true; do
    val=$(i2cget -y 1 0x39 0x9c)
    val_hex=$(printf "%02X" "$val")

    frame="000000$val_hex"

    echo "sample $count: Proximity = $dec_val sending: $frame"
    cansend can0 123#"$frame"
    sleep 1
done
