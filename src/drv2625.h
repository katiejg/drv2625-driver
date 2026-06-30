/* INCLUDE */
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>

/* PINS */
#define I2C_NODE DT_NODELABEL(drv2625)
#define NRST_PORT "GPIO_1"
#define NRST_PIN 15

/* REGISTER MASKS */
// reg 0x07
#define MODE_MASK 0x03
// reg 0x08
#define LRA_ERM_MASK 0x80