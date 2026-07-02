#include <zephyr/kernel.h>

#include <drv2625.h>

int main(void) {
        /* Create motor struct */
        // My Motor = VG0825001U
        struct motor myMotor;
        myMotor.ratedVoltage = 52;
        myMotor.odClamp = 41;
        myMotor.driveTime = 16; // LRA Drive Time = 2.1ms
        myMotor.olLRAPeriod = 169;
        myMotor.isLRA = true;

        drv2625_init(&myMotor, CLOSED_LOOP);
        waveform_sequencer(1, 2);
        
        return 0;
}
