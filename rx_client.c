// client_can_toggle.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/socket.h>

#define LED_GPIO 17
#define SERVER_CAN_ID 0x100

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

int get_gpio(int gpio) {
    char path[64];
    char value;
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", gpio);
    int fd = open(path, O_RDONLY);
    if (fd < 0) return 0;
    read(fd, &value, 1);
    close(fd);
    return value == '1' ? 1 : 0;
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

void toggle_gpio(int gpio) {
    int current = get_gpio(gpio);
    set_gpio(gpio, !current);
}

int main() {
    int can_socket;
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame frame;

    // Setup GPIO
    export_gpio(LED_GPIO);
    set_gpio(LED_GPIO, 0); // LED off initially

    // Open CAN socket
    if ((can_socket = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
        perror("CAN socket failed");
        return EXIT_FAILURE;
    }

    strcpy(ifr.ifr_name, "can0");
    if (ioctl(can_socket, SIOCGIFINDEX, &ifr) < 0) {
        perror("SIOCGIFINDEX failed");
        close(can_socket);
        return EXIT_FAILURE;
    }

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(can_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("CAN bind failed");
        close(can_socket);
        return EXIT_FAILURE;
    }

    printf("[Client] Waiting for CAN messages from server...\n");

    while (1) {
        int nbytes = read(can_socket, &frame, sizeof(frame));
        if (nbytes > 0 && frame.can_id == SERVER_CAN_ID && frame.can_dlc >= 1) {
            printf("[Client] Received %d — toggling LED\n", frame.data[0]);
            toggle_gpio(LED_GPIO);
        }

        sleep(1);
    }

    close(can_socket);
    return EXIT_SUCCESS;
}

