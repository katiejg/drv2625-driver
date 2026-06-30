#include <zephyr/kernel.h>

#include "drv2625.h"

int main(void) {
        // Part No. VG0825001U
        struct motor myMotor;
        myMotor.ratedVoltage = 52;
        myMotor.odClamp = 41;
        myMotor.driveTime = 16; // LRA Drive Time = 2.1ms
        myMotor.isLRA = true;
        return 0;
}
