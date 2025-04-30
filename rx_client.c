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

void toggle_gpio(int gpio) {
    char path[64], val;
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", gpio);
    int fd = open(path, O_RDWR);
    if (fd >= 0) {
        read(fd, &val, 1);
        lseek(fd, 0, SEEK_SET);
        if (val == '1') write(fd, "0", 1);
        else            write(fd, "1", 1);
        close(fd);
    }
}

int open_fifo_reader() {
    int fd;
    while ((fd = open(FIFO_PATH, O_RDONLY | O_NONBLOCK)) < 0) {
        perror("Waiting for writer to open FIFO");
        sleep(1);
    }
    printf("[INFO] FIFO opened for reading\n");
    return fd;
}

int main() {
    int can_socket;
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame frame;
    char buf[16];
    int fifo_fd;

    export_gpio(LED_GPIO);
    set_gpio(LED_GPIO, 0); // initially OFF

    // Open CAN socket
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

    fifo_fd = open_fifo_reader();

    while (1) {
        // --- CAN RX block ---
        int nbytes = read(can_socket, &frame, sizeof(frame));
        if (nbytes > 0 && frame.can_id == SERVER_CAN_ID) {
            printf("[CAN RX] Received from server: %d\n", frame.data[0]);
            if (frame.data[0] > THRESHOLD) {
                set_gpio(LED_GPIO, 1);
                printf("[LED] Turned ON (command from server)\n");
            }
        }

        // --- Local proximity sensor block ---
        memset(buf, 0, sizeof(buf));
        ssize_t r = read(fifo_fd, buf, sizeof(buf) - 1);
        if (r > 0) {
            int proximity = atoi(buf);
            printf("[Sensor] Proximity = %d\n", proximity);

            if (proximity > THRESHOLD) {
                set_gpio(LED_GPIO, 0);
                printf("[LED] Turned OFF (local proximity triggered)\n");
            }
        } else if (r == 0) {
            // FIFO writer closed, reopen it
            close(fifo_fd);
            printf("[INFO] FIFO writer closed, reopening...\n");
            fifo_fd = open_fifo_reader();
        } else if (r < 0 && errno != EAGAIN) {
            perror("FIFO read failed");
        }

        sleep(1); // Prevent CPU overuse
    }

    close(fifo_fd);
    close(can_socket);
    return 0;
}

