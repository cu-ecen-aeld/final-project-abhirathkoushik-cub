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

typedef enum {
    STATE_WAIT_FOR_SERVER,
    STATE_CHECK_LOCAL_SENSOR
} system_state_t;

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

int open_fifo_reader() {
    int fd;
    while ((fd = open(FIFO_PATH, O_RDONLY | O_NONBLOCK)) < 0) {
        perror("[WAIT] Waiting for FIFO writer...");
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
    int fifo_fd = -1;
    char buf[16];
    system_state_t state = STATE_WAIT_FOR_SERVER;

    export_gpio(LED_GPIO);
    set_gpio(LED_GPIO, 0);  // LED OFF

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

    printf("[STATE] Starting in WAIT_FOR_SERVER state...\n");

    while (1) {
        switch (state) {
            case STATE_WAIT_FOR_SERVER: {
                int nbytes = read(can_socket, &frame, sizeof(frame));
                if (nbytes > 0 && frame.can_id == SERVER_CAN_ID) {
                    printf("[CAN RX] Received: %d\n", frame.data[0]);
                    if (frame.data[0] > THRESHOLD) {
                        set_gpio(LED_GPIO, 1);
                        printf("[LED] Turned ON (from server)\n");
                        state = STATE_CHECK_LOCAL_SENSOR;
                        fifo_fd = open_fifo_reader();
                    }
                }
                break;
            }

            case STATE_CHECK_LOCAL_SENSOR: {
                memset(buf, 0, sizeof(buf));
                ssize_t r = read(fifo_fd, buf, sizeof(buf) - 1);
		
		printf("[DEBUG] FIFO read returned r = %zd\n", r);

                if (r > 0) {
                    int proximity = atoi(buf);
                    printf("[Sensor] Proximity: %d\n", proximity);
                    if (proximity > THRESHOLD) {
                        set_gpio(LED_GPIO, 0);
                        printf("[LED] Turned OFF (from sensor)\n");
                        close(fifo_fd);
                        state = STATE_WAIT_FOR_SERVER;
                        printf("[STATE] Switched to WAIT_FOR_SERVER\n");
                    }
                } else if (r == 0) {
                    close(fifo_fd);
                    printf("[INFO] FIFO writer closed, reopening...\n");
                    fifo_fd = open_fifo_reader();
                } else if (r < 0 && errno != EAGAIN) {
                    perror("[ERROR] FIFO read failed");
                }

                break;
            }
        }

        sleep(1);
    }

    close(can_socket);
    if (fifo_fd >= 0) close(fifo_fd);
    return 0;
}

