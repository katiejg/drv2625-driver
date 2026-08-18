#include <zephyr/kernel.h>

#include <drv2625.h>

int main(void) {
        /* Create motor struct */
        // My Motor = VG0825001U
        struct motor myMotor;
        myMotor.ratedVoltage = 47;
        myMotor.odClamp = 83;
        myMotor.driveTime = 16; // LRA Drive Time = 2.1ms
        myMotor.olLRAPeriod = 169;
        myMotor.isLRA = 1;

        // This motor has brown-out issues during auto-calibration
        // Cannot use Closed Loop
        drv2625_init(&myMotor, 1);
        for (int i = 0; i < 300; i++) { // should be ~30 seconds
                rtp_drive(127); // keep it at default
                k_msleep(100);
        }
        stop_effect();
        
        // turn off after 5 seconds
        // k_msleep(5000);
        // power_down(0);

        while (1) {
                k_sleep(K_FOREVER);
        }
        
        return 0;
}
