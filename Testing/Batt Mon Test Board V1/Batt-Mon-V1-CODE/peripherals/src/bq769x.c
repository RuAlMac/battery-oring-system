/******************<Dependencies>*****************/
#include "peripherals/bq769.h"
/******************</Dependencies>*****************/



/******************<NTC Thermistor Values>*****************/
static const ntc_references ntc_vals[] = {
    {3.3208f ,  0},
    {2.5842f ,  5},
    {2.0238f , 10},
    {1.5858f , 15},
    {1.2507f , 20},
    {1.0000f , 25},
    {0.7964f , 30},
    {0.64053f, 35},
    {0.51772f, 40},
    {0.41958f, 45},
    {0.34172f, 50},
    {0.27877f, 55},
    {0.22861f, 60},
    {0.18872f, 65},
    {0.15645f, 70}
};

const size_t ntc_vals_size = sizeof(ntc_vals)/sizeof(ntc_vals[0]);
/******************</NTC Thermistor Values>*****************/



/******************<BQ769x I2C Helpers>*****************/
/*******bq769_read*******
 * Description
        > reads data from a register on a BM769x device
 * Arguments
        > i2c_inst_t* i2c_bus: I2C bus to use
        > uint8_t addr: slave device address (for BM769x)
        > uint8_t reg: register addr from which to read
        > uint8_t* data: pointer to data array
 * Returns
        > N/A
*/
void bq769_read(i2c_inst_t* i2c_bus, uint8_t addr, uint8_t reg, uint8_t* data){

    int ret;
    
    // write register address to read from, generate stop bit
        ret = i2c_write_blocking(i2c_bus, addr, &reg, 1, false);
        if (ret < 0){printf("bm769_read(): write error [%i]\n", ret); return;}

    // read one byte of data from device, generate stop bit
        ret = i2c_read_blocking(i2c_bus, addr, data, 1, false);
        if (ret < 0){printf("bm769_read(): read error [%i]\n", ret); return;}

    return;
}


/*******bq769_write*******
 * Description
        > writes data to a register on a BM769x device
 * Arguments
        > i2c_inst_t* i2c_bus: I2C bus to use
        > uint8_t addr: slave device address (for BM769x)
        > uint8_t reg: register addr to which to write
        > uint8_t data: data to write
 * Returns
        > N/A
*/
void bq769_write(i2c_inst_t* i2c_bus, uint8_t addr, uint8_t reg, uint8_t data){

    // write data to targeted device and register
    uint8_t buf[2] = {reg, data};
    int ret = i2c_write_blocking(i2c_bus, addr, buf, 2, false);
    if (ret < 0){printf("bq769_write(): write error [%i]\n", ret);}

    return;
}
/******************</BQ769x I2C Helpers>*****************/




/******************<Helpers for Initialization>*****************/
/*******bq769_get_adc_gain*******
 * Description
        > reads ADCGAIN1 and ADCGAIN2 registers and gets the
            ADC's current gain
        > ADCGAIN1[3..2] stores gain[4..3]
        > ADCGAIN2[7..5] stores gain[2..0]
 * Arguments
        > i2c_inst_t* i2c_bus: I2C bus to use
        > uint8_t addr: slave device address (for BM769x)
        > float* result: where to store raw result
 * Returns
        > N/A
*/
void bq769_get_adc_gain(i2c_inst_t* i2c_bus, uint8_t addr, float* result){
    // get ADCGAIN1 and ADCGAIN2
        uint8_t adcgain1_val = 0;
        uint8_t adcgain2_val = 0;

        bq769_read(i2c_bus, addr, ADCGAIN1, &adcgain1_val);
        bq769_read(i2c_bus, addr, ADCGAIN2, &adcgain2_val);

    // extract ADCGAIN1[3..2] to get gain[4..3]
        uint8_t upper = (adcgain1_val >> 2) & 0x03;

    // extract ADCGAIN2[7..5] to get gain[2..0]
        uint8_t lower = (adcgain2_val >> 5) & 0x07;

    // combine to get gain[4..0]
        uint8_t raw_gain = (upper << 3) | lower;

    // apply formula from datasheet and print value
        float gain_val = 365.0f + (float)raw_gain;
        printf("bq769_get_adc_gain(): gain = %f\n", gain_val);

    // store result
        *result = gain_val;
}

/*******bq769_get_adc_offset*******
 * Description
        > reads ADCOFFSET and gets current ADC offset in mV
        > ADCOFFSET[7..0] stores offset[7..0]
 * Arguments
        > i2c_inst_t* i2c_bus: I2C bus to use
        > uint8_t addr: slave device address (for BM769x)
        > float* result: where to store raw result
 * Returns
        > N/A
*/
void bq769_get_adc_offset(i2c_inst_t* i2c_bus, uint8_t addr, float* result){
    // get ADCOFFSET
        uint8_t raw_offset = 0;
        bq769_read(i2c_bus, addr, ADCOFFSET, &raw_offset);

    // convert to float, print value
        float adc_offset = (float)(int8_t)(raw_offset);
        printf("bq769_get_adc_offset(): offset = %.1f mV\n", adc_offset);

    // store result
        *result = adc_offset;
}
/******************</Helpers for Initialization>*****************/



/******************<Helpers for Normal Operation>*****************/
/*******bq769_enable_TEMP_SEL*******
 * Description
        > enables NTC thermistor measurements
        > this function must be called during init 
            in order to get valid temperature readings
 * Arguments
        > i2c_inst_t* i2c_bus: I2C bus to use
        > uint8_t addr: slave device address (for BM769x)
 * Returns
        > N/A
*/
void bq769_enable_TEMP_SEL(i2c_inst_t* i2c_bus, uint8_t addr){
    // read SYS_CTRL1
        uint8_t sys_ctrl1_val = 0;
        bq769_read(i2c_bus, addr, SYS_CTRL1, &sys_ctrl1_val);

    // OR current value with TEMP_SEL enable bitmask
        sys_ctrl1_val |= TEMP_SEL_ENABLE_BM;

    // write new value back to SYS_CTRL1
        bq769_write(i2c_bus, addr, SYS_CTRL1, sys_ctrl1_val);
}


/*******bq769_get_raw_cell_voltage*******
 * Description
        > gets the raw voltage measurement for a battery cell
        > CURRENT IMPLEMENTATION: 4 cells, not 5!
 * Arguments
        > i2c_inst_t* i2c_bus: I2C bus to use
        > uint8_t addr: slave device address (for BM769x)
        > uint8_t cell: battery cell whose voltage is to be read
        > uint16_t* result: pointer to variable in which to store result
 * Returns
        > N/A
*/
void bq769_get_raw_cell_voltage(i2c_inst_t* i2c_bus, uint8_t addr, uint8_t cell, uint16_t* result){

    // prepare to read a cell voltage
        uint8_t cell_hi_addr = 0;
        uint8_t cell_lo_addr = 0;

        uint8_t hi = 0;
        uint8_t lo = 0;

    // select appropriate registers to read from
        switch(cell){
            case 1: {
                cell_hi_addr = VC1_HI;
                cell_lo_addr = VC1_LO;
                break;
            }
            case 2: {
                cell_hi_addr = VC2_HI;
                cell_lo_addr = VC2_LO;
                break;
            }
            case 3: {
                cell_hi_addr = VC3_HI;
                cell_lo_addr = VC3_LO;
                break;
            }
            case 4: {
                cell_hi_addr = VC4_HI;
                cell_lo_addr = VC4_LO;
                break;
            }
            case 5: {
                cell_hi_addr = VC5_HI;
                cell_lo_addr = VC5_LO;
                break;
            }
            default: {
                printf("bq769_read_cell_voltage(): invalid cell selection [%u]\n", cell);
            }
        }

    // read cell voltage
        bq769_read(i2c_bus, addr, cell_hi_addr, &hi);
        bq769_read(i2c_bus, addr, cell_lo_addr, &lo);

    // convert result to 16-bit decimal value
        *result = ((uint16_t)(hi & 0x3F) << 8) | lo;        // upper 4 bits come from hi[3..0], lower 8 come from lo

}


/*******bq769_get_raw_battery_voltage*******
 * Description
        > gets the raw voltage measurement for the entire battery
 * Arguments
        > i2c_inst_t* i2c_bus: I2C bus to use
        > uint8_t addr: slave device address (for BM769x)
        > uint16_t* result: pointer to variable in which to store result
 * Returns
        > N/A
*/
void bq769_get_raw_battery_voltage(i2c_inst_t* i2c_bus, uint8_t addr, uint16_t* result){
    // get raw voltage value from appropriate registers
        uint8_t hi = 0;
        uint8_t lo = 0;

        bq769_read(i2c_bus, addr, BAT_HI, &hi);
        bq769_read(i2c_bus, addr, BAT_LO, &lo);

    // convert result to 16-bit decimal value
        uint16_t raw_voltage = ((uint16_t)hi << 8) | lo;

    // store result
        *result = raw_voltage;
}



/*******bq769_get_raw_ntc*******
 * Description
        > gets the raw measurement from the NTC thermistor
        > used to determine temperature
 * Arguments
        > i2c_inst_t* i2c_bus: I2C bus to use
        > uint8_t addr: slave device address (for BM769x)
        > uint16_t* result: pointer to variable in which to store result
 * Returns
        > N/A
*/
void bq769_get_raw_ntc(i2c_inst_t* i2c_bus, uint8_t addr, uint16_t *result){
    // get raw value from appropriate registers
        uint8_t lo = 0;
        uint8_t hi = 0;

        bq769_read(i2c_bus, addr, TS1_HI, &hi);
        bq769_read(i2c_bus, addr, TS1_LO, &lo);

    // convert result to 16-bit decimal value
        uint16_t raw_val = ((uint16_t)(hi & 0x3F) << 8) | lo;   // use TS1_HI[5..0] and TS1_LO[7..0]

    // store result
        *result = raw_val;
}
/******************</Helpers for Normal Operation>*****************/



/******************<Public BQ769x Functions>*****************/

/*******init_bq769*******
 * Description
        > initializes a BQ769x device
        > before calling this function, the user must create an instance
            of the bq769_config struct to store important values used
            throughout normal operation of this device
 * Arguments
        > i2c_inst_t* i2c_bus: I2C bus to use for this device
        > uint8_t addr: I2C address for this device
        > bq769_config *config: pointer to struct instance in
            which to store data for this device
 * Returns
        > N/A
*/
void init_bq769(i2c_inst_t* i2c_bus, uint8_t addr, bq769_config *config){
    // prepare to get ADC gain and offset values
        float adc_gain, adc_offset;

    // get adc gain and offset
        bq769_get_adc_gain(i2c_bus, addr, &adc_gain);
        bq769_get_adc_offset(i2c_bus, addr, &adc_offset);
        
    // write 0x19 to CC_CFG (see pg 35 of datasheet)
        bq769_write(i2c_bus, addr, CC_CFG, 0x19);

    // enable TEMP_SEL to start NTC thermistor measurements
        bq769_enable_TEMP_SEL(i2c_bus, addr);

    // store values in struct
        config->i2c_bus = i2c_bus;
        config->i2c_address = addr;
        config->adc_gain = adc_gain;
        config->adc_offset = adc_offset;
}


/*******bq769_read_register*******
 * Description
        > reads from a register on the current device
 * Arguments
        > bq769_config* config: pointer to this device's config struct
        > uint8_t reg: register to read
        > uint8_t* result: location to store read data
 * Returns
        > N/A
*/
void bq769_read_register(bq769_config* config, uint8_t reg, uint8_t* result){

    // read from i2c bus
        uint8_t data = 0;
        bq769_read(config->i2c_bus, config->i2c_address, reg, &data);

    // save result
        *result = data;
}



/*******bq769_measure_cell*******
 * Description
        > gets the voltage for a single cell
 * Arguments
        > bq769_config* config: pointer to this device's config struct
        > uint8_t cell: which cell's voltage will be read
        > float* result: location to store result in mV
 * Returns
        > N/A
*/
void bq769_measure_cell(bq769_config* config, uint8_t cell, float* result){
    // get raw voltage value for targeted cell
        uint16_t raw_voltage = 0;
        bq769_get_raw_cell_voltage(config->i2c_bus, config->i2c_address, cell, &raw_voltage);

    // compute cell voltage in uV to match units for gain
        float voltage_uv = (config->adc_gain * (float)raw_voltage) + (config->adc_offset * 1000.0f);

    // convert result to mV and V, store result
        float voltage_mv = voltage_uv / 1000.0f;
        float voltage_v = voltage_mv / 1000.0f;

    // print result in V
        printf("[Cell %u]: %f\n", cell, voltage_v);

    // store result in mV
        *result = voltage_mv;
    
}



/*******bq769_measure_battery*******
 * Description
        > gets the voltage for the entire battery
 * Arguments
        > bq769_config* config: pointer to this device's config struct
        > float* result: location to store result in mV
 * Returns
        > N/A
*/
void bq769_measure_battery(bq769_config* config, float* result){
    // get raw voltage value for battery
        uint16_t raw_voltage = 0;
        bq769_get_raw_battery_voltage(config->i2c_bus, config->i2c_address, &raw_voltage);

    // compute battery voltage in uV to match units for gain (see page 21 of datasheet for formula)
        float voltage_uv = 4.0f * (config->adc_gain) * (float)(raw_voltage) + (config->adc_offset * 4.0);

    // convert results to mV and V, store result
        float voltage_mv = voltage_uv / 1000.0f;
        float voltage_v = voltage_mv / 1000.0f;

    // print result in V
        printf("[Battery]: %f\n", voltage_v);

    // store result in mV
        *result = voltage_mv;
}


/*******bq769_get_temperature*******
 * Description
        > gets the voltage for the entire battery
        > see page 20 of the BQ769's datasheet for formulas used
 * Arguments
        > bq769_config* config: pointer to this device's config struct
        > float* result: location to store result in deg C
 * Returns
        > N/A
*/
void bq760_get_temperature(bq769_config* config, float* result){
    // get raw temp val from NTC
        uint16_t val = 0;
        bq769_get_raw_ntc(config->i2c_bus, config->i2c_address, &val);

    // get V_TSX in V
        float v_tsx = ((float)val * 382.0f) * 1e-6f;

    // get R_TS
        float r_ts = (10000.0f * v_tsx) / (3.3f - v_tsx);

    // get ratio = R_TS / R_25, where R_25 = 10k
        float ratio = r_ts / 10000.0f;

    // perform linear interpolation to approximate to values in table
        // set output temp to highest temp in LUT to start
        float output_temp = ntc_vals[ntc_vals_size - 1].temperature;

        // if ratio is larger than largest in LUT, set output temp to be lowest in LUT
        if (ratio > ntc_vals[0].ratio){
            output_temp = ntc_vals[0].temperature;
        }

        // if ratio is smaller than largest in LUT, perform linear interpolation to find best match
        else{
            for (size_t i = 0; i < ntc_vals_size; i++){

                // if the ratio is between two ratios in ntc_vals
                if (ratio <= ntc_vals[i].ratio && ratio >= ntc_vals[i+1].ratio){
                    float ratio_hi = ntc_vals[i].ratio;
                    float temp_hi = (float)ntc_vals[i].temperature;

                    float ratio_lo = ntc_vals[i+1].ratio;
                    float temp_lo = (float)ntc_vals[i+1].temperature;

                    float fraction = (ratio - ratio_hi) / (ratio_lo - ratio_hi);
                    output_temp = temp_hi + fraction * (temp_lo - temp_hi);
                    break;
                }
            }
        }

    // store and print result
        *result = output_temp;
        printf("bq760_get_temperature(): %f\n", output_temp);
}

/******************</Public BQ769x Functions>*****************/
