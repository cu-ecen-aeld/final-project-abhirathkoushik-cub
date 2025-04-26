#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/i2c-smbus.h>
#include <linux/init.h>

#define APDS_ID_REG	0x92	// ID reg address
#define APDS_I2C_ADDR         (0x39)   // APDS-9960 default I2C address (you can adjust if needed)
#define APDS_EXPECTED_ID      (0xAB)   // Replace with actual expected ID if datasheet says different

static int apds_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret, id_val;
    dev_info(&client->dev, "APDS Hello World Probe!\n");

    // Step 1: Send quick write to check ACK
    ret = i2c_smbus_write_quick(client, I2C_SMBUS_WRITE);
    if (ret < 0) {
        dev_err(&client->dev, "APDS device not responding at 0x%02x (NACK)\n", client->addr);
        return ret;
    }
    dev_info(&client->dev, "APDS device ACKed at address 0x%02x\n", client->addr);

    // Step 2: Read ID register to confirm device
    id_val = i2c_smbus_read_byte_data(client, APDS_ID_REG);
    if (id_val < 0) {
        dev_err(&client->dev, "Failed to read ID register from device\n");
        return id_val;
    }

    dev_info(&client->dev, "APDS device ID read: 0x%02x\n", id_val);

    if (id_val != APDS_EXPECTED_ID) {
        dev_err(&client->dev, "Unexpected device ID (expected 0x%02x)\n", APDS_EXPECTED_ID);
        return -ENODEV;
    }

    dev_info(&client->dev, "APDS sensor successfully detected!\n");

    return 0;
}

static int apds_remove(struct i2c_client *client)
{
    dev_info(&client->dev, "APDS driver removed\n");
    return 0;
}

static const struct i2c_device_id apds_id[] = {
    { "apds-driver", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, apds_id);

static const struct of_device_id apds_of_match[] = {
    { .compatible = "apds-driver" },
    { }
};
MODULE_DEVICE_TABLE(of, apds_of_match);

static struct i2c_driver apds_driver = {
    .driver = {
        .name = "apds-driver",
        .of_match_table = apds_of_match,
    },
    .probe    = apds_probe,
    .remove   = apds_remove,
    .id_table = apds_id,
};

module_i2c_driver(apds_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Parth Varsani");
MODULE_DESCRIPTION("Hello World I2C Driver for APDS Sensor");

