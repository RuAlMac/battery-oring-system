/******************<Dependencies>*****************/
#include "peripherals/battery_monitoring.h"
/******************</Dependencies>*****************/



/******************<Public Variables>*****************/
bq769_config batt_mon_config;
/******************</Public Variables>*****************/



/******************<Private Variables>*****************/
static repeating_timer_t status_check_timer;

static uint16_t batt_check_interval = 250;      // time (in ms) between battery checks
static volatile bool check_status = false;      // set by status_check_timer, true=system will now check battery
/******************</Private Variables>*****************/



/******************<Status Check ISR>*****************/
// Executes whenever it is time to check battery status
static bool status_check_timer_callback(repeating_timer_t *rt){

    check_status = true;

    // Do not automatically restart, will be restarted by check_battery_status()
        return false;
}
/******************</Status Check ISR>*****************/



/******************<Helper Functions>*****************/
static uint8_t get_minimum(uint8_t a, uint8_t b, uint8_t c){
    if (a <= b && a <= c){return a;}
    else if (b <= a && b <= c){return b;}
    else {return c;}
}
/******************</Helper Functions>*****************/


/******************<Public Functions>*****************/

/*******check_bq769_booted*******
 * Description
        > checks to see if the BQ769 has booted
            based on voltage measured at REGOUT pin
        > if the BQ769 IC has successfully booted, it
            should output +3V3 from its REGOUT pin
 * Arguments
        > bool* status: true=successful boot, false=unsuccessful boot
 * Returns
        > N/A
*/
void check_bq769_booted(bool* status){
    // measure voltage at ADC pin
        adc_select_input(BATT_MON_REGOUT_ADC_CH);
        uint16_t adc_val = adc_read();
        float adc_voltage = (float)(adc_val) * (ADC_VREF / ADC_RESOLUTION);

    // print status
    
        if (adc_voltage >= 3.0f){
            printf("\t> check_bq769_booted(): SUCCESSFUL BOOT");
            *status = true;
        }
        else {
            printf("\t> check_bq769_booted(): FAILED / UNINITIALIZED");
            *status = false;
        }

        printf("   (adc voltage = %f)\n", adc_voltage);

}



/*******init_batt_mon*******
 * Description
        > initializes the BQ769 IC and its I2C bus
 * Arguments
        > uint32_t check_interval: how often (in ms) battery status should be checked
 * Returns
        > N/A
*/
void init_batt_mon(uint32_t check_interval){
    // init I2C
        init_pico_i2c(BATT_MON_I2C_BUS, SDA0_PIN, SCL0_PIN, BATT_MON_I2C_BAUDRATE, true);

    // initialize relevant GPIO
        gpio_init(BATT_MON_ALERT_PIN);
        gpio_init(BATT_MON_RESET_PIN);

        gpio_set_dir(BATT_MON_ALERT_PIN, GPIO_IN);
        gpio_set_dir(BATT_MON_RESET_PIN, GPIO_OUT);

    // init ADC pin
        adc_init();
        adc_gpio_init(BATT_MON_REGOUT_SENSE);
        adc_select_input(BATT_MON_REGOUT_ADC_CH);

    // pulse reset pin to activate IC
        gpio_put(BATT_MON_RESET_PIN, GPIO_SET_HIGH);
        sleep_ms(100);
        gpio_put(BATT_MON_RESET_PIN, GPIO_SET_LOW);

        sleep_ms(500);

    // initialize battery monitor
        printf("init_batt_mon():\n");
        init_bq769(BATT_MON_I2C_BUS, BQ7692006PWR_I2C_ADDR, &batt_mon_config);

    // wait until successful
        bool boot_status = false;
        uint8_t num_attempts = 1;

        for (int i = 0; i < NUM_BOOT_ATTEMPTS; i++){
            check_bq769_booted(&boot_status);

            if (!boot_status){      // if unsuccessful, wait a second and try again
                sleep_ms(1000);
                num_attempts += 1;
                continue;
            }

            break;
        }

        printf("\t> Num times checked for boot: %u\n", num_attempts);

        if (boot_status = false){
            printf("\t BOOT TERMINATED");
            return;
        }

    // set battery check interval
        batt_check_interval = check_interval;
        if (batt_check_interval < MIN_BATT_CHECK_INTERVAL){batt_check_interval = MIN_BATT_CHECK_INTERVAL;}
        if (batt_check_interval > MAX_BATT_CHECK_INTERVAL){batt_check_interval = MAX_BATT_CHECK_INTERVAL;}

        printf("\t> Batt check interval set to %ums\n", batt_check_interval);

    // start battery checking
        printf("\t> Starting battery check timer...\n");
        start_status_check_timer(batt_check_interval);
}




/*******start_status_check_timer*******
 * Description
        > starts the timer/counter that, when triggered, indicates
            that the system should check on the battery
 * Arguments
        > uint32_t interval_ms: battery status will be checked every 'interval_ms'
 * Returns
        > N/A
*/
void start_status_check_timer(uint32_t interval_ms){
    add_repeating_timer_ms(interval_ms, status_check_timer_callback, NULL, &status_check_timer);
}



/*******stop_status_check_timer*******
 * Description
        > stops the battery status check timer
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void stop_status_check_timer(){
    cancel_repeating_timer(&status_check_timer);
}



/*******check_values*******
 * Description
        > Determines whether or not current battery voltage
            and temp is safe for continued operation
        > Updates the LEDs and buzzers depending on status
 * Arguments
        > float* batt_voltage: pointer to overall battery voltage in V
        > float cell_voltages: cell voltages in V
        > float* temp_c: pointer to current batt tube temperature in deg C
        > uint8_t* status: pointer to store battery status (0=danger, 1=warning, 2=good)
 * Returns
        > N/A
*/
void check_values(float* batt_voltage, float* cell_voltages, float* temp_c){

    // define variables for use throughout function
        uint8_t batt_voltage_status = 2;         // (0=danger, 1=warning, 2=good)
        uint8_t cell_voltage_status = 2;
        uint8_t temp_status = 2;
        uint32_t new_buzzer_interval = 0;

    // check cell voltages
        //tbd

    // check overall battery voltage
        if (*batt_voltage <= MED_BATT_VOLTAGE){batt_voltage_status = 1;}
        if (*batt_voltage <= MIN_BATT_VOLTAGE){batt_voltage_status = 0;}

    // check temperature
        if (*temp_c >= MED_TEMP){temp_status = 1;}
        if (*temp_c >= MAX_TEMP){temp_status = 0;}

    // get worst status
        uint8_t overall_status = get_minimum(batt_voltage_status, cell_voltage_status, temp_status);

    // update LEDs as needed
        if (overall_status == 2){
            set_led_byte(ONLY_G_LED_ON);
            stop_buzzer_beeping();
        }
        else if (overall_status == 1){
            set_led_byte(ONLY_Y_LED_ON);
            new_buzzer_interval = WARN_BUZZER_INTERVAL;
        }
        else {
            set_led_byte(ONLY_R_LED_ON);
            new_buzzer_interval = DANGER_BUZZER_INTERVAL;
        }

    // update buzzer interval as needed
        uint32_t current_buzzer_interval = 0;
        get_buzzer_interval(&current_buzzer_interval);

        if (current_buzzer_interval != new_buzzer_interval){
            stop_buzzer_beeping();
            //start_buzzer_beeping(new_buzzer_interval);
        }

    // print (debug)
        printf("check_values(): overall status = %u\n", overall_status);
        printf("\t> battery voltage: status = %u, voltage = %f\n", batt_voltage_status, batt_voltage);
        printf("\t> temperature: status = %u, temp = %f\n", temp_status, temp_c);
}



/*******check_battery_status*******
 * Description
        > checks on battery status
        > gets battery voltage, cell voltages, and temperature
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void check_battery_status(){
    // check if it is now time to check on the battery
        if (!check_status){
            return;
        }
        else{
            check_status = false;
        }

    // create variables
        float batt_voltage_mv = 0.0f;
        float batt_voltage_v = 0.0f;
        float cell_voltages_mv[4];
        float cell_voltages_v[4];
        float temp_deg_c = 0.0f;

    // read cell voltages
        for (int i = 1; i <= 5; i++){
            bq769_measure_cell(&batt_mon_config, i, &cell_voltages_mv[i]);
            sleep_ms(20);
        }

    // read battery voltage
        bq769_measure_battery(&batt_mon_config, &batt_voltage_mv);
        batt_voltage_v = batt_voltage_mv / 1000.0f;

    // read temperature
        bq760_get_temperature(&batt_mon_config, &temp_deg_c);

    // update safety devices
        check_values(&batt_voltage_v, cell_voltages_v, &temp_deg_c);
        
    // restart timer/counter
        start_status_check_timer(batt_check_interval);
}

/******************</Public Functions>*****************/
