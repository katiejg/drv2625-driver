/**
 * @file drv2625.c
 * @author Katie Jiang
 * @brief DRV2625EVM-MINI Driver for nRF5340dk
 * @version 0.1
 * @date 2026-06-30
 * @note Trigger control is currently done with Software Trigger (GO bit).
 */

#include "drv2625.h"
#include <zephyr/sys/printk.h>

static const struct i2c_dt_spec dev_i2c = I2C_DT_SPEC_GET(I2C_NODE);

static void i2c_ready() {
      if (!device_is_ready(dev_i2c.bus)) {
            printk("I2C bus %s is not ready!\n\r", dev_i2c.bus->name);
      }
      return;
}

/* I2C FUNCTIONS */
static void write_transfer(uint8_t reg_addr, uint8_t data) {
      // SINGLE-BYTE WRITE
      uint8_t buf[2] = {reg_addr, data};
      int ret = i2c_write_dt(&dev_i2c, buf, sizeof(buf));
      if (ret != 0) {
            printk("%d: Failed to write to I2C device address %x at reg. %x \n\r", ret, dev_i2c.addr, buf[0]);
      }
}

uint8_t data[2];
static uint8_t read_transfer(uint8_t reg_addr, int num_read) {
      // SINGLE-BYTE READ
      uint8_t regs[] = {reg_addr};
      int ret = i2c_write_read_dt(&dev_i2c, regs, 1, &data, num_read);
      if (ret != 0){
            printk("%d: Failed to write/read I2C device address %x at reg. %x \n\r", ret, dev_i2c.addr, reg_addr);
      }
      return data;
}

/* SWITCH SETUP */
static const struct gpio_dt_spec drv_switch = GPIO_DT_SPEC_GET_OR(DT_NODELABEL(drv_switch), gpios, {0});

void switch_init() {
      if (!gpio_is_ready_dt(&drv_switch)) {
            printk("The switch pin GPIO port is not ready!\n\r");
            return;
      }
      uint8_t err = gpio_pin_configure_dt(&drv_switch, GPIO_OUTPUT_INACTIVE);
      if (err != 0) {
            printk("Configuring GPIO pin to inactive output failed.");
      }
}

// level should be either 0 (LOW/OFF) or 1 (HIGH/ON)
void switch_set(uint8_t level) {
      uint8_t err = gpio_pin_set_dt(&drv_switch, level);
      if (err != 0) {
            printk("Setting GPIO pin level failed.");
      }
}

/**
 * @brief
 *
 * @param myMotor
 * @param open_loop Set to true if you want to configure for open-loop mode
 */
void drv2625_init(struct motor* myMotor) {
      uint8_t* data = read_transfer(CHIPID_REG, 1);
      printk("Chip ID (should be 1): %x \n", data[0]);
}
