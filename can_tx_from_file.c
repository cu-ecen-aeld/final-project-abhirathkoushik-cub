#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>

#define APDS_FILE "/tmp/apds"
#define THRESHOLD 100
#define CAN_INTERFACE "can0"
#define CAN_ID 0x100

int read_proximity_from_file() {
    FILE *fp = fopen(APDS_FILE, "r");
    if (!fp) {
        perror("Failed to open /tmp/apds");
        return -1;
    }

    char buf[16];
    if (!fgets(buf, sizeof(buf), fp)) {
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return atoi(buf);
}

int main() {
    int sock;
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame frame;

    // Open CAN socket
    sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    strcpy(ifr.ifr_name, CAN_INTERFACE);
    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
        perror("ioctl");
        return 1;
    }

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }

    while (1) {
        int proximity = read_proximity_from_file();
        if (proximity < 0) {
            sleep(1);
            continue;
        }

        printf("[INFO] Read proximity: %d\n", proximity);

        if (proximity > THRESHOLD) {
            frame.can_id = CAN_ID;
            frame.can_dlc = 1;
            frame.data[0] = proximity;

            if (write(sock, &frame, sizeof(frame)) != sizeof(frame)) {
                perror("write");
            } else {
                printf("[CAN TX] Sent proximity: %d\n", proximity);
            }
        }

        sleep(1);  // wait before next read
    }

    close(sock);
    return 0;
}

