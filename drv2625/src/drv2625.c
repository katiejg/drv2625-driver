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

/* SWITCH SETUP */
static const struct gpio_dt_spec drv_switch = GPIO_DT_SPEC_GET_OR(DT_NODELABEL(drv_switch), gpios, {0});

static void switch_init() {
      if (!gpio_is_ready_dt(&drv_switch)) {
            printk("The switch pin GPIO port is not ready!\n\r");
            return;
      }
      uint8_t err = gpio_pin_configure_dt(&drv_switch, GPIO_OUTPUT_INACTIVE);
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

/* I2C SETUP */
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

/* DRV2625 HELPERS */
static void set_go() { // blocking
      uint8_t buf = read_transfer(GO_REG) | GO_MASK;
      write_transfer(GO_REG, buf);
      // GO automatically clears when process is complete
      // TODO Change to polling, add timeout
      while (read_transfer(GO_REG) & GO_MASK);
}

static void trigger_go() { // nonblocking
      uint8_t buf = read_transfer(GO_REG) | GO_MASK;
      write_transfer(GO_REG, buf);
}

static void set_mode(uint8_t mode) {
      uint8_t buf = read_transfer(MODE_REG) & ~(MODE_MASK);
      buf += mode;
      write_transfer(MODE_REG, buf);
      printk("Set mode (%x)\n", mode);
}

// The status register latches faults and clears on read. Powering the device up
// ramps VDD through the undervoltage threshold, which latches UVLO before any
// process has run. Read once and discard so the first process to check status
// does not inherit a stale fault that has nothing to do with it.
static void clear_status() {
      uint8_t status = read_transfer(DIAG_RESULT_REG);
      printk("Power-on status (discarded): reg01=%02x reg00=%02x\n",
             status, read_transfer(CHIPID_REG));
}

static uint8_t get_diag_result() {
      uint8_t status = read_transfer(DIAG_RESULT_REG);
      uint8_t result = status & DIAG_RESULT_MASK;
      if (result) {
            printk("Process failed (status=0x%02x): UVLO=%d OVER_TEMP=%d OC_DETECT=%d PROCESS_DONE=%d\n",
                   status,
                   (status & UVLO_MASK) ? 1 : 0,
                   (status & OVER_TEMP_MASK) ? 1 : 0,
                   (status & OC_DETECT_MASK) ? 1 : 0,
                   (status & PROCESS_DONE_MASK) ? 1 : 0);
      }
      return result;
}

static uint8_t run_diagnostics() {
      set_mode(MODE_DIAG);
      printk("MODE_REG after set: %02x\n", read_transfer(MODE_REG));
      set_go();  // actually trigger the routine this time

      uint8_t diagZ = read_transfer(DIAG_Z_RESULT_REG);
      printk("DIAG_Z_RESULT: %02x\n", diagZ);

      uint8_t currK = read_transfer(CURRENT_K_REG);
      printk("CURRENT K: %02x\n", currK);

      uint8_t result = read_transfer(DIAG_RESULT_REG) & DIAG_RESULT_MASK;
      if (result) {
            printk("Diagnostics: actuator open, shorted, or invalid BEMF\n");
      }
      return result;
}

/* DRV2625 ROUTINES */
static void set_motor_params(struct motor* motorPtr) {
      // Print motor parameters (debug)
      printk("Rated Voltage: %02i\n", motorPtr->ratedVoltage);
      printk("OD Clamp: %02i\n", motorPtr->odClamp);
      printk("Drive Time: %02i\n", motorPtr->driveTime);
      printk("OL LRA Period: %02i\n", motorPtr->olLRAPeriod);
      printk("LRA?: %02i\n", motorPtr->isLRA);

      uint8_t buf;
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
}

static uint8_t autocalibrate(struct motor* motorPtr) {
      // Pass relevant parameters to auto-calibration engine:
      set_motor_params(motorPtr);

      // Set auto-calibration routine (MODE[1:0] = 0x03)
      set_mode(MODE_CALIBRATION);
      
      // Start auto-calibration process
      set_go();

      // Check auto-calibration results:
      printk("BEMF Gain (default 0x02): %02x\n", (read_transfer(BEMF_GAIN_REG) & BEMF_GAIN_MASK));
      printk("A_CAL_COMP (default 0x0D): %02x\n", read_transfer(A_CAL_COMP_REG));
      printk("A_CAL_BEMF (default 0x6D): %02x\n", read_transfer(A_CAL_BEMF_REG));

      // Check DIAG_RESULT for success
      uint8_t diagnostic = get_diag_result();
      if (diagnostic) {
            printk("Auto-calibration failed\n");
            return diagnostic;
      }

      printk("Auto-calibration complete\n");
      return 0;
}

// Configuration for Closed Loop Architecture
static void closed_loop_config() {
      // Clear CONTROL_LOOP bit
      uint8_t buf = read_transfer(CONTROL_LOOP_REG) & ~(CONTROL_LOOP_MASK);
      write_transfer(CONTROL_LOOP_REG, buf);
      // Set to enable auto-braking
      buf = read_transfer(AUTO_BRK_INTO_STBY_REG) | AUTO_BRK_INTO_STBY_MASK;
      write_transfer(AUTO_BRK_INTO_STBY_REG, buf);
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

      printk("Configured for Open Loop\n");
}

void drv2625_init(struct motor* motorPtr, uint8_t isOpenLoop) {
      // Turn on GPIOs
      switch_init();

      // Power cycle
      switch_set(0);
      k_msleep(50);
      switch_set(1);

      // Required delay for DRV2625 to power on:
      k_msleep(10);
      i2c_ready();
      uint8_t data = read_transfer(CHIPID_REG);
      printk("DRV2625 Chip ID (should be 1): %x \n", ((data & CHIPID_MASK) >> 4));

      clear_status();

      // Allow GO to trigger starts
      data = read_transfer(TRIG_PIN_FUNC_REG) & ~(TRIG_PIN_FUNC_MASK);
      write_transfer(TRIG_PIN_FUNC_REG, data);

      set_motor_params(motorPtr);
      run_diagnostics();

      // Remove device from standby by writing to 0x00 to MODE:
      set_mode(MODE_RTP);
      
      // Autocalibrate for each power-up
      autocalibrate(motorPtr);

      // Select library and configure for open/close loop
      // Lib A = LRA, Closed Loop
      // Lib B = ERM, Open Loop
      // No library support for LRA Open Loop
      if (isOpenLoop) {
            data = read_transfer(LIB_SEL_REG) | (LIB_SEL_MASK);
            write_transfer(LIB_SEL_REG, data);
            open_loop_config(motorPtr->olLRAPeriod);
      } else {
            data = read_transfer(LIB_SEL_REG) & ~(LIB_SEL_MASK);
            write_transfer(LIB_SEL_REG, data);
            closed_loop_config();
      }
}

// Turn off DRV_VDD
void power_down() {
      switch_set(0);
      printk("Haptics driver powered off.\n");
}

/* PLAYBACK MODES */
// For both RTP and WAVEFORM_SEQ
void stop_effect() {
      uint8_t buf = read_transfer(GO_REG) & ~(GO_MASK);
      write_transfer(GO_REG, buf);
      printk("Effect stop\n");
}

// RTP Mode: Write Waveforms to MEM, RTP Params, Trigger
void rtp_drive(uint8_t amplitude) {
      // Write desired drive amplitude:
      write_transfer(RTP_INPUT_REG, amplitude); // signed 8b
      set_mode(MODE_RTP);
      trigger_go();
      // printk("RTP amplitude = %02d\n", amplitude);
}

// WAVEFORM SEQUENCER
static void play_effect() {
      set_mode(MODE_WAVEFORM_SEQ);
      // Non-blocking: playback duration (including infinite loops via
      // WAV_SEQ_MAIN_LOOP = 7) is caller-defined, so don't wait for GO here.
      trigger_go();
      printk("Effect start\n");
}

/**
 * @brief Assumes using an effect from the library
 * 
 * @param effect_id See 9.1.1 Waveform Library Effects List
 * @param main_loop_count See Table 8-27. 0-6 = play sequence 1-7 times;
 *                         7 = loop the sequence forever.
 */
void waveform_sequencer(uint8_t effect_id, uint8_t main_loop_count) {
      // Make sure params are valid:
      if (effect_id > 123 || effect_id < 1) {
            printk("Invalid effect ID\n\r");
            return;
      }
      if (main_loop_count > 7) {
            printk("Invalid loop count number (max. 7)\n\r");
            return;
      }

      // Clear WAIT to indicate SEQ holds a wavefrom identifier
      uint8_t buf = read_transfer(WAV_FRM_SEQ1_REG) & ~(WAITn_MASK);
      write_transfer(WAV_FRM_SEQ1_REG, buf);

      // set_waveform(effect_id);
      // // Populate with ID
      buf = (read_transfer(WAV_FRM_SEQ1_REG) & ~(WAV_FRM_SEQn_MASK)) + effect_id;
      write_transfer(WAV_FRM_SEQ1_REG, buf);
      // Terminate SEQ
      buf = read_transfer(WAV_FRM_SEQ2_REG) & ~(WAV_FRM_SEQn_MASK);
      write_transfer(WAV_FRM_SEQ2_REG, buf);

      // TODO STEP 4: Allow loop control of each sequence. For now, leave WAVn_SEQ_LOOP as default.

      // STEP 5: Set main loop control
      buf = (read_transfer(WAV_SEQ_MAIN_LOOP_REG) & ~(WAV_SEQ_MAIN_LOOP_MASK)) + main_loop_count;
      write_transfer(WAV_SEQ_MAIN_LOOP_REG, buf);

      play_effect();
}
