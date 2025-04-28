#!/bin/sh

FIFO_PATH="/tmp/proxpipe"
THRESHOLD=50
SERVER_CAN_ID="100"
CLIENT_CAN_ID="101"

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

STATE="CAN_TX"

while true; do
    case "$STATE" in
        "CAN_TX")
            echo "[Server] State: CAN_TX - Reading proximity from FIFO..."
            prox_val=$(read_proximity)
            echo "[Server] Proximity: $prox_val"

            if [ "$prox_val" -gt "$THRESHOLD" ]; then
                frame=$(printf "%02X" "$prox_val")
                echo "[Server] Sending EXIT signal CAN frame: $frame"
                cansend can0 $SERVER_CAN_ID#$frame
                STATE="CAN_RX"
            fi

            sleep 1
            ;;

        "CAN_RX")
            echo "[Server] State: CAN_RX - Waiting for Entrance signal..."
            candump can0 | while read -r iface canid bracket dlc data; do
                if [ "$canid" = "$CLIENT_CAN_ID" ]; then
                    val_dec=$((16#$dlc))
                    echo "[Server] Received Entrance CAN frame. Value=$val_dec"
                    # After receiving, switch back
                    STATE="CAN_TX"
                    break
                fi
            done
            ;;

        *)
            echo "Unknown state: $STATE"
            exit 1
            ;;
    esac
done

