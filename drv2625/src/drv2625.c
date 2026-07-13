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
            printk("%d: Failed to write to I2C device address %x at reg. %x \n\r", ret, drv_i2c.addr, buf[0]);
      }
}

uint8_t data[2];
static uint8_t read_transfer(uint8_t reg_addr, int num_read) {
      // SINGLE-BYTE READ
      uint8_t regs[] = {reg_addr};
      int ret = i2c_write_read_dt(&drv_i2c, regs, 1, &data, num_read);
      if (ret != 0){
            printk("%d: Failed to write/read I2C device address %x at reg. %x \n\r", ret, drv_i2c.addr, reg_addr);
      }
      return data;
}

static const struct gpio_dt_spec nrst_spec = GPIO_DT_SPEC_GET(I2C_NODE, nrst_gpios);

static void nrst_setup() {
      if (!gpio_is_ready_dt(&nrst_spec)) {
            printk("NRST GPIO not ready!\n\r");
            return;
      }
      gpio_pin_configure_dt(&nrst_spec, GPIO_OUTPUT_INACTIVE);
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