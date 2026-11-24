#include <stdio.h>
#include "pico/stdlib.h"

#include "battery_monitoring.h"
#include "battery_settings.h"
#include "serial_interface.h"
#include "indication_board.h"
#include "onboard_peripherals.h"

#include "pins.h"


int main()
{
    stdio_init_all();

    // initialize LEDS
        init_leds();

    // initialize buzzers
        init_all_buzzers();
        start_buzzer_beeping(50);
        sleep_ms(100);
        stop_buzzer_beeping();

    // initialize indication board
        init_segment_displays();
        sevenseg_write(0, 8, true);
        sevenseg_write(1, 8, true);
        sevenseg_write(2, 8, true);

    // load settings from EEPROM (falls back to defaults)
        init_battery_settings();

    // initialize battery monitor
        init_batt_mon(500);

    // initialize serial command interface
        serial_interface_init();

    // delay briefly for lamp test
        sleep_ms(250);

    while (true) {
        check_battery_status();
        serial_interface_poll();
    }
}
