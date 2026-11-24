
#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"

#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "pins.h"

void pico_pwm_init();
void set_buzzer(uint8_t target, bool enable);