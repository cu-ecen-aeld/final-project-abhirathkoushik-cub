#!/bin/sh

ip link set can0 up type can bitrate 125000

FIFO="/tmp/proxpipe"


while read -r val; do
    val=$(echo "$val" | tr -d '[:spcae:]')
    val_hex=$(printf "%02X" "$val")

    frame="$val_hex"

    echo "$val, send: $frame"
    cansend can0 123#"$frame"
done < "$FIFO"
