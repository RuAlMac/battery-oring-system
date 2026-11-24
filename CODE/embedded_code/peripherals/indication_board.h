#ifndef INDICATION_BOARD_H
#define INDICATION_BOARD_H

/******************<Summary>*****************
 * Name: indication_board.h
 * Purpose:
 *      > Functions for indication board
 * Written / updated by:
 *      > Russell
******************</Summary>*****************/



/******************<Dependencies>*****************/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"

#include "hardware/spi.h"

#include "pins.h"
/******************</Dependencies>*****************/



/******************<Defines>*****************/
#define NUM_SEGMENTS            3               // there are 3 seven-seg displays chained together
#define SEG_DP_BIT              0b10000000      // bit 8 = DP bit

#define NUM_DATA_BITS           8
#define CLK_OUT_POLARITY        SPI_CPOL_0
#define CLK_OUT_PHASE           SPI_CPHA_0

// LED banks (external LEDs, bank 1 = EXTERNAL_LED_1_PIN, bank 2 = EXTERNAL_LED_2_PIN)
// blink speeds are EEPROM settings (warn_led_interval, danger_led_interval)
#define LED_BANK_SHUTDOWN_MODE      LED_BANK_OFF        // SHUTDOWN: banks stop blinking and stay in this mode
                                                        //   (LED_BANK_OFF or LED_BANK_SOLID_ON)
#define LED_BANK_MIN_BLINK_MS       10      // shortest time a bank stays on / off while blinking
/******************</Defines>*****************/



/******************<Structs>*****************/
// behavior of the two LED banks
typedef enum {
    LED_BANK_OFF = 0,           // both banks off
    LED_BANK_BLINK,             // banks alternate: bank 1 on / bank 2 off, then bank 1 off / bank 2 on
    LED_BANK_SOLID_ON           // both banks on, not blinking
} led_bank_mode_t;
/******************</Structs>*****************/



/******************<Function Declarations>*****************/
void init_segment_displays();
void sevenseg_write(uint8_t segment, uint8_t digit, bool dp);
void sevenseg_set_blank(uint8_t segment);
void sevenseg_set_enabled(bool enabled);
bool sevenseg_get_enabled();

void init_led_banks();
void set_led_bank(uint8_t bank, bool state);
void set_led_bank_mode(led_bank_mode_t mode, uint32_t blink_interval_ms);
led_bank_mode_t get_led_bank_mode();
uint32_t get_led_bank_blink_interval();
/******************</Function Declarations>*****************/

#endif // INDICATION_BOARD_H
