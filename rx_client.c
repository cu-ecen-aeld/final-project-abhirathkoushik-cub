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
    struct can_frame frame, temp_frame;
    system_state_t state = STATE_WAIT_FOR_SERVER;

    export_gpio(LED_GPIO);
    set_gpio(LED_GPIO, 0);  

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

    // Set CAN socket to non-blocking mode
    int flags = fcntl(can_socket, F_GETFL, 0);
    fcntl(can_socket, F_SETFL, flags | O_NONBLOCK);

    printf("[STATE] Starting in WAIT_FOR_SERVER state...\n");

    while (1) {
        switch (state) {
        case STATE_WAIT_FOR_SERVER: {
            printf("[STATE] Entered WAIT_FOR_SERVER\n");

	    // Drain all stale CAN frames
	    while (read(can_socket, &temp_frame, sizeof(temp_frame)) > 0) {
	    }

        // Read a fresh CAN frame
	int nbytes = read(can_socket, &frame, sizeof(frame));
	if (nbytes > 0 && frame.can_id == SERVER_CAN_ID) {
	    printf("[CAN RX] Received: %d\n", frame.data[0]);
	    if (frame.data[0] > THRESHOLD) {
		set_gpio(LED_GPIO, 1);
		printf("[LED] Turned ON (from server)\n");
		state = STATE_CHECK_LOCAL_SENSOR;
	    }
	}
                sleep(1);
                break;
            }
	case STATE_CHECK_LOCAL_SENSOR: {
                printf("Entered STATE_CHECK_LOCAL_SENSOR\n");

                while (state == STATE_CHECK_LOCAL_SENSOR) {
                    FILE *fp = fopen(FILE_PATH, "r");
                    if (fp) {
                        char buf[16];
                        if (fgets(buf, sizeof(buf), fp)) {
                            int proximity = atoi(buf);
                            printf("[Sensor] Proximity: %d\n", proximity);
                            if (proximity > THRESHOLD) {
                                set_gpio(LED_GPIO, 0);
                                printf("[LED] Turned OFF (from local sensor)\n");
                                state = STATE_WAIT_FOR_SERVER;
                                printf("[STATE] Back to WAIT_FOR_SERVER\n");
                            }
                        } else {
                            perror("[ERROR] Could not read proximity value from file");
                        }
                        fclose(fp);
                    } else {
                        perror("[ERROR] Could not open proximity value file");
                    }

                    sleep(1);  
                }

                break;
            }
	}        

        sleep(1);
    }

    close(can_socket);
    return 0;
}

