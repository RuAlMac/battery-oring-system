#ifndef LED_MATRIX_H
#define LED_MATRIX_H

/******************<Summary>*****************
 * Name: led_matrix.h
 * Purpose:
 *      > functions for interacting with 8x8 LED matrix
 * Written / updated by:
 *      > Russell
******************</Summary>*****************/



/******************<Dependencies>*****************/
#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"

#include "pins.h"
#include "hardware/gpio.h"
/******************</Dependencies>*****************/


/******************<Structs>*****************/

/*******led_matrix_config*******
 * Description
        > stores an array that maps to each of the pins used on
            this 8x8 LED matrix
        > simplifies addressing for a specific LED
 * Data
        > const uint8_t row[8]: row pin numbers
        > const uint8_t col[8]: col pin numbers
        > bool pixels[x][y]: is led at (x,y) turned on (true) or off (false)?
*/
typedef struct {
    uint8_t row[8];
    uint8_t col[8];
    bool pixels[8][8];
} led_matrix_config;
/******************</Structs>*****************/




/******************<Defines>*****************/
/*
* Time between vertical scans
* Positive value: time in ms between end of one scan and start of the next
* Negative value: time in ms between start of one scan and start of the next (ignore sign)
*/
#define TIME_BETWEEN_SCANS_MS       -2
/******************</Defines>*****************/



/******************<Functions>*****************/
void init_led_matrix(led_matrix_config* config);
void start_led_matrix(led_matrix_config* config);
void set_matrix_led(led_matrix_config* config, uint8_t x, uint8_t y, bool state);
/******************</Functions>*****************/

#endif // LED_MATRIX_H