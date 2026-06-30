/* INCLUDE */
#include <stdint.h>
#include <stdbool.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>

/* PINS */
#define I2C_NODE DT_NODELABEL(drv2625)
#define NRST_PORT "GPIO_1"
#define NRST_PIN 15

/* REGISTERS */
#define DIAG_RESULT_REG 0x01
#define MODE_REG 0x07
#define LRA_ERM_REG 0x08
#define GO_REG 0x0C
#define RATED_VOLTAGE_REG 0x1f
#define OD_CLAMP_REG 0x20
#define DRIVE_TIME_REG 0x27

/* REGISTER MASKS */
#define DIAG_RESULT_MASK 0x80
#define MODE_MASK 0x03
#define LRA_ERM_MASK 0x80
#define GO_MASK 0x01
#define DRIVE_TIME_MASK 0x1f

/* MOTOR STRUCT */
struct motor {
        uint8_t ratedVoltage; // Calculated with Eq. 6/7 in DRV2625 datasheet
        uint8_t odClamp; // Calculated with Eq. 8/9 in DRV2625 datasheet
        // DRIVE_TIME[4:0] is the five least significant bits of driveTime.
        uint8_t driveTime = 0x10; // See 7.6.1.1 and Table 8-40 in the datasheet
        bool isLRA;
};

/* PUBLIC FUNCTIONS */
int configuration(struct motor* myMotor);
