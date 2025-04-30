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

int read_proximity_value() {
    FILE *fp = fopen(FILE_PATH, "r");
    if (!fp) {
        perror("[ERROR] Could not open proximity file");
        return -1;
    }

    char buf[16];
    if (!fgets(buf, sizeof(buf), fp)) {
        perror("[ERROR] Could not read proximity value");
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return atoi(buf);
}

int main() {
    int can_socket;
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame frame;
    system_state_t state = STATE_WAIT_FOR_SERVER;

    export_gpio(LED_GPIO);
    set_gpio(LED_GPIO, 0);  // Ensure LED is initially OFF

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
            printf("[STATE] Entered WAIT_FOR_SERVER\n");

            struct can_frame frame1, frame2;
            int nbytes1 = read(can_socket, &frame1, sizeof(frame1));
            if (nbytes1 > 0 && frame1.can_id == SERVER_CAN_ID) {
                printf("[CAN RX] Frame 1: %d\n", frame1.data[0]);
            } else {
                printf("[CAN RX] Failed to receive frame 1\n");
                break;
            }

            sleep(2);  // wait before reading second frame

            int nbytes2 = read(can_socket, &frame2, sizeof(frame2));
            if (nbytes2 > 0 && frame2.can_id == SERVER_CAN_ID) {
                printf("[CAN RX] Frame 2: %d\n", frame2.data[0]);

                if (frame1.data[0] > THRESHOLD && frame2.data[0] > THRESHOLD) {
                    set_gpio(LED_GPIO, 1);
                    printf("[LED] Turned ON (from server)\n");
                    state = STATE_CHECK_LOCAL_SENSOR;
                } else {
                    printf("[INFO] One or both frame values below threshold\n");
                }
            } else {
                printf("[CAN RX] Failed to receive frame 2\n");
            }

            break;
        }

        case STATE_CHECK_LOCAL_SENSOR: {
            printf("[STATE] Entered CHECK_LOCAL_SENSOR\n");

            int first = read_proximity_value();
            if (first == -1) break;
            printf("[Sensor] First proximity = %d\n", first);

            sleep(2);

            int second = read_proximity_value();
            if (second == -1) break;
            printf("[Sensor] Second proximity = %d\n", second);

            if (first > THRESHOLD && second > THRESHOLD) {
                set_gpio(LED_GPIO, 0);
                printf("[LED] Turned OFF (from local sensor double check)\n");
            } else {
                printf("[INFO] Conditions not met to turn off LED\n");
            }

            state = STATE_WAIT_FOR_SERVER;
            printf("[STATE] Returning to WAIT_FOR_SERVER\n");
            break;
        }
        }

        sleep(1);  // Avoid tight loop
    }

    close(can_socket);
    return 0;
}

