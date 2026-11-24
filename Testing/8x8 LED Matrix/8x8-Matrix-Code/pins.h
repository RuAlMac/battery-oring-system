#ifndef PINS_H
#define PINS_H

/******************<Summary>*****************
 * Name: pins.h
 * Purpose:
 *      > Define values for peripheral and GPIO assignment HERE
 * Written / updated by:
 *      > Russell
******************</Summary>*****************/


/******************<Dependencies>*****************/
#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
/******************</Dependencies>*****************/



/******************<General macros>*****************/
#define GPIO_SET_LOW            0           // for use with gpio_put
#define GPIO_SET_HIGH           1           // for use with gpio_put
/******************</General macros>*****************/



/******************<8x8 LED Matrix>*****************/
// Rows (positive)
#define ROW_0_PIN           16
#define ROW_1_PIN           21
#define ROW_2_PIN           7
#define ROW_3_PIN           19
#define ROW_4_PIN           0
#define ROW_5_PIN           6
#define ROW_6_PIN           1
#define ROW_7_PIN           4

// Cols (negative)
#define COL_A_PIN           20
#define COL_B_PIN           2
#define COL_C_PIN           3
#define COL_D_PIN           17
#define COL_E_PIN           5
#define COL_F_PIN           18
#define COL_G_PIN           22
#define COL_H_PIN           26
/******************</8x8 LED Matrix>*****************/



#endif // PINS_H