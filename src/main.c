#include <zephyr/kernel.h>

#include <drv2625.h>

int main(void) {
        /* Create motor struct */
        // My Motor = VG0825001U
        struct motor myMotor;
        myMotor.ratedVoltage = 47;
        myMotor.odClamp = 59;
        myMotor.driveTime = 16; // LRA Drive Time = 2.1ms
        myMotor.olLRAPeriod = 169;
        myMotor.isLRA = 1;

        drv2625_init(&myMotor, 0); // closed loop
        // turn off after 5 seconds
        // k_msleep(5000);
        // power_down(0);
        waveform_sequencer(1, 7);
        
        return 0;
}
