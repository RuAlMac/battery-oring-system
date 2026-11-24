#ifndef ONBOARD_PERIPHERALS_H
#define ONBOARD_PERIPHERALS_H

/******************<Summary>*****************
 * Name: onboard_peripherals.h
 * Purpose:
 *      > Functions for onboard LEDs and buzzer
 * Written / updated by:
 *      > Russell
******************</Summary>*****************/



/******************<Dependencies>*****************/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"

#include "hardware/pwm.h"

#include "pins.h"
/******************</Dependencies>*****************/



/******************<Defines>*****************/
// Fixed buzzer tone: f = f_sys / (clkdiv * (wrap + 1)) = 150 MHz / (8 * 5000) = 3750 Hz
// (the tone is fixed, not a setting, so the buzzers are as loud as possible)
#define BUZZER_PWM_CLKDIV           8.0f
#define BUZZER_PWM_WRAP             4999u
#define BUZZER_PWM_LEVEL            ((BUZZER_PWM_WRAP + 1u) / 2u)   // exactly 50% duty cycle
/******************</Defines>*****************/


/******************<Structs>*****************/

/*******buzzer_config*******
 * Description
        > stores info on one buzzer
 * Data
        > uint8_t buzzer_id: ID for this buzzer, should be between 1 and NUM_BUZZERS
        > uint8_t buzzer_pin: GPIO pin that drives MOSFET for this buzzer
        > uint8_t pwm_slice: GPIO PWM slice corresponding to this buzzer
        > uint8_t pwm_channel: GPIO PWM channel corresponding to this buzzer (0=A, 1=B)
        > bool state: true=buzzer ON, false=buzzer OFF
*/
typedef struct {
    uint8_t buzzer_id;
    uint8_t buzzer_pin;
    uint8_t pwm_slice;
    uint8_t pwm_channel;
    
    bool state;
} buzzer_config;
/******************</Structs>*****************/



/******************<Function Declarations>*****************/
void set_led(char led, bool state);
void set_led_byte(uint8_t value);
void set_all_led(bool state);
void init_leds();

void init_buzzer(buzzer_config* config, uint8_t id);
void init_all_buzzers();
void set_buzzer(buzzer_config* config, bool state);
bool start_buzzer_beeping(uint32_t beep_ms);
void get_buzzer_interval(uint32_t* interval_ms);
void stop_buzzer_beeping();
void set_buzzers_enabled(bool enabled);
bool get_buzzers_enabled();
/******************</Function Declarations>*****************/

#endif // ONBOARD_PERIPHERALS_H
