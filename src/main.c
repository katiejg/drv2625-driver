#include <zephyr/kernel.h>

#include <drv2625.h>

int main(void) {
        /* Create motor struct */
        // My Motor = VG0825001U
        struct motor myMotor;
        myMotor.ratedVoltage = 47;
        myMotor.odClamp = 63;
        myMotor.driveTime = 16; // LRA Drive Time = 2.1ms
        myMotor.olLRAPeriod = 169;
        myMotor.isLRA = 1;

        // This motor has brown-out issues during auto-calibration
        // Cannot use Closed Loop
        drv2625_init(&myMotor, 1);
        // turn off after 5 seconds
        // k_msleep(5000);
        // power_down(0);

        while (1) {
                k_sleep(K_FOREVER);
        }
        
        return 0;
}
