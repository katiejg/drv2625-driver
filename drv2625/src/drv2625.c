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
            printk("Failed to write to I2C device address %x at reg. %x \n\r", drv_i2c.addr, buf[0]);
      }
}

static uint8_t read_transfer(uint8_t reg_addr) {
      // SINGLE-BYTE READ
      uint8_t data;
      int ret = i2c_write_read_dt(&drv_i2c, &reg_addr, 1, &data, 1);
      if (ret != 0){
            printk("Failed to write/read I2C device address %x at reg. %x \n\r", drv_i2c.addr, reg_addr);
      }
      return data;
}

static void nrst_setup() {
      int dev = device_get_binding(NRST_PORT);
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
      uint8_t buf = read_transfer(MODE_REG) & ~(MODE_MASK); // Clear MODE
      write_transfer(MODE_REG, buf);
}

static void go_trigger() {
      // Set GO bit
      uint8_t buf = read_transfer(GO_REG) | GO_MASK;
      write_transfer(GO_REG, buf);
      // GO automatically clears when process is complete
}

/**
 * @brief Auto-Calibration Routine
 * @note Uses default values for the following calibration engine inputs: 
 * SAMPLE_TIME, FB_BRAKE_FACTOR, AUTO_CAL_TIME, LOOP_GAIN, BLANKING_TIME,
 * IDISS_TIME, and ZC_DET_TIME
 * @param myMotor (struct motor*)
 */
static void autocalibrate(struct motor* myMotor) {
      // STEP 1: Set auto-calibration routine:
      // Write MODE[1:0] to 0x03
      uint8_t buf = read_transfer(MODE_REG) | MODE_MASK; // Set MODE
      write_transfer(MODE_REG, buf);

      // STEP 2: Populate input parameters:
      // Configure LRA_ERM
      if (myMotor->isLRA) { // Set LRA_ERM
            buf = read_transfer(LRA_ERM_REG) | LRA_ERM_MASK;
      } else { // Clear LRA_ERM
            buf = read_transfer(LRA_ERM_REG) & ~(LRA_ERM_MASK);
      }
      write_transfer(LRA_ERM_REG, buf);
      // Configure RATED_VOLTAGE
      write_transfer(RATED_VOLTAGE_REG, myMotor->ratedVoltage);
      // Configure OD_CLAMP
      write_transfer(OD_CLAMP_REG, myMotor->odClamp);
      // Configure DRIVE_TIME
      buf = (read_transfer(DRIVE_TIME_REG) & ~(DRIVE_TIME_MASK)) + (myMotor->driveTime & DRIVE_TIME_MASK);
      write_transfer(DRIVE_TIME_REG, buf);

      // STEP 3: Start auto-calibration process
      go_trigger();

      // STEP 4: Check DIAG_RESULT for success
      uint8_t diagnostic = read_transfer(DIAG_RESULT_REG) & DIAG_RESULT_MASK;
      if (diagnostic) {
            // DIAG_RESULT is high if a fault is detected
            printk("Failed to auto-calibrate \n\r");
      }
      
      // STEP 5: Evaluate system performance
}

static void closed_loop_config() {
      // Clear CONTROL_LOOP bit
      uint8_t buf = read_transfer(CONTROL_LOOP_REG) & ~(CONTROL_LOOP_MASK);
      write_transfer(CONTROL_LOOP_REG, buf);
      // Set to enable auto-braking
      buf = read_transfer(AUTO_BRK_INTO_STBY_REG) | AUTO_BRK_INTO_STBY_MASK;
      write_transfer(AUTO_BRK_INTO_STBY_REG, buf);
}

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
}

// TODO
static void rtp_mode() {
      // Select RTP mode operation (Clear MODE)
      // uint8_t buf = read_transfer(MODE_REG) & ~(MODE_MASK);
      // write_transfer(MODE_REG, buf);
      // TODO Write desired drive amplitude (signed)
      // TODO Trigger the waveform...
      return;
}

/**
 * @brief Assumes using an effect from the library
 * 
 * @param effect_id See 9.1.1 Waveform Library Effects List
 * @param main_loop_count See Table 8-27
 */
void waveform_sequencer(uint8_t effect_id, uint8_t main_loop_count) {
      // Make sure params are valid:
      if (effect_id > 123 || effect_id < 1) {
            return;
      }
      if (main_loop_count > 7) {
            return;
      }

      // STEP 1: Set MODE to 1 to select Waveform Sequencer
      uint8_t buf = (read_transfer(MODE_REG) & ~(MODE_MASK)) + 0x01;
      write_transfer(MODE_REG, buf);

      // STEP 3: Clear WAIT to indicate SEQ holds a wavefrom identifier
      buf = read_transfer(WAV_FRM_SEQ1_REG) & ~(WAITn_MASK);
      write_transfer(WAV_FRM_SEQ1_REG, buf);
      // Populate with ID
      buf = (read_transfer(WAV_FRM_SEQ1_REG) & ~(WAV_FRM_SEQn_MASK)) + effect_id;
      write_transfer(WAV_FRM_SEQ1_REG, buf);
      // Terminate SEQ
      buf = read_transfer(WAV_FRM_SEQ2_REG) & ~(WAV_FRM_SEQn_MASK);
      write_transfer(WAV_FRM_SEQ2_REG, buf);

      // TODO STEP 4: Allow loop control of each sequence. For now, leave WAVn_SEQ_LOOP as default.

      // STEP 5: Set main loop control
      buf = read_transfer(WAV_SEQ_MAIN_LOOP_REG) & ~(WAV_SEQ_MAIN_LOOP_MASK) + main_loop_count;
      write_transfer(WAV_SEQ_MAIN_LOOP_REG, buf);

      // STEP 6: Trigger waveform with GO bit
      go_trigger();
}

/**
 * @brief 
 * 
 * @param myMotor
 * @param open_loop Set to true if you want to configure for open-loop mode
 */
void drv2625_init(struct motor* myMotor, enum Loop loop_type) {
      power_on();
      autocalibrate(myMotor);

      // Closed or open loop?
      if (loop_type == OPEN_LOOP) {
            // if using ROM library B (ERM Open-Loop)
            uint8_t buf = read_transfer(LIB_SEL_REG) | LIB_SEL_MASK;
            write_transfer(LIB_SEL_REG, buf);
            open_loop_config(myMotor->olLRAPeriod);
      } else {
            // if using ROM library A (LRA Closed-Loop)
            uint8_t buf = read_transfer(LIB_SEL_REG) & ~(LIB_SEL_MASK);
            write_transfer(LIB_SEL_REG, buf);
            closed_loop_config();
      }

      // // Playback mode?
      // if (mode == WAVEFORM_SEQUENCER) {
      //       waveform_sequencer(waveform_id);
      // } 
      // // else {
      // //       rtp_mode();
      // // }
}