#include <stdio.h>
#include "pico/stdlib.h"

#include "peripherals/safety_devices.h"


int main()
{
    stdio_init_all();

    init_leds();
    //init_buzzer();

    bool state = false;

    while (true) {
        state = !state;
        set_all_led(state);
        sleep_ms(1000);
    }
}
