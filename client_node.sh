#!/bin/bash

FIFO_PATH="/tmp/proxpipe"
THRESHOLD=100
SERVER_CAN_ID="100"
CLIENT_CAN_ID="101"
LED=17

# Configure LED GPIO
echo "$LED" > /sys/class/gpio/export 2>/dev/null
echo "out" > /sys/class/gpio/gpio$LED/direction
echo "0" > /sys/class/gpio/gpio$LED/value


# Setup CAN interface
ip link set can0 up type can bitrate 125000

# Read proximity from FIFO
read_proximity() {
    if read -r val < "$FIFO_PATH"; then
        echo "$val"
    else
        echo "0"
    fi
}

STATE="CAN_RX"

while true; do
    case "$STATE" in
        "CAN_RX")
            echo "[Client] State: CAN_RX - Waiting for Exit signal..."
            echo "0" > /sys/class/gpio/gpio$LED/value
            candump can0 | while read -r iface canid bracket dlc data; do
                if [ "$canid" = "$SERVER_CAN_ID" ]; then
                    val_dec=$((16#$dlc))
                    echo "[Client] Received Exit CAN frame. Value=$val_dec"
                    echo "1" > /sys/class/gpio/gpio$LED/value
                    STATE="CAN_TX"
                    break
                fi
            done
            ;;

        "CAN_TX")
            echo "[Client] State: CAN_TX - Reading proximity from FIFO and Transmitting back through CAN"
            prox_val=$(read_proximity)
            echo "[Client] Proximity: $prox_val"

            if [ "$prox_val" -gt "$THRESHOLD" ]; then
                frame=$(printf "%02X" "$prox_val")
                echo "[Client] Sending Entrance signal CAN frame: $frame"
                cansend can0 $CLIENT_CAN_ID#$frame
                STATE="CAN_RX"
            fi

            sleep 1
            ;;

        *)
            echo "Unknown state: $STATE"
            exit 1
            ;;
    esac
done

