#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/init.h>

static int apds_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    dev_info(&client->dev, "APDS Hello World Probe!\n");
    return 0;
}

static int apds_remove(struct i2c_client *client)
{
    dev_info(&client->dev, "APDS driver removed\n");
    return 0;
}

static const struct i2c_device_id apds_id[] = {
    { "apds", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, apds_id);

static const struct of_device_id apds_of_match[] = {
    { .compatible = "parth,apds" },
    { }
};
MODULE_DEVICE_TABLE(of, apds_of_match);

static struct i2c_driver apds_driver = {
    .driver = {
        .name = "apds",
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

