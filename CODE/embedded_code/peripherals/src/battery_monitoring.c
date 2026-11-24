/******************<Dependencies>*****************/
#include "battery_monitoring.h"
#include "battery_settings.h"
/******************</Dependencies>*****************/



/******************<Public Variables>*****************/
bq769_config batt_mon_config;
/******************</Public Variables>*****************/



/******************<Private Variables>*****************/
static repeating_timer_t status_check_timer;

static uint16_t batt_check_interval = 250;      // time (in ms) between battery checks
static volatile bool check_status = false;      // set by status_check_timer, true=system will now check battery

static batt_snapshot_t snapshot;                // latest readings + status (zero at boot, have_data=false)
static float ovr_values[BATT_OVR_COUNT];        // override value for each target
static uint8_t ovr_mask = 0;                    // bit n set = override n is active

static bool shutdown_counting = false;          // true = shutdown conditions present, countdown into shutdown is running
static uint32_t shutdown_end_ms = 0;            // time (ms since boot) at which the countdown finishes
static bool shutdown_latched = false;           // true = shutdown reached with shtdwn_latch on, stays until batt_unlatch() / reset
static bool new_sample = false;                 // true = a fresh measurement has not been added to cell_filtered yet
static bool cell_filter_valid = false;          // false until the first measurement initializes cell_filtered
static float cell_filtered[NUM_CELL_SLOTS];     // running average of the effective cell voltages (V)
static bool shutdown_reached = false;           // true = countdown finished (or no delay), system is in shutdown
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

/*******level_for_low_value*******
 * Description
        > classifies a value where LOWER is worse (voltages)
 * Arguments
        > float value: value to classify
        > float warn: value at/below which status is WARNING
        > float danger: value at/below which status is DANGER
        > float shutdown: value at/below which status is SHUTDOWN
 * Returns
        > batt_level_t: severity of the value
*/
static batt_level_t level_for_low_value(float value, float warn, float danger, float shutdown){
    if (value <= shutdown){return BATT_LEVEL_SHUTDOWN;}
    if (value <= danger){return BATT_LEVEL_DANGER;}
    if (value <= warn){return BATT_LEVEL_WARNING;}
    return BATT_LEVEL_GOOD;
}



/*******level_for_high_value*******
 * Description
        > classifies a value where HIGHER is worse (temperature)
 * Arguments
        > float value: value to classify
        > float warn: value at/above which status is WARNING
        > float danger: value at/above which status is DANGER
        > float shutdown: value at/above which status is SHUTDOWN
 * Returns
        > batt_level_t: severity of the value
*/
static batt_level_t level_for_high_value(float value, float warn, float danger, float shutdown){
    if (value >= shutdown){return BATT_LEVEL_SHUTDOWN;}
    if (value >= danger){return BATT_LEVEL_DANGER;}
    if (value >= warn){return BATT_LEVEL_WARNING;}
    return BATT_LEVEL_GOOD;
}



/*******worst_level*******
 * Description
        > returns the most severe of three levels
 * Arguments
        > batt_level_t a, b, c: levels to compare
 * Returns
        > batt_level_t: most severe of a, b, and c
*/
static batt_level_t worst_level(batt_level_t a, batt_level_t b, batt_level_t c){
    batt_level_t worst = a;
    if (b > worst){worst = b;}
    if (c > worst){worst = c;}
    return worst;
}



/*******float_to_digits*******
 * Description
        > converts a float value of format XY.Z to uint8 digits
 * Arguments
        > float f_value: float value to convert
        > uint8_t* uint8_digits: array of uint8_t digits
            [0]: tens place
            [1]: ones place
            [2]: first decimal place
 * Returns
        > N/A
*/
static void float_to_digits(float f_value, uint8_t uint8_digits[3]){
    // ensure only valid floats
        if (f_value < 0.0f)  f_value = 0.0f;
        if (f_value > 99.9f) f_value = 99.9f;

    // scale by 10 and round to avoid float error
        uint16_t scaled = (uint16_t)(f_value * 10.0f + 0.5f);   // 0 - 999

    // return uint8_t values
        uint8_digits[0] = (uint8_t)(scaled / 100);
        uint8_digits[1] = (uint8_t)((scaled / 10) % 10);
        uint8_digits[2] = (uint8_t)(scaled % 10);
    }


/*******update_led_banks*******
 * Description
        > sets the behavior of the two LED banks from the battery / cell voltage status
            GOOD     -> banks off
            WARNING  -> banks blink alternately, every warn_led_interval ms (setting)
            DANGER   -> banks blink alternately, every danger_led_interval ms (setting)
            SHUTDOWN -> (only once the system is really in shutdown, not during the shutdown countdown)
                        banks stop blinking and turn off (LED_BANK_SHUTDOWN_MODE), whichever
                        category (voltage or temperature) caused the shutdown
        > otherwise only the cell and battery voltages count, temperature does not
        > safe to call after every evaluation, the blink phase is only restarted when the mode or
            blink interval changes (so a saved settings change takes effect at the next evaluation)
 * Arguments
        > batt_level_t cell_level: status of the lowest cell voltage
        > batt_level_t batt_level: status of the battery voltage
        > batt_level_t system_level: overall system status (worst of everything)
 * Returns
        > N/A
*/
static void update_led_banks(batt_level_t cell_level, batt_level_t batt_level, batt_level_t system_level){
    batt_level_t voltage_level = (cell_level > batt_level) ? cell_level : batt_level;
    if (system_level == BATT_LEVEL_SHUTDOWN){voltage_level = BATT_LEVEL_SHUTDOWN;}
    else if (voltage_level == BATT_LEVEL_SHUTDOWN){voltage_level = BATT_LEVEL_DANGER;}     // shutdown countdown still running

    switch (voltage_level){
        case BATT_LEVEL_WARNING:  {set_led_bank_mode(LED_BANK_BLINK, (uint32_t)battery_settings_get(SETTING_WARN_LED_INTERVAL)); break;}
        case BATT_LEVEL_DANGER:   {set_led_bank_mode(LED_BANK_BLINK, (uint32_t)battery_settings_get(SETTING_DANGER_LED_INTERVAL)); break;}
        case BATT_LEVEL_SHUTDOWN: {set_led_bank_mode(LED_BANK_SHUTDOWN_MODE, 0); break;}
        case BATT_LEVEL_GOOD:
        default:                  {set_led_bank_mode(LED_BANK_OFF, 0); break;}
    }
}



/*******apply_overrides*******
 * Description
        > builds the effective readings from the real readings and any active overrides
        > if battery voltage is not overridden, it is the sum of the effective cell
            voltages, so overriding a cell also changes the battery voltage
 * Arguments
        > const batt_readings_t* real: real (measured) readings
        > batt_readings_t* effective: output, readings to use for all decisions
 * Returns
        > N/A
*/
static void apply_overrides(const batt_readings_t* real, batt_readings_t* effective){
    *effective = *real;

    // cell overrides
        for (int i = 0; i < NUM_CELL_SLOTS; i++){
            if (ovr_mask & (1u << (BATT_OVR_CELL1 + i))){
                effective->cell_v[i] = ovr_values[BATT_OVR_CELL1 + i];
            }
        }

    // battery voltage: override, otherwise sum of effective cells
        if (ovr_mask & (1u << BATT_OVR_BATT)){
            effective->batt_v = ovr_values[BATT_OVR_BATT];
        }
        else {
            effective->batt_v = 0.0f;
            for (int i = 0; i < NUM_CELL_SLOTS; i++){
                if (CELL_SLOT_POPULATED(i)){effective->batt_v += effective->cell_v[i];}
            }
        }

    // temperature override
        if (ovr_mask & (1u << BATT_OVR_TEMP)){
            effective->temp_c = ovr_values[BATT_OVR_TEMP];
        }
}



/*******check_cell_imbalance*******
 * Description
        > checks whether any one populated cell differs from the average of the OTHER populated
            cells by more than cell_diff_max (mV)
        > works on a running average of the effective cell voltages (CELL_FILTER_ALPHA) so
            single noisy samples do not trip it. The average is only updated once per new
            measurement, re-evaluations (overrides / settings) reuse it
        > does nothing (returns false) if cell_diff_check is off
 * Arguments
        > N/A
 * Returns
        > bool: true=a cell is outside the allowed difference
*/
static bool check_cell_imbalance(){
    // update running average once per measurement
        if (new_sample){
            new_sample = false;
            for (int i = 0; i < NUM_CELL_SLOTS; i++){
                float v = snapshot.effective.cell_v[i];
                cell_filtered[i] = cell_filter_valid ? cell_filtered[i] + CELL_FILTER_ALPHA * (v - cell_filtered[i]) : v;
            }
            cell_filter_valid = true;
        }

    if (battery_settings_get(SETTING_CELL_DIFF_CHECK) < 0.5f || !cell_filter_valid){return false;}

    float limit_v = battery_settings_get(SETTING_CELL_DIFF_MAX) / 1000.0f;

    float sum = 0.0f;
    int count = 0;
    for (int i = 0; i < NUM_CELL_SLOTS; i++){
        if (CELL_SLOT_POPULATED(i)){sum += cell_filtered[i]; count++;}
    }
    if (count < 2){return false;}

    for (int i = 0; i < NUM_CELL_SLOTS; i++){
        if (!CELL_SLOT_POPULATED(i)){continue;}
        float others_avg = (sum - cell_filtered[i]) / (float)(count - 1);
        float diff = cell_filtered[i] - others_avg;
        if (diff < 0.0f){diff = -diff;}
        if (diff > limit_v){return true;}
    }
    return false;
}



/*******update_shutdown_countdown*******
 * Description
        > decides whether the system is really in shutdown, taking the shutdown delay setting
            (shtdwn_delay, seconds) into account
            raw level < SHUTDOWN : countdown cancelled, shutdown cleared (system follows the raw level)
            raw level = SHUTDOWN : delay 0        -> shutdown immediately
                                   countdown idle -> countdown starts, system stays at DANGER
                                   countdown done -> shutdown
        > shutdown is only latched if the shtdwn_latch setting is on, otherwise the system leaves
            shutdown when the conditions go away
 * Arguments
        > batt_level_t raw_level: worst level of the cell / battery / temperature categories
 * Returns
        > batt_level_t: level the system should act on (SHUTDOWN only after the countdown)
*/
static batt_level_t update_shutdown_countdown(batt_level_t raw_level){
    uint32_t now = to_ms_since_boot(get_absolute_time());

    // latched shutdown: stays until batt_unlatch() (or the latch setting is turned off)
        if (shutdown_latched && battery_settings_get(SETTING_SHTDWN_LATCH) < 0.5f){shutdown_latched = false;}
        if (shutdown_latched){
            shutdown_counting = false;
            shutdown_reached = true;
            return BATT_LEVEL_SHUTDOWN;
        }

    if (raw_level != BATT_LEVEL_SHUTDOWN){
        shutdown_counting = false;
        shutdown_reached = false;
        return raw_level;
    }

    if (!shutdown_reached){
        uint32_t delay_ms = (uint32_t)(battery_settings_get(SETTING_SHTDWN_DELAY) * 1000.0f + 0.5f);

        if (delay_ms == 0){
            shutdown_counting = false;
            shutdown_reached = true;
        }
        else {
            if (!shutdown_counting){
                shutdown_counting = true;
                shutdown_end_ms = now + delay_ms;
            }

            if ((int32_t)(now - shutdown_end_ms) >= 0){     // wrap-safe compare
                shutdown_counting = false;
                shutdown_reached = true;
            }
        }
    }

    // shutdown reached: remember it if the latch is on
        if (shutdown_reached && battery_settings_get(SETTING_SHTDWN_LATCH) >= 0.5f){shutdown_latched = true;}

    return shutdown_reached ? BATT_LEVEL_SHUTDOWN : BATT_LEVEL_DANGER;
}



/*******set_oring_enable*******
 * Description
        > drives the battery ORing enable pin (active high)
        > driven low only when the system is in shutdown (after the shutdown countdown),
            unless ORING_DISCONNECT_ON_SIMULATED_SHUTDOWN is 0 and an override is active
 * Arguments
        > bool shutdown: true=system is in shutdown
 * Returns
        > N/A
*/
static void set_oring_enable(bool shutdown){
    bool disconnect = shutdown;
    if (!ORING_DISCONNECT_ON_SIMULATED_SHUTDOWN && ovr_mask != 0){disconnect = false;}
    gpio_put(ORING_EN_PIN, !disconnect);
}



/*******evaluate_and_indicate*******
 * Description
        > applies overrides to the latest real readings, classifies cell voltage,
            battery voltage and temperature against the ACTIVE settings, and
            stores the result in the snapshot
        > updates the LEDs, LED banks, buzzers and indication board depending on status
            (SHUTDOWN: only the red onboard LED stays on)
        > handles the shutdown countdown and the battery ORing enable pin
 * Arguments
        > N/A
 * Returns
        > N/A
*/
static void evaluate_and_indicate(){
    // build effective readings
        apply_overrides(&snapshot.real, &snapshot.effective);
        snapshot.ovr_mask = ovr_mask;

    // find lowest populated cell
        float lowest_cell_v = 1000.0f;
        for (int i = 0; i < NUM_CELL_SLOTS; i++){
            if (CELL_SLOT_POPULATED(i) && snapshot.effective.cell_v[i] < lowest_cell_v){
                lowest_cell_v = snapshot.effective.cell_v[i];
            }
        }

    // classify each category using the active settings
        snapshot.cell_level = level_for_low_value(
            lowest_cell_v,
            battery_settings_get(SETTING_WARN_CELL_VOLTAGE),
            battery_settings_get(SETTING_DANGER_CELL_VOLTAGE),
            battery_settings_get(SETTING_SHTDWN_CELL_VOLTAGE));

        snapshot.batt_level = level_for_low_value(
            snapshot.effective.batt_v,
            battery_settings_get(SETTING_WARN_BATT_VOLTAGE),
            battery_settings_get(SETTING_DANGER_BATT_VOLTAGE),
            battery_settings_get(SETTING_SHTDWN_BATT_VOLTAGE));

        snapshot.temp_level = level_for_high_value(
            snapshot.effective.temp_c,
            battery_settings_get(SETTING_WARN_TEMP),
            battery_settings_get(SETTING_DANGER_TEMP),
            battery_settings_get(SETTING_SHTDWN_TEMP));

    // a cell that differs too much from the others counts as shutdown conditions
        snapshot.cell_imbalance = check_cell_imbalance();
        if (snapshot.cell_imbalance){snapshot.cell_level = BATT_LEVEL_SHUTDOWN;}

    // get worst status, then apply the shutdown delay (stays at DANGER while the countdown runs)
        snapshot.level = update_shutdown_countdown(
            worst_level(snapshot.cell_level, snapshot.batt_level, snapshot.temp_level));
        snapshot.countdown_active = shutdown_counting;
        snapshot.latched = shutdown_latched;

    // battery ORing: disconnected only once the system is really in shutdown
        set_oring_enable(snapshot.level == BATT_LEVEL_SHUTDOWN);

    // update LEDs and buzzers as needed
    //  GOOD:     green LED, buzzers silent
    //  WARNING:  yellow LED, buzzers beep slowly
    //  DANGER:   red LED, buzzers beep quickly
    //  SHUTDOWN: red LED stays on, everything else off (LED banks, buzzers, seven-seg displays)
    //  (while the shutdown countdown runs the level is DANGER)
        if (snapshot.level == BATT_LEVEL_GOOD){
            set_led_byte(ONLY_G_LED_ON);
            stop_buzzer_beeping();
        }
        else if (snapshot.level == BATT_LEVEL_WARNING){
            set_led_byte(ONLY_Y_LED_ON);
            start_buzzer_beeping((uint32_t)battery_settings_get(SETTING_WARN_BUZZER_BEEP_SPEED));
        }
        else if (snapshot.level == BATT_LEVEL_DANGER){
            set_led_byte(ONLY_R_LED_ON);
            start_buzzer_beeping((uint32_t)battery_settings_get(SETTING_DANGER_BUZZER_BEEP_SPEED));
        }
        else {
            set_led_byte(ONLY_R_LED_ON);
            stop_buzzer_beeping();
        }

    // update LED banks (blink speed follows the cell / battery voltage status)
        update_led_banks(snapshot.cell_level, snapshot.batt_level, snapshot.level);

    // update indication board
        if (snapshot.level == BATT_LEVEL_SHUTDOWN){
            sevenseg_set_enabled(false);                            // displays completely off
        }
        else {
            sevenseg_set_enabled(true);

            uint8_t digits[3];
            float_to_digits(snapshot.effective.batt_v, digits);     // convert battery voltage (float) to three uint8_t digits
            sevenseg_write(0, digits[0], false);                    // update seven-seg displays
            sevenseg_write(1, digits[1], true);
            sevenseg_write(2, digits[2], false);
        }

    // record when this was computed
        snapshot.timestamp_ms = to_ms_since_boot(get_absolute_time());
        snapshot.update_count++;
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
        > enables the battery ORing (ORING_EN_PIN high), initializes the LED banks,
            the BQ769 IC and its I2C bus
 * Arguments
        > uint32_t check_interval: how often (in ms) battery status should be checked
 * Returns
        > N/A
*/
void init_batt_mon(uint32_t check_interval){
    // battery ORing enable (active high): connected until a shutdown is reached
        gpio_init(ORING_EN_PIN);
        gpio_set_dir(ORING_EN_PIN, GPIO_OUT);
        gpio_put(ORING_EN_PIN, true);

    // init LED banks (off until the first battery measurement)
        init_led_banks();

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

        if (boot_status == false){
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
        > stores the latest real (measured) readings
        > applies any active overrides, then determines whether the battery
            voltage, cell voltages and temperature are safe for continued
            operation
        > updates the LEDs, buzzers and indication board depending on status
 * Arguments
        > float* cell_voltages_mv: cell voltages in mV, NUM_CELL_SLOTS long (unpopulated cells = 0)
        > float* temp_c: pointer to current batt tube temperature in deg C
 * Returns
        > N/A
*/
void check_values(float* cell_voltages_mv, float* temp_c){
    // store real readings (mV -> V), battery voltage = sum of populated cells
        snapshot.real.batt_v = 0.0f;
        for (int i = 0; i < NUM_CELL_SLOTS; i++){
            float cell_v = cell_voltages_mv[i] / 1000.0f;
            snapshot.real.cell_v[i] = cell_v;
            if (CELL_SLOT_POPULATED(i)){snapshot.real.batt_v += cell_v;}
        }
        snapshot.real.temp_c = *temp_c;

        snapshot.have_data = true;
        snapshot.sample_count++;
        new_sample = true;

    // evaluate and update LEDs, buzzers, indication board
        evaluate_and_indicate();
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
    // finish the shutdown countdown on time, independent of the measurement interval
        batt_countdown_tick();

    // check if it is now time to check on the battery
        if (check_status == false){return;}
        else{check_status = false;}

    // create variables
        float cell_voltages_mv[NUM_CELL_SLOTS] = {0};
        float temp_deg_c = 0.0f;

    // read cell voltages
        for (int i = 1; i <= NUM_CELL_SLOTS; i++){
            if (!CELL_SLOT_POPULATED(i - 1)){      // ignoring cell 4 for a 4S battery (not 5S)
                cell_voltages_mv[i - 1] = 0;
                continue;
            }

            bq769_measure_cell(&batt_mon_config, i, &cell_voltages_mv[i-1]);
            sleep_ms(20);
        }

    // read temperature
        bq760_get_temperature(&batt_mon_config, &temp_deg_c);

    // update LEDs, buzzers, indication board
        check_values(cell_voltages_mv, &temp_deg_c);

    // restart timer/counter
        start_status_check_timer(batt_check_interval);
}



/*******batt_countdown_tick*******
 * Description
        > re-evaluates the status when the shutdown countdown has run out so the system
            enters shutdown on time (not at the next battery measurement)
        > cheap, called continuously from check_battery_status()
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void batt_countdown_tick(){
    if (!shutdown_counting){return;}

    if ((int32_t)(to_ms_since_boot(get_absolute_time()) - shutdown_end_ms) >= 0){
        evaluate_and_indicate();
    }
}



/*******batt_unlatch*******
 * Description
        > overrides the shutdown latch once: the system leaves the latched shutdown and re-checks
            the battery. If shutdown conditions are still present the shutdown countdown starts
            again (or the system goes straight back into shutdown if there is no delay)
        > does nothing if the system is not latched
 * Arguments
        > N/A
 * Returns
        > bool: true=system was latched and is now unlatched, false=nothing to do
*/
bool batt_unlatch(){
    if (!shutdown_latched){return false;}

    shutdown_latched = false;
    shutdown_reached = false;
    shutdown_counting = false;

    if (snapshot.have_data){evaluate_and_indicate();}
    return true;
}



/*******batt_get_snapshot*******
 * Description
        > copies the latest readings and status
        > countdown_ms is the live time remaining in the shutdown countdown
 * Arguments
        > batt_snapshot_t* out: location to store the snapshot
 * Returns
        > N/A
*/
void batt_get_snapshot(batt_snapshot_t* out){
    *out = snapshot;

    out->countdown_ms = 0;
    if (shutdown_counting){
        int32_t left = (int32_t)(shutdown_end_ms - to_ms_since_boot(get_absolute_time()));
        out->countdown_ms = (left > 0) ? (uint32_t)left : 0;
    }
    out->countdown_active = shutdown_counting;
}



/*******batt_reevaluate*******
 * Description
        > recomputes status from the latest real readings, using the current
            overrides and ACTIVE settings, and updates LEDs/buzzers/display
        > call after overrides or settings change so the system reacts now
            instead of at the next measurement
        > does nothing until the first measurement has completed
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void batt_reevaluate(){
    if (!snapshot.have_data){return;}
    evaluate_and_indicate();
}



/*******batt_set_check_interval*******
 * Description
        > changes how often the battery is measured
        > value is clamped to MIN_BATT_CHECK_INTERVAL..MAX_BATT_CHECK_INTERVAL
        > takes effect immediately (restarts the running timer)
 * Arguments
        > uint32_t interval_ms: new time between battery checks
 * Returns
        > uint32_t: interval that was actually applied (after clamping)
*/
uint32_t batt_set_check_interval(uint32_t interval_ms){
    if (interval_ms < MIN_BATT_CHECK_INTERVAL){interval_ms = MIN_BATT_CHECK_INTERVAL;}
    if (interval_ms > MAX_BATT_CHECK_INTERVAL){interval_ms = MAX_BATT_CHECK_INTERVAL;}

    batt_check_interval = (uint16_t)interval_ms;

    // if a timer is counting down, restart it with the new interval
    // (if a check is already pending, check_battery_status() will restart it itself)
        if (!check_status && cancel_repeating_timer(&status_check_timer)){
            start_status_check_timer(batt_check_interval);
        }

    return batt_check_interval;
}



/*******batt_get_check_interval*******
 * Description
        > gets the current time between battery checks
 * Arguments
        > N/A
 * Returns
        > uint32_t: interval in ms
*/
uint32_t batt_get_check_interval(){
    return batt_check_interval;
}



/*******batt_override_set*******
 * Description
        > overrides a measured value with a simulated one (debug use)
        > the simulated value is used for all decisions (LEDs, buzzers, display,
            status) instead of the real measurement
        > takes effect immediately
 * Arguments
        > batt_ovr_target_t target: what to override (cell 1-5, battery voltage, temperature)
        > float value: simulated value (V for voltages, deg C for temperature)
 * Returns
        > bool: true=override applied, false=invalid target, unpopulated cell,
            or value outside OVR_* sanity limits (NaN included)
*/
bool batt_override_set(batt_ovr_target_t target, float value){
    float lo, hi;

    if ((int)target < 0 || target >= BATT_OVR_COUNT){return false;}

    if (target <= BATT_OVR_CELL5){
        if (!CELL_SLOT_POPULATED(target - BATT_OVR_CELL1)){return false;}
        lo = OVR_CELL_MIN_V;
        hi = OVR_CELL_MAX_V;
    }
    else if (target == BATT_OVR_BATT){
        lo = OVR_BATT_MIN_V;
        hi = OVR_BATT_MAX_V;
    }
    else {
        lo = OVR_TEMP_MIN_C;
        hi = OVR_TEMP_MAX_C;
    }

    if (!(value >= lo && value <= hi)){return false;}

    ovr_values[target] = value;
    ovr_mask |= (uint8_t)(1u << target);

    batt_reevaluate();
    return true;
}



/*******batt_override_clear*******
 * Description
        > removes one override, that value goes back to the real measurement
 * Arguments
        > batt_ovr_target_t target: override to remove
 * Returns
        > N/A
*/
void batt_override_clear(batt_ovr_target_t target){
    if ((int)target < 0 || target >= BATT_OVR_COUNT){return;}

    ovr_mask &= (uint8_t)~(1u << target);
    batt_reevaluate();
}



/*******batt_override_clear_all*******
 * Description
        > removes every override
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void batt_override_clear_all(){
    if (ovr_mask == 0){return;}

    ovr_mask = 0;
    batt_reevaluate();
}



/*******batt_override_get_mask*******
 * Description
        > gets which overrides are active
 * Arguments
        > N/A
 * Returns
        > uint8_t: bit n set = override n (batt_ovr_target_t) is active
*/
uint8_t batt_override_get_mask(){
    return ovr_mask;
}



/*******batt_override_get_value*******
 * Description
        > gets the value of an active override
 * Arguments
        > batt_ovr_target_t target: override to look up
        > float* value: location to store the override value
 * Returns
        > bool: true=override is active and value was stored, false=not active
*/
bool batt_override_get_value(batt_ovr_target_t target, float* value){
    if ((int)target < 0 || target >= BATT_OVR_COUNT){return false;}
    if (!(ovr_mask & (1u << target))){return false;}

    *value = ovr_values[target];
    return true;
}

/******************</Public Functions>*****************/
