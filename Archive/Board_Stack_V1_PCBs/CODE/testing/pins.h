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



/******************<Safety Devices>*****************/
#define LED_G                   9
#define LED_Y                   10
#define LED_R                   11

#define BUZZER_CONTROL          12
/******************</Safety Devices>*****************/



/******************</Battery Monitor IC>*****************/
#define BATT_MON_I2C_BUS                i2c0
#define BATT_MON_I2C_BAUDRATE           100 * 1000      // init at 100kHz
#define BQ7692006PWR_I2C_ADDR           0x18            // different addresses based on part number

#define SDA0_PIN                        0
#define SCL0_PIN                        1

#define BATT_MON_ALERT_PIN              2
#define BATT_MON_RESET_PIN              3
#define BATT_MON_REGOUT_SENSE           27              // REGOUT is connected to GP27 / ADC1
/******************</Battery Monitor IC>*****************/



/******************<CAN-SPI Bridge IC>*****************/
#define CAN_BRIDGE_SPI_BUS              spi0
#define CAN_BRIDGE_SPI_BAUDRATE         8500000         // init at 8.5 MHz, SCK <= 0.85 * (FSYSCLK/2) per MCP2518 datasheet

#define MOSI0_PIN                       19
#define MISO0_PIN                       16
#define SCK0_PIN                        18

#define CAN_BRIDGE_CS_PIN               20
#define CAN_BRIDGE_INT_PIN              17
/******************</CAN-SPI Bridge IC>*****************/



/******************<ORing>*****************/
#define ORING_EN_PIN                    6
/******************</ORing>*****************/



/******************<Off-Board Peripherals>*****************/
#define NEOPIXEL_DATA_PIN               5
#define CURRENT_SENSE                   26
/******************</Off-Board Peripherals>*****************/



#endif // PINS_H