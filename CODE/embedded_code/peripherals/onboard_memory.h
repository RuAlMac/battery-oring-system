#ifndef ONBOARD_MEMORY_H
#define ONBOARD_MEMORY_H

/******************<Summary>*****************
 * Name: onboard_memory.h
 * Purpose:
 *      > Driver for onboard M93C86 Microwire EEPROM
 *      > Init, read, write, clear, write-enable control
 * Notes:
 *      > ORG pin tied to GND, memory is byte-addressed (NOT word-addressed)
 *      > CS on this device is ACTIVE HIGH
 *      > Microwire frames are NOT byte-aligned, every instruction must be padded
 *          with leading zeros
 *      > Each instruction packet is formatted as 16 bits: 
 *          2 padding zeros, start bit, 2 op-code bits, 11 addr bits
 * Written / updated by:
 *      > Russell
 *      > Claude
******************</Summary>*****************/



/******************<Dependencies>*****************/
#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"

#include "hardware/gpio.h"
#include "hardware/spi.h"

#include "communication/rp2350_spi.h"

#include "pins.h"
/******************</Dependencies>*****************/



/******************<Defines>*****************/
#define EEPROM_SIZE_BYTES               2048        // using x8 addressing
#define EEPROM_ADDR_MASK                0x07FF      // 11-bit addresses (A10-A0)

#define EEPROM_WRITE_TIMEOUT_MS         10          // max time to wait for single byte write
#define EEPROM_ERASE_ALL_TIMEOUT_MS     50          // max time to wait for erase all


// EEPROM instructions
#define EEPROM_CMD_WRITE_DISABLE        0x2000
#define EEPROM_CMD_WRITE_ENABLE         0x2600
#define EEPROM_CMD_ERASE_ALL            0x2400
/******************</Defines>*****************/




/******************<Function Declarations>*****************/
void init_onboard_memory();
void onboard_memory_set_write_enable(bool enable);
bool onboard_memory_write(uint16_t addr, uint8_t* src, uint16_t size);
bool onboard_memory_read(uint16_t addr, uint8_t* dest, uint16_t size);
bool onboard_memory_clear();
/******************</Function Declarations>*****************/

#endif // ONBOARD_MEMORY_H
