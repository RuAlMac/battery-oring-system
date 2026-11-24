#include <stdio.h>
#include "pico/stdlib.h"

#include "pico_pwm.h"

int main()
{
    stdio_init_all();

    // init pwm
    pico_pwm_init();

    // enable both buzzers
    set_buzzer(1, true);
    set_buzzer(2, true);

    while (true) {
        tight_loop_contents();
    }
}
