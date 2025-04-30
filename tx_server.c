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

#define FILE_PATH "/tmp/proxval.txt"
#define THRESHOLD 100
#define SERVER_CAN_ID 0x100

int main() {
    int can_socket;
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame frame;

    // Setup CAN socket
    if ((can_socket = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
        perror("CAN socket failed");
        return 1;
    }

    strcpy(ifr.ifr_name, "can0");
    if (ioctl(can_socket, SIOCGIFINDEX, &ifr) < 0) {
        perror("SIOCGIFINDEX failed");
        close(can_socket);
        return 1;
    }

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(can_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("CAN bind failed");
        close(can_socket);
        return 1;
    }

    while (1) {
        FILE *fp = fopen(FILE_PATH, "r");
        if (fp) {
            char buf[16];
            if (fgets(buf, sizeof(buf), fp)) {
                int proximity = atoi(buf);
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
            } else {
                perror("Failed to read from proximity file");
            }
            fclose(fp);
        } else {
            perror("Failed to open proximity file");
        }

        sleep(1);
    }

    close(can_socket);
    return 0;
}

