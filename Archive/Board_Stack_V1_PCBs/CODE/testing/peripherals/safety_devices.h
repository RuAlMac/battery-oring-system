#ifndef SAFETY_DEVICES_H
#define SAFETY_DEVICES_H

/******************<Summary>*****************
 * Name: safety_devices.h
 * Purpose:
 *      > //
 * Written / updated by:
 *      > Russell
******************</Summary>*****************/



/******************<Dependencies>*****************/
#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"

#include "pins.h"
/******************</Dependencies>*****************/



/******************<Defines>*****************/
/******************</Defines>*****************/



/******************<Global variables>*****************/
/******************</Global variables>*****************/



/******************<Function Declarations>*****************/
void set_all_led(bool state);
void set_led(char led, bool state);
void init_leds();

void init_buzzer();
/******************</Function Declarations>*****************/

#endif // SAFETY_DEVICES_H
