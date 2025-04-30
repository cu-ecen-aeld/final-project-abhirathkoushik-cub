#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

#define FIFO_PATH "/tmp/proxpipe"
#define SERVER_CAN_ID 0x100
#define CLIENT_CAN_ID 0x101
#define THRESHOLD 100

typedef enum {
    STATE_RX,
    STATE_TX
} node_state_t;

int main() {
    int fifo_fd, can_socket;
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame frame;
    node_state_t state = STATE_RX;

    // Open FIFO to read proximity values
    fifo_fd = open(FIFO_PATH, O_RDONLY);
    if (fifo_fd < 0) {
        perror("Failed to open FIFO");
        return 1;
    }

    // Open CAN socket
    if ((can_socket = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
        perror("CAN socket error");
        return 1;
    }

    strcpy(ifr.ifr_name, "can0");
    if (ioctl(can_socket, SIOCGIFINDEX, &ifr) < 0) {
        perror("SIOCGIFINDEX");
        return 1;
    }

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(can_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("CAN bind error");
        return 1;
    }

    while (1) {
        if (state == STATE_RX) {
            struct can_frame rx_frame;
            int nbytes = read(can_socket, &rx_frame, sizeof(rx_frame));
            if (nbytes > 0 && rx_frame.can_id == SERVER_CAN_ID) {
                printf("[RX] Received from server: %d\n", rx_frame.data[0]);
                state = STATE_TX;
            }
        } else if (state == STATE_TX) {
            char buf[16];
            int proximity;

            lseek(fifo_fd, 0, SEEK_SET);
            if (read(fifo_fd, buf, sizeof(buf)) > 0) {
                proximity = atoi(buf);
                printf("[TX] Proximity = %d\n", proximity);

                if (proximity > THRESHOLD) {
                    frame.can_id = CLIENT_CAN_ID;
                    frame.can_dlc = 1;
                    frame.data[0] = (uint8_t)proximity;

                    if (write(can_socket, &frame, sizeof(frame)) != -1) {
                        printf("[TX] Sent CAN frame to server: %d\n", proximity);
                        state = STATE_RX;
                    }
                }
            }
        }

        sleep(1); // Control polling frequency
    }

    close(fifo_fd);
    close(can_socket);
    return 0;
}

