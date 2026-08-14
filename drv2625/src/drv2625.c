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
static uint8_t read_transfer(uint8_t reg_addr) {
      // SINGLE-BYTE READ
      uint8_t regs[] = {reg_addr};
      int ret = i2c_write_read_dt(&dev_i2c, regs, 1, &data, 1);
      if (ret != 0){
            printk("%d: Failed to write/read I2C device address %x at reg. %x \n\r", ret, dev_i2c.addr, reg_addr);
      }
      return data[0];
}

/* SWITCH SETUP */
static const struct gpio_dt_spec drv_switch = GPIO_DT_SPEC_GET_OR(DT_NODELABEL(drv_switch), gpios, {0});

static void switch_init() {
      if (!gpio_is_ready_dt(&drv_switch)) {
            printk("The switch pin GPIO port is not ready!\n\r");
            return;
      }
      uint8_t err = gpio_pin_configure_dt(&drv_switch, GPIO_OUTPUT_ACTIVE);
      if (err != 0) {
            printk("Configuring GPIO pin to output failed.\n");
            return;
      }
      printk("Configured GPIO pin to output\n");
}

// level should be either 0 (LOW/OFF) or 1 (HIGH/ON)
static void switch_set(int level) {
      uint8_t err = gpio_pin_set_dt(&drv_switch, level);
      if (err != 0) {
            printk("Setting GPIO pin level failed\n");
            return;
      }
      printk("Setting GPIO pin level (%i)", level);
}

/* DRV2625 SETUP ROUTINES */
static void autocalibrate(struct motor* motorPtr) {
      // Set auto-calibration routine (MODE[1:0] = 0x03)
      uint8_t buf = read_transfer(MODE_REG) | MODE_MASK;
      write_transfer(MODE_REG, buf);

      // Print motor parameters (debug)
      printk("Rated Voltage: %02i\n", motorPtr->ratedVoltage);
      printk("OD Clamp: %02i\n", motorPtr->odClamp);
      printk("Drive Time: %02i\n", motorPtr->driveTime);
      printk("OL LRA Period: %02i\n", motorPtr->olLRAPeriod);
      printk("is LRA?: %02i\n", motorPtr->isLRA);

      // Pass relevant parameters to auto-calibration engine:
      if (motorPtr->isLRA) {
            buf = read_transfer(LRA_ERM_REG) | LRA_ERM_MASK;
      } else {
            buf = read_transfer(LRA_ERM_REG) & ~(LRA_ERM_MASK);
      }
      write_transfer(LRA_ERM_REG, buf);
      // Configure RATED_VOLTAGE
      write_transfer(RATED_VOLTAGE_REG, motorPtr->ratedVoltage);
      // Configure OD_CLAMP
      write_transfer(OD_CLAMP_REG, motorPtr->odClamp);
      // Configure DRIVE_TIME
      buf = (read_transfer(DRIVE_TIME_REG) & ~(DRIVE_TIME_MASK)) + (motorPtr->driveTime & DRIVE_TIME_MASK);
      write_transfer(DRIVE_TIME_REG, buf);

      // Start auto-calibration process
      buf = read_transfer(GO_REG) | GO_MASK;
      write_transfer(GO_REG, buf);
      // GO automatically clears when process is complete
      while (read_transfer(GO_REG) & GO_MASK);

      // Check DIAG_RESULT for success
      uint8_t diagnostic = read_transfer(DIAG_RESULT_REG) & DIAG_RESULT_MASK;
      if (diagnostic) {
            // DIAG_RESULT is high if a fault is detected
            printk("Failed to auto-calibrate\n\r");
            return;
      }
      printk("Auto-calibration complete\n");
}

// Configuration for Closed Loop Architecture
static void closed_loop_config() {
      // Clear CONTROL_LOOP bit
      uint8_t buf = read_transfer(CONTROL_LOOP_REG) & ~(CONTROL_LOOP_MASK);
      write_transfer(CONTROL_LOOP_REG, buf);
      // Set to enable auto-braking
      buf = read_transfer(AUTO_BRK_INTO_STBY_REG) | AUTO_BRK_INTO_STBY_MASK;
      write_transfer(AUTO_BRK_INTO_STBY_REG, buf);
      // Set to enable OL auto-braking
      buf = read_transfer(AUTO_BRK_OL_REG) | AUTO_BRK_OL_MASK;
      write_transfer(AUTO_BRK_OL_REG, buf);

      printk("Configured for Closed Loop\n");
}

// Configuration for Open Loop Architecture
static void open_loop_config(uint16_t olLRAPeriod) {
      // Set CONTROL_LOOP bit
      uint8_t buf = read_transfer(CONTROL_LOOP_REG) | CONTROL_LOOP_MASK;
      write_transfer(CONTROL_LOOP_REG, buf);
      // Set to enable auto-braking
      buf = read_transfer(AUTO_BRK_INTO_STBY_REG) | AUTO_BRK_INTO_STBY_MASK;
      write_transfer(AUTO_BRK_INTO_STBY_REG, buf);
      // Set to enable OL auto-braking
      buf = read_transfer(AUTO_BRK_OL_REG) | AUTO_BRK_OL_MASK;
      write_transfer(AUTO_BRK_OL_REG, buf);
      // Establish driving frequency for LRA in open loop
      buf = (uint8_t)(olLRAPeriod & ~(0xff00));
      write_transfer(OL_LRA_PERIOD_REG_LOWER, buf);
      buf = (uint8_t)((olLRAPeriod & ~(0xff)) >> 8);
      buf += read_transfer(OL_LRA_PERIOD_REG_UPPER) & ~(OL_LRA_PERIOD_MASK_UPPER);
      write_transfer(OL_LRA_PERIOD_REG_UPPER, buf);

      printk("Configured for Open Loop\n\r");
}

void drv2625_init(struct motor* motorPtr, uint8_t isOpenLoop) {
      // Turn on GPIOs
      switch_init();
      switch_set(1);

      // Required delay for DRV2625 to power on:
      k_msleep(10);
      i2c_ready();
      uint8_t data = read_transfer(CHIPID_REG);
      printk("DRV2625 Chip ID (should be 1): %x \n", ((data & CHIPID_MASK) >> 4));

      // Remove device from standby by writing to 0x00 to MODE:
      data = read_transfer(MODE_REG);
      write_transfer(MODE_REG, (data & ~(MODE_MASK)));

      // Autocalibrate for each power-up
      autocalibrate(motorPtr);

      // Exit autocalibration mode
      data = read_transfer(MODE_REG);
      write_transfer(MODE_REG, (data & ~(MODE_MASK)));

      // Select library and configure for open/close loop
      if (isOpenLoop) {
            data = read_transfer(LIB_SEL_REG) & ~(LIB_SEL_MASK);
            write_transfer(LIB_SEL_REG, data);
            open_loop_config(motorPtr->olLRAPeriod);
      } else {
            data = read_transfer(LIB_SEL_REG) | (LIB_SEL_MASK);
            write_transfer(LIB_SEL_REG, data);
            closed_loop_config();
      }

      // Allow GO to trigger waveform sequencer (clear reg)
      data = read_transfer(TRIG_PIN_FUNC_REG);
      write_transfer(TRIG_PIN_FUNC_REG, (data & ~(TRIG_PIN_FUNC_MASK)));
}

// Turn off DRV_VDD
void power_down() {
      switch_set(0);
      printk("Haptics driver powered off.\n");
}

// TODO: Implement RTP Mode
// TODO: Implement Waveform Sequencer