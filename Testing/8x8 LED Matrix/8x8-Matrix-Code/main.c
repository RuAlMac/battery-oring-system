#include <stdio.h>
#include "pico/stdlib.h"


#include "peripherals/led_matrix.h"

int main()
{
    stdio_init_all();

    // create instance of LED matrix config struct
    led_matrix_config matrix_config;

    // init LED matrix
    init_led_matrix(&matrix_config);

    // start vertical scanning
    start_led_matrix(&matrix_config);

    // turn on some LEDs
    set_matrix_led(&matrix_config, 1, 1, true);
    set_matrix_led(&matrix_config, 1, 2, true);
    set_matrix_led(&matrix_config, 1, 3, true);
    set_matrix_led(&matrix_config, 1, 4, true);
    set_matrix_led(&matrix_config, 1, 5, true);
    set_matrix_led(&matrix_config, 1, 6, true);
    sleep_ms(250);
    set_matrix_led(&matrix_config, 2, 1, true);
    set_matrix_led(&matrix_config, 3, 1, true);
    set_matrix_led(&matrix_config, 4, 1, true);
    set_matrix_led(&matrix_config, 5, 1, true);
    set_matrix_led(&matrix_config, 6, 1, true);
    sleep_ms(250);

    while (true) {
        tight_loop_contents();
    }
}