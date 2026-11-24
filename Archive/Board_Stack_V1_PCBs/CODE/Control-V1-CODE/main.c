#include <stdio.h>
#include "pico/stdlib.h"

#include "peripherals/safety_devices.h"
#include "peripherals/battery_monitoring.h"



int main() {
    stdio_init_all();

    sleep_ms(2000);

    // initialize safety devices
        init_leds();
        init_all_buzzers();
        init_batt_ORing();

        set_all_led(true);

        //start_buzzer_beeping(50);

        sleep_ms(300);
        stop_buzzer_beeping();
        set_all_led(false);

    // initialize battery monitor IC
        init_batt_mon(500);


    while (true) {
        check_battery_status();
    }
}
