// server_can_tx.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdint.h>

#define FIFO_PATH "/tmp/proxpipe"
#define THRESHOLD 100
#define SERVER_CAN_ID 0x100

int main() {
    int fifo_fd, can_socket;
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame frame;

    fifo_fd = open(FIFO_PATH, O_RDONLY);
    if (fifo_fd < 0) {
        perror("FIFO open failed");
        return 1;
    }

    if ((can_socket = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
        perror("CAN socket failed");
        return 1;
    }

    strcpy(ifr.ifr_name, "can0");
    if (ioctl(can_socket, SIOCGIFINDEX, &ifr) < 0) {
        perror("SIOCGIFINDEX failed");
        return 1;
    }

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(can_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("CAN bind failed");
        return 1;
    }

    while (1) {
        char buf[16];
        int proximity;

        if (read(fifo_fd, buf, sizeof(buf)) > 0) {
            proximity = atoi(buf);
            printf("[Server] Proximity = %d\n", proximity);
            if (proximity > THRESHOLD) {
                frame.can_id = SERVER_CAN_ID;
                frame.can_dlc = 1;
                frame.data[0] = (uint8_t)proximity;

                if (write(can_socket, &frame, sizeof(frame)) != -1) {
                    printf("[Server] Sent CAN frame: %d\n", proximity);
                } else {
                    perror("CAN write failed");
                }
            }
        }
        sleep(1);
    }

    close(fifo_fd);
    close(can_socket);
    return 0;
}

