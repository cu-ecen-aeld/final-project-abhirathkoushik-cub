#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <stdlib.h>
#include <stdint.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

#define I2C_DEVICE "/dev/i2c-1"
#define APDS9960_ADDR 0x39
#define DEVICE_ID_REG 0x92
#define ENABLE_REG 0x80
#define PROXIMITY_REG 0x9C
#define FILE_PATH "/tmp/proxval.txt"
#define PINET_IOCTL_SEND_SENSOR_DATA _IOW('p', 1, int)

// Check if the sensor is connected
int check_sensor_connected(int fd)
{
    uint8_t reg = DEVICE_ID_REG;
    uint8_t data;

    struct i2c_msg messages[2] = {
        {APDS9960_ADDR, 0, 1, &reg},
        {APDS9960_ADDR, I2C_M_RD, 1, &data}
    };

    struct i2c_rdwr_ioctl_data ioctl_data = {messages, 2};

    if (ioctl(fd, I2C_RDWR, &ioctl_data) < 0)
    {
        syslog(LOG_ERR, "I2C_RDWR ioctl failed to read ID reg");
        return EXIT_FAILURE;
    }

    syslog(LOG_INFO, "APDS9960 Device ID: 0x%02X", data);

    if (data != 0xAB)
    {
        syslog(LOG_ERR, "Unexpected APDS9960 Device ID: 0x%02X", data);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// Enable proximity sensing
int enable_proximity(int fd)
{
    uint8_t buf[2];
    buf[0] = ENABLE_REG;
    buf[1] = 0x05; // PON (power ON) + PEN (proximity enable)

    if (write(fd, buf, 2) != 2)
    {
        syslog(LOG_ERR, "Failed to write to ENABLE_REG");
        return EXIT_FAILURE;
    }

    syslog(LOG_INFO, "Proximity mode enabled by writing 0x05 to ENABLE_REG");
    return EXIT_SUCCESS;
}

// Read proximity data
int read_proximity_data(int fd)
{
    uint8_t reg = PROXIMITY_REG;
    uint8_t data;

    struct i2c_msg messages[2] = {
        {APDS9960_ADDR, 0, 1, &reg},
        {APDS9960_ADDR, I2C_M_RD, 1, &data}
    };

    struct i2c_rdwr_ioctl_data ioctl_data = {messages, 2};

    if (ioctl(fd, I2C_RDWR, &ioctl_data) < 0)
    {
        syslog(LOG_ERR, "Failed to read proximity data");
        return -1;
    }

    syslog(LOG_INFO, "Proximity value: %d", data);
    return data;
}

int main()
{
    openlog("APDS_LOG", LOG_PID, LOG_USER);

    int fd = open(I2C_DEVICE, O_RDWR);
    if (fd < 0)
    {
        syslog(LOG_ERR, "Unable to open I2C device");
        exit(EXIT_FAILURE);
    }

    if (ioctl(fd, I2C_SLAVE, APDS9960_ADDR) < 0)
    {
        syslog(LOG_ERR, "Failed to set I2C slave address");
        close(fd);
        return EXIT_FAILURE;
    }

    if (check_sensor_connected(fd) != 0)
    {
        close(fd);
        return EXIT_FAILURE;
    }

    if (enable_proximity(fd) != 0)
    {
        close(fd);
        return EXIT_FAILURE;
    }

    int pinet_file = open("/dev/pinet", O_RDWR);
    if (pinet_file < 0)
    {
        syslog(LOG_ERR, "open /dev/pinet failed: %s", strerror(errno));
    }

    while (1)
    {
        int proximity = read_proximity_data(fd);
        if (proximity == -1)
        {
            syslog(LOG_ERR, "Failed to read proximity data");
            break;
        }

        // ✅ Write to text file instead of FIFO
        FILE *fp = fopen(FILE_PATH, "w");
        if (fp)
        {
            fprintf(fp, "%d\n", proximity);
            fclose(fp);
        }
        else
        {
            syslog(LOG_ERR, "Failed to write to proximity text file: %s", strerror(errno));
        }

        // Optional: send to /dev/pinet
        if (pinet_file >= 0)
        {
            if (ioctl(pinet_file, PINET_IOCTL_SEND_SENSOR_DATA, &proximity) == -1)
            {
                syslog(LOG_ERR, "ioctl to /dev/pinet failed");
            }
        }

        sleep(1);
    }

    close(fd);
    if (pinet_file >= 0)
        close(pinet_file);
    closelog();
    return EXIT_SUCCESS;
}
