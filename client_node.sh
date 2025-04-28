#!/bin/sh

FIFO_PATH="/tmp/proxpipe"
PIPE_CAN_DUMP="/tmp/candump_pipe"
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

# Create FIFO for candump output if needed
[ -p "$PIPE_CAN_DUMP" ] || mkfifo "$PIPE_CAN_DUMP"

STATE="CAN_RX"

while true; do
    case "$STATE" in
        "CAN_RX")
            echo "[Client] State: CAN_RX - Waiting for Exit signal..."
            echo "0" > /sys/class/gpio/gpio$LED/value

            candump can0 > "$PIPE_CAN_DUMP" &
            CANDUMP_PID=$!

            while read -r iface canid bracket dlc data; do
                if [ "$canid" = "$SERVER_CAN_ID" ]; then
                    val_dec=$((16#$dlc))
                    echo "[Client] Received Exit CAN frame. Value=$val_dec"
                    echo "1" > /sys/class/gpio/gpio$LED/value
                    STATE="CAN_TX"
                    kill "$CANDUMP_PID"
                    break
                fi
            done < "$PIPE_CAN_DUMP"
            ;;
        
        "CAN_TX")
            echo "[Client] State: CAN_TX - Reading proximity from FIFO and Transmitting back through CAN"

            # Make sure /usr/bin/apds_read is running
            if ! pgrep -f "apds_read" >/dev/null; then
                echo "[Client] starting /usr/bin/apds_read..."
                /usr/bin/apds_read &
                sleep 1
            fi

            while read -r val; do
                val=$(echo "$val" | tr -d '[:space:]')
                val_hex=$(printf "%02X" "$val")
                frame="$val_hex"
                echo "$val, Send=$frame"

                if [ "$val" -gt "$THRESHOLD" ]; then
                    cansend can0 $CLIENT_CAN_ID#$frame
                    STATE="CAN_RX"
                    break
                fi
            done < "$FIFO_PATH"
            ;;
        
        *)
            echo "Unknown state: $STATE"
            exit 1
            ;;
    esac
done

