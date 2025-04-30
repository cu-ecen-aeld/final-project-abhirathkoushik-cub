// client_can_control.c
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
#define SERVER_CAN_ID 0x100
#define THRESHOLD 100
#define LED_GPIO 17

void export_gpio(int gpio) {
    char path[64];
    int fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd >= 0) {
        dprintf(fd, "%d", gpio);
        close(fd);
    }
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", gpio);
    fd = open(path, O_WRONLY);
    if (fd >= 0) {
        write(fd, "out", 3);
        close(fd);
    }
}

void set_gpio(int gpio, int value) {
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", gpio);
    int fd = open(path, O_WRONLY);
    if (fd >= 0) {
        dprintf(fd, "%d", value);
        close(fd);
    }
}

int main() {
    int can_socket, fifo_fd;
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame frame;

    export_gpio(LED_GPIO);
    set_gpio(LED_GPIO, 0); // initially OFF

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
        // 1. Check for CAN from server
        int nbytes = read(can_socket, &frame, sizeof(frame));
        if (nbytes > 0 && frame.can_id == SERVER_CAN_ID) {
            printf("[Client] Received CAN: %d\n", frame.data[0]);
            if (frame.data[0] > THRESHOLD) {
                set_gpio(LED_GPIO, 1); // Turn ON LED
                printf("[Client] LED ON from server\n");
            }
        }

        // 2. Check local proximity to turn OFF LED
        char buf[16];
        int proximity;
        if (read(fifo_fd, buf, sizeof(buf)) > 0) {
            proximity = atoi(buf);
            if (proximity > THRESHOLD) {
                set_gpio(LED_GPIO, 0);
                printf("[Client] LED OFF by local sensor: %d\n", proximity);
            }
        }
        sleep(1);
    }

    close(fifo_fd);
    close(can_socket);
    return 0;
}

