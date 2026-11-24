#ifndef BATTERY_SETTINGS_H
#define BATTERY_SETTINGS_H

/******************<Summary>*****************
 * Name: battery_settings.h
 * Purpose:
 *      > Stores battery safety thresholds in the onboard EEPROM
 * Operation:
 *      > Two RAM copies of settings:
 *          ACTIVE: settings that battery monitoring code uses, always
 *              matches EEPROM contents (or the defaults if EEPROM is invalid)
 *          PENDING: working copy that battery_settings_set_pending() edits,
 *              moved to EEPROM (and made active) only when
 *              battery_settings_save() is called
 * EEPROM Memory Map (settings block starts at address 0)
 *      > 0x0000  4 bytes  magic
 *      > 0x0004  2 bytes  layout version
 *      > 0x0006  2 bytes  number of settings
 *      > 0x0008  4 bytes  each setting (float, little endian), in SETTINGS_TABLE order
 *      > last 2 bytes     CRC16-CCITT over everything before it
 * Written / updated by:
 *      > Claude :(
 *      > Russell
******************</Summary>*****************/



/******************<Dependencies>*****************/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "pico/stdlib.h"

#include "onboard_memory.h"

#include "pins.h"
/******************</Dependencies>*****************/



/******************<Types>*****************/
typedef enum {
    SETTINGS_OK = 0,
    SETTINGS_ERR_BAD_ID,        // [error] setting ID does not exist
    SETTINGS_ERR_RANGE,         // [error] value is NaN or outside setting min/max
    SETTINGS_ERR_ORDER,         // thresholds not in a safe order
    SETTINGS_ERR_EEPROM         // EEPROM write or read-back verify failed
} settings_status_t;

typedef struct {
    const char* name;           // lowercase name for serial commands
    const char* units;          // "V", "C", "ms"
    float default_value;
    float min;
    float max;
} setting_info_t;
/******************</Types>*****************/



/******************<Settings Table>*****************/

/*
 * Description
        > singular definition of every setting
        > uses X macros so that there only needs to be one table
            listing all settings instead of multiple
        > to add a new setting, add a new row to the table
        > NOTE: changing number of rows makes existing EEPROM format
            INVALID, need to save to EEPROM after making changes otherwise
            only the default values defined below will be used
 * Parameters:
        >      id: becomes the enum SETTING_<id>
        >    name: lowercase name for serial commands
        >   units: display units ('V'=volts, 'ms'=milliseconds, 's'=seconds, 'bool'=0/1 flag, 'mV'=millivolts, 'C'=deg C)
        > default: value used when EEPROM is blank/invalid
        >  lo, hi: allowed ranges
*/
#define SETTINGS_TABLE(X) \
    /* id                      name                       units       default     lo          hi  */    \
    X(WARN_CELL_VOLTAGE,       "warn_cell_voltage",       "V",        3.8f,       2.5f,       4.35f  )  \
    X(WARN_BATT_VOLTAGE,       "warn_batt_voltage",       "V",        14.9f,      8.0f,       17.4f  )  \
    X(WARN_TEMP,               "warn_temp",               "C",        45.0f,      -20.0f,     100.0f )  \
    X(WARN_BUZZER_BEEP_SPEED,  "warn_buzzer_beep_speed",  "ms",       400.0f,     20.0f,      5000.0f)  \
    X(WARN_LED_INTERVAL,       "warn_led_interval",       "ms",       1000.0f,    50.0f,      5000.0f)  \
    \
    X(DANGER_CELL_VOLTAGE,     "danger_cell_voltage",     "V",        3.69f,       2.5f,      4.35f  )  \
    X(DANGER_BATT_VOLTAGE,     "danger_batt_voltage",     "V",        14.8f,      8.0f,       17.4f  )  \
    X(DANGER_TEMP,             "danger_temp",             "C",        50.0f,      -20.0f,     100.0f )  \
    X(DANGER_BUZZER_BEEP_SPEED, "danger_buzzer_beep_speed", "ms",     120.0f,     20.0f,      5000.0f)  \
    X(DANGER_LED_INTERVAL,     "danger_led_interval",     "ms",       250.0f,     50.0f,      5000.0f)  \
    \
    X(SHTDWN_CELL_VOLTAGE,     "shtdwn_cell_voltage",     "V",        3.65f,       2.5f,      4.35f  )  \
    X(SHTDWN_BATT_VOLTAGE,     "shtdwn_batt_voltage",     "V",        14.6f,      8.0f,       17.4f  )  \
    X(SHTDWN_TEMP,             "shtdwn_temp",             "C",        60.0f,      -20.0f,     100.0f )  \
    X(SHTDWN_DELAY,            "shtdwn_delay",            "s",        0.0f,       0.0f,       50.0f  )  \
    X(SHTDWN_LATCH,            "shtdwn_latch",            "bool",     0.0f,       0.0f,       1.0f   )  \
    \
    X(CELL_DIFF_CHECK,         "cell_diff_check",         "bool",     1.0f,       0.0f,       1.0f   )  \
    X(CELL_DIFF_MAX,           "cell_diff_max",           "mV",       100.0f,     1.0f,       1000.0f)
    

// index of every setting, generated from SETTINGS_TABLE
typedef enum{
    #define X(id, name, units, def, lo, hi) SETTING_##id,
        SETTINGS_TABLE(X)
    #undef X
        NUM_SETTINGS
} setting_id_t;

/******************</Settings Table>*****************/



/******************<Function Declarations>*****************/
settings_status_t init_battery_settings();

settings_status_t battery_settings_load();
settings_status_t battery_settings_save();

float battery_settings_get(setting_id_t id);

float battery_settings_get_pending(setting_id_t id);
settings_status_t battery_settings_set_pending(setting_id_t id, float value);
settings_status_t battery_settings_validate_pending();
bool battery_settings_has_unsaved();
void battery_settings_discard_pending();
void battery_settings_load_defaults();

const setting_info_t* battery_settings_get_info(setting_id_t id);
const char* battery_settings_get_name(setting_id_t id);
int battery_settings_find(const char* name);
/******************</Function Declarations>*****************/

#endif // BATTERY_SETTINGS_H
