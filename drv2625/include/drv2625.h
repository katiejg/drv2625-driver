#ifndef DRV2625_H
#define DRV2625_H

/* INCLUDE */
#include <stdint.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>

/* PINS */
#define I2C_NODE DT_NODELABEL(drv2625)

/* REGISTERS */
#define CHIPID_REG 0x00
#define DIAG_RESULT_REG 0x01
#define DIAG_Z_RESULT_REG 0x03
#define MODE_REG 0x07
#define TRIG_PIN_FUNC_REG 0x07
#define AUTO_BRK_OL_REG 0x08
#define AUTO_BRK_INTO_STBY_REG 0x08
#define CONTROL_LOOP_REG 0x08
#define LRA_ERM_REG 0x08
#define GO_REG 0x0c
#define LIB_SEL_REG 0x0d
#define RTP_INPUT_REG 0x0e
#define A_CAL_COMP_REG 0x21
#define A_CAL_BEMF_REG 0x22
#define BEMF_GAIN_REG 0x23
#define CURRENT_K_REG 0x30

// WAV_FRM_SEQ Regs
#define SEQ_SIZE 8
#define WAV_FRM_SEQ1_REG 0x0f
#define WAV_FRM_SEQ2_REG 0x10
#define WAV_FRM_SEQ3_REG 0x11
#define WAV_FRM_SEQ4_REG 0x12
#define WAV_FRM_SEQ5_REG 0x13
#define WAV_FRM_SEQ6_REG 0x14
#define WAV_FRM_SEQ7_REG 0x15
#define WAV_FRM_SEQ8_REG 0x16
// enum wav_frm_seq {
//         WAV_FRM_SEQ1_REG = 0x0f,
//         WAV_FRM_SEQ2_REG,
//         WAV_FRM_SEQ3_REG,
//         WAV_FRM_SEQ4_REG,
//         WAV_FRM_SEQ5_REG,
//         WAV_FRM_SEQ6_REG,
//         WAV_FRM_SEQ7_REG,
//         WAV_FRM_SEQ8_REG
// };

#define WAV_SEQ_MAIN_LOOP_REG 0x19
#define WAV1_SEQ_LOOP_REG 0x17 // Also WAV2-4
#define WAV5_SEQ_LOOP_REG 0x18 // Also WAV6-8
#define RATED_VOLTAGE_REG 0x1f
#define OD_CLAMP_REG 0x20
#define DRIVE_TIME_REG 0x27
#define OL_LRA_PERIOD_REG_UPPER 0x2e
#define OL_LRA_PERIOD_REG_LOWER 0x2f

/* REGISTER MASKS */
#define CHIPID_MASK 0xf0
#define DIAG_RESULT_MASK 0x80
#define MODE_MASK 0x03
#define TRIG_PIN_FUNC_MASK 0x0c
#define AUTO_BRK_OL_MASK 0x10
#define AUTO_BRK_INTO_STBY_MASK 0x08
#define CONTROL_LOOP_MASK 0x40
#define LRA_ERM_MASK 0x80
#define GO_MASK 0x01
#define LIB_SEL_MASK 0x40
#define WAITn_MASK 0x80
#define WAV_FRM_SEQn_MASK 0x7f
#define WAV_SEQ_MAIN_LOOP_MASK 0x07
#define DRIVE_TIME_MASK 0x1f
#define OL_LRA_PERIOD_MASK_UPPER 0x03
#define BEMF_GAIN_MASK 0x03


/* CONFIG DEFINES */
// enum Mode {
//         MODE_RTP = 0x00,
//         MODE_WAVEFORM_SEQ,
//         MODE_DIAG,
//         MODE_CALIBRATION
// };
#define MODE_RTP 0x00
#define MODE_WAVEFORM_SEQ 0x01
#define MODE_DIAG 0x02
#define MODE_CALIBRATION 0x03

/* STRUCTS */
struct motor {
        // OL_LRA_PERIOD[9:0] is the nine least significant bits of olLRAPeriod.
        uint16_t olLRAPeriod; // default=198
        uint8_t ratedVoltage; // Calculated with Eq. 6/7 in DRV2625 datasheet
        uint8_t odClamp; // Calculated with Eq. 8/9 in DRV2625 datasheet
        // DRIVE_TIME[4:0] is the five least significant bits of driveTime.
        uint8_t driveTime; // (default=0x10) See 7.6.1.1 and Table 8-40 in the datasheet
        uint8_t isLRA;
};

struct waveform {
        unsigned char effect;
        unsigned char loop;
};

struct waveform_sequencer {
        struct waveform Waveform[SEQ_SIZE];
};

struct wave_setting {
        unsigned char loop;
        unsigned char interval;
        unsigned char scale;
};

/* PUBLIC FUNCTIONS */
void drv2625_init(struct motor* motorPtr, uint8_t isOpenLoop);
void waveform_sequencer(uint8_t effect_id, uint8_t main_loop_count);
void power_down();

#endif // DRV2625_H