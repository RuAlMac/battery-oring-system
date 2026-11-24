#ifndef PINS_H
#define PINS_H

/******************<Summary>*****************
 * Name: pins.h
 * Purpose:
 *      > Defines pins used for this project
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



/******************<LEDs>*****************/
#define LED_G_PIN                   9
#define LED_Y_PIN                   10
#define LED_R_PIN                   11
/******************</LEDs>*****************/



/******************<Buzzer PWM>*****************/
#define NUM_BUZZERS                 2

#define BUZZER_CONTROL_1_PIN        22
#define BUZZER_CONTROL_2_PIN        20

#define BUZZER_1_PWM_CLKDIV 		8
#define BUZZER_1_PWM_WRAP		    4999        // 300Hz,   f(PWM) = f(SYS) / (clkdiv * (wrap + 1)),     f(SYS) = 12 MHz

#define BUZZER_2_PWM_CLKDIV 		8
#define BUZZER_2_PWM_WRAP		    4999
/******************</Buzzer PWM>*****************/



/******************<Battery Monitor IC>*****************/
#define BATT_MON_I2C_BUS                i2c0
#define BATT_MON_I2C_BAUDRATE           100 * 1000      // init at 100kHz
#define BQ7692006PWR_I2C_ADDR           0x18            // different addresses based on part number

#define SDA0_PIN                        0
#define SCL0_PIN                        1

#define BATT_MON_ALERT_PIN              2
#define BATT_MON_RESET_PIN              3

#define BATT_MON_REGOUT_SENSE           27              // REGOUT is connected to GP27
#define BATT_MON_REGOUT_ADC_CH          1               // GP27 is ADC CH1
/******************</Battery Monitor IC>*****************/



/******************<ORing>*****************/
#define ORING_EN_PIN                    23
/******************</ORing>*****************/



/******************<Off-Board Peripherals>*****************/
#define NEOPIXEL_DATA_PIN               19
/******************</Off-Board Peripherals>*****************/



#endif // PINS_H