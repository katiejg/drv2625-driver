#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <drv2625.h>

int main(void) {
        /* Create motor struct */
        init();
        // float temp = temperature();
        // while (1) {
        //         printk("Temperature in Celsius : %8.2f C\n", temp);
        //         k_msleep(1000);
        // }
        return 0;
}
