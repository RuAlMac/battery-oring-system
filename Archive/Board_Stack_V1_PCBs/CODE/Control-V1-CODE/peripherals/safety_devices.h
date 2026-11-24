#ifndef SAFETY_DEVICES_H
#define SAFETY_DEVICES_H

/******************<Summary>*****************
 * Name: safety_devices.h
 * Purpose:
 *      > Functions for onboard LEDs and buzzer
 * Written / updated by:
 *      > Russell
******************</Summary>*****************/



/******************<Dependencies>*****************/
#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"

#include "hardware/pwm.h"
#include "pico/time.h"

#include "pins.h"
/******************</Dependencies>*****************/



/******************<Defines>*****************/
/******************</Defines>*****************/



/******************<Structs>*****************/

/*******buzzer_config*******
 * Description
        > stores info on one buzzer
 * Data
        > uint8_t buzzer_id: ID for this buzzer, should be between 1 and NUM_BUZZERS
        > uint8_t buzzer_pin: GPIO pin that drives MOSFET for this buzzer
        > uint8_t buzzer_pwm_clkdiv: PWM clock div value for this buzzer
        > uint16_t pwm_wrap: pwm wrap value for this buzzer
        > uint8_t pwm_slice: GPIO PWM slice corresponding to this buzzer
        > uint8_t pwm_channel: GPIO PWM channel corresponding to this buzzer (0=A, 1=B)
        > bool state: true=buzzer ON, false=buzzer OFF
*/
typedef struct {
    uint8_t buzzer_id;
    uint8_t buzzer_pin;
    uint8_t pwm_clkdiv;
    uint16_t pwm_wrap;
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
void set_buzzer(buzzer_config* config, bool state, float duty_cycle);
bool start_buzzer_beeping(uint32_t interval_ms);
void get_buzzer_interval(uint32_t* interval_ms);
void stop_buzzer_beeping();

void set_batt_ORing(bool state);
void init_batt_ORing();
/******************</Function Declarations>*****************/

#endif // SAFETY_DEVICES_H
