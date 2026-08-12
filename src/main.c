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
        myMotor.isLRA = 1;

        drv2625_init(&myMotor);
        // turn off after 5 seconds
        // k_msleep(5000);
        // power_down(0);
        
        return 0;
}
