/* INCLUDE */
#include <stdint.h>
#include <stdbool.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>

/* PINS */
#define I2C_NODE DT_NODELABEL(drv2625)

#define MCP9808 0x18
#define DEVICE_ID 0x04
#define REG_CONFIG 0x01
#define REG_UPPER_TEMP 0x02
#define REG_LOWER_TEMP 0x03
#define REG_CRIT_TEMP 0x04
#define REG_TEMP 0x05
#define REG_MANUFACTURER_ID 0x06
#define REG_DEVICE_ID 0x07

int init();
float temperature();