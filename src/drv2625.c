/**
 * @file drv2625.c
 * @author Katie Jiang
 * @brief DRV2625EVM-MINI Driver for nRF5340dk
 * @version 0.1
 * @date 2026-06-30
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#include "drv2625.h"
#include <zephyr/sys/printk.h>

static const struct i2c_dt_spec drv_i2c = I2C_DT_SPEC_GET(I2C_NODE);

static bool is_ready() {
      if (!device_is_ready(drv_i2c.bus)) {
            printk("I2C bus %s is not ready!\n\r", drv_i2c.bus->name);
            return false;
      }
      return true;
}

/* I2C FUNCTIONS */
static void write_transfer(uint8_t reg_addr, uint8_t data) {
      // SINGLE-BYTE WRITE
      uint8_t buf[2] = {reg_addr, data};
      int ret = i2c_write_dt(&drv_i2c, buf, sizeof(buf));
      if (ret != 0) {
            printk("Failed to write to I2C device address %x at reg. %x \n\r", drv_i2c.addr, buf[0]);
      }
}

static uint8_t read_transfer(uint8_t reg_addr) {
      // SINGLE-BYTE READ
      uint8_t data;
      int ret = i2c_write_read_dt(&dev_i2c, &reg_addr, 1, &data, 1);
      if (ret != 0){
            printk("Failed to write/read I2C device address %x at reg. %x \n\r", dev_i2c.addr, reg_addr);
      }
      return data;
}

static void nrst_setup() {
      dev = device_get_binding(NRST_PORT);
      gpio_pin_configure(dev, NRST_PIN, GPIO_OUTPUT);
}

static void power_on() {
      while (!is_ready()); // wait until ready
      // Call nrst_setup() before calling power_on()
      nrst_setup();
      // Assert NRST (logic high)
      gpio_pin_set(NRST_PORT, NRST_PIN, 1);
      // Remove device from standby:
      // Write MODE[1:0] to 0x00
      uint8_t buf = read_transfer(0x07) & ~(MODE_MASK); // Clear MODE
      write_transfer(0x07, buf);
}

static void autocalibrate() {
      // Set auto-calibration routine:
      // Write MODE[1:0] to 0x03
      uint8_t buf = read_transfer(0x07) | MODE_MASK; // Set MODE
      write_transfer(0x07, buf);
      // Populate input parameters:
      // Set LRA_ERM
      buf = read_transfer(0x08) 
}