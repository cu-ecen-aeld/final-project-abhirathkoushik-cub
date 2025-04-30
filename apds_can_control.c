#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>

int main() {
    int s;
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame frame;

    // Open CAN raw socket
    s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (s < 0) {
        perror("Socket");
        return 1;
    }

    // Specify CAN interface
    strcpy(ifr.ifr_name, "can0");
    if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) {
        perror("SIOCGIFINDEX");
        return 1;
    }

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    // Bind the socket
    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Bind");
        return 1;
    }

    // Prepare CAN frame
    frame.can_id = 0x100;        // CAN ID
    frame.can_dlc = 1;           // Data length
    frame.data[0] = 0x7F;        // Payload byte (can be any value)

    // Send the frame
    if (write(s, &frame, sizeof(frame)) != sizeof(frame)) {
        perror("Write");
        return 1;
    }

    printf("CAN frame sent: ID=0x%X Data=0x%02X\n", frame.can_id, frame.data[0]);

    close(s);
    return 0;
}

