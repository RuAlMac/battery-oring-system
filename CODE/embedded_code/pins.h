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
#define LED_G_PIN                   2
#define LED_Y_PIN                   1
#define LED_R_PIN                   0
/******************</LEDs>*****************/



/******************<Buzzer PWM>*****************/
#define NUM_BUZZERS                 2

#define BUZZER_CONTROL_1_PIN        24
#define BUZZER_CONTROL_2_PIN        23

// The buzzer tone / PWM values are fixed in onboard_peripherals.h (BUZZER_PWM_CLKDIV / BUZZER_PWM_WRAP)
/******************</Buzzer PWM>*****************/



/******************<Battery Monitor IC>*****************/
#define BATT_MON_I2C_BUS                i2c0
#define BATT_MON_I2C_BAUDRATE           100 * 1000      // init at 100kHz
#define BQ7692006PWR_I2C_ADDR           0x18            // different addresses based on part number

#define SDA0_PIN                        4
#define SCL0_PIN                        5

#define BATT_MON_ALERT_PIN              6
#define BATT_MON_RESET_PIN              7

#define BATT_MON_REGOUT_SENSE           28              // REGOUT is connected to GP28
#define BATT_MON_REGOUT_ADC_CH          2               // GP28 is ADC CH2
/******************</Battery Monitor IC>*****************/



/******************<ORing>*****************/
#define ORING_EN_PIN                    21
/******************</ORing>*****************/



/******************<Indication Board>*****************/
#define SEG_SPI_BUS                     spi0
#define SEG_SPI_BAUDRATE                1000u * 1000u       // 1 MHz

#define SEG_SCK_PIN                     22
#define SEG_MOSI_PIN                    19
#define SEG_MISO_PIN                    20

#define SEG_LE_PIN                      17
#define SEG_OE_PIN                      26

#define EXTERNAL_LED_1_PIN              14
#define EXTERNAL_LED_2_PIN              18
/******************</Indication Board>*****************/



/******************<Onboard Memory>*****************/
#define ONBOARD_MEM_SPI_BUS             spi1
#define ONBOARD_MEM_SPI_BAUDRATE        1000u * 1000u       // 1 MHz

#define ONBOARD_MEM_SCK_PIN             10
#define ONBOARD_MEM_MOSI_PIN            11
#define ONBOARD_MEM_MISO_PIN            8

#define ONBOARD_MEM_CS_PIN              13
/******************</Onboard Memory>*****************/




#endif // PINS_H