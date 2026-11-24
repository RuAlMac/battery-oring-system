#ifndef BQ769_H
#define BQ769_H

/******************<Summary>*****************
 * Name: bq769.h
 * Purpose:
 *      > Communicating with BQ769x Battery Monitor IC
 * Written / updated by:
 *      > Russell
******************</Summary>*****************/


/******************<Dependencies>*****************/
#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"

#include "communication/pico_i2c.h"

#include "pins.h"
/******************</Dependencies>*****************/



/******************<Registers>*****************/
// Note: these are the registers available for the BQ7692006PWR IC. Larger versions of this IC have more registers available.
#define SYS_STAT        0x00
#define CELLBAL1	    0x01
#define SYS_CTRL1	    0x04
#define SYS_CTRL2	    0x05
#define PROTECT1	    0x06
#define PROTECT2	    0x07
#define PROTECT3	    0x08
#define OV_TRIP	        0x09
#define UV_TRIP	        0x0A
#define CC_CFG	        0x0B
#define VC1_HI	        0x0C
#define VC1_LO	        0x0D
#define VC2_HI	        0x0E
#define VC2_LO	        0x0F
#define VC3_HI	        0x10
#define VC3_LO	        0x11
#define VC4_HI	        0x12
#define VC4_LO	        0x13
#define VC5_HI	        0x14
#define VC5_LO	        0x15
#define BAT_HI	        0x2A
#define BAT_LO	        0x2B
#define TS1_HI	        0x2C
#define TS1_LO	        0x2D
#define CC_HI	        0x32
#define CC_LO	        0x33
#define ADCGAIN1	    0x50
#define ADCOFFSET	    0x51
#define ADCGAIN2	    0x59
/******************</Registers>*****************/



/******************<Bit Masks>*****************/
#define TEMP_SEL_ENABLE_BM      0b00001000
/******************</Bit Masks>*****************/


/******************<Structs>*****************/

/*******bq769_config*******
 * Description
        > stores values that are used frequently during
            normal operation
        > adc_gain and adc_offset are both required for
            calculating the voltage of a battery cell
 * Data
        > i2c_inst_t* i2c_bus: the I2C bus that this device is connected to
        > uint8_t i2c_address: the address of this device (provided in datasheet)
        > float adc_gain: current ADC gain in uV/LSB
        > float adc_offset: current ADC offset in mV
*/
typedef struct {
    i2c_inst_t* i2c_bus;
    uint8_t i2c_address;
    float adc_gain;
    float adc_offset;
} bq769_config;


/*******ntc_references*******
 * Description
        > stores values to be used to interpolate a temperature
            given a resistance from the NTC thermistor
        > NTC thermistor used: B57164K0103J000 (Digikey)
        > See thermistor datasheet, page 5, R/T no 1012
 * Data
        > float ratio: RT / R25 value from datasheet
        > int temperature: the temp (in C) that corresponds to the ratio value
*/
typedef struct {
    float ratio;
    int temperature;
} ntc_references;
/******************</Structs>*****************/



/******************<Functions>*****************/
void init_bq769(i2c_inst_t* i2c_bus, uint8_t addr, bq769_config *config);
void bq769_measure_cell(bq769_config* config, uint8_t cell, float* result);
void bq769_measure_battery(bq769_config* config, float* result);
void bq769_read_register(bq769_config* config, uint8_t reg, uint8_t* result);
void bq760_get_temperature(bq769_config* config, float* result);
/******************</Functions>*****************/



#endif // BQ769_H