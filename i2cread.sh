#!/bin/sh
while true
do
  proximity=$(cat /sys/bus/i2c/devices/1-0039/proximity)
  echo "Proximity value: $proximity"
  sleep 1
done

