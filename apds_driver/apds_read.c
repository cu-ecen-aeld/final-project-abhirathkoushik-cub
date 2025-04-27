#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#define APDS_I2C_ADDR 0x39
#define APDS_ID_REG   0x92
#define EXPECTED_ID   0xAB  // Expected ID for APDS sensor

int main()
{
    int file;
    char *filename = "/dev/i2c-1";

    if ((file = open(filename, O_RDWR)) < 0) {
        perror("Failed to open i2c bus");
        exit(1);
    }

    if (ioctl(file, I2C_SLAVE, APDS_I2C_ADDR) < 0) {
        perror("Failed to set slave address");
        close(file);
        exit(2);
    }

    // Read ID register
    int id = i2c_smbus_read_byte_data(file, APDS_ID_REG);
    if (id < 0) {
        perror("Failed to read ID register");
        close(file);
        exit(3);
    }

    printf("APDS ID Register: 0x%02X\n", id);

    if (id == EXPECTED_ID) {
        printf("APDS Sensor detected successfully!\n");
    } else {
        printf("Unexpected APDS ID. Check wiring or sensor.\n");
    }

    close(file);
    return 0;
}

