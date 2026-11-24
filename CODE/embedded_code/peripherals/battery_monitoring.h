#ifndef BATTERY_MONITORING_H
#define BATTERY_MONITORING_H

/******************<Summary>*****************
 * Name: battery_monitoring.h
 * Purpose:
 *      > High-level functions that oversee battery monitoring
 *      > Interact with BQ769x library and IC
 *      > Keeps a snapshot of the latest REAL (measured) readings and the
 *          EFFECTIVE readings (real readings with any debug overrides applied)
 *          that the safety logic, LEDs, buzzers, and displays actually use
 * Written / updated by:
 *      > Russell
 *      > Claude
******************</Summary>*****************/



/******************<Dependencies>*****************/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"

#include "hardware/adc.h"

#include "libraries/bq769x/bq769x.h"

#include "indication_board.h"
#include "onboard_peripherals.h"

#include "pins.h"
/******************</Dependencies>*****************/



/******************<Defines>*****************/
#define ADC_VREF				3.3f		// ADC VREF = 3.3V
#define ADC_RESOLUTION			4096.0f		// ADC resolution = 12-bit

#define NUM_BOOT_ATTEMPTS       5           // number of times to check ADC pin after init to check for successful boot

#define MIN_BATT_CHECK_INTERVAL 50          // minimum amount of time between battery checks
#define MAX_BATT_CHECK_INTERVAL 10000       // maximum amount of time between battery checks

#define ONLY_G_LED_ON           0b00000001  // green LED only bitmask
#define ONLY_Y_LED_ON           0b00000010  // yellow LED only bitmask
#define ONLY_R_LED_ON           0b00000100  // red LED only bitmask

#define ORING_DISCONNECT_ON_SIMULATED_SHUTDOWN  1   // 1 = a SIMULATED (overridden) shutdown also drives ORING_EN_PIN low
                                                    // 0 = ORING_EN_PIN is only driven low when no override is active

#define CELL_FILTER_ALPHA       0.5f        // running average weight of a new cell sample (cell imbalance check)

#define NUM_CELL_SLOTS          5           // BQ769 supports 5 cells (cell slots 0-4 = cells 1-5)
#define CELL_SLOT_POPULATED(i)  ((i) != 3)  // 4S battery: cell 4 (slot 3) is not connected

// allowed override values (sanity limits only, deliberately wider than the safety thresholds
// so unsafe conditions can be simulated)
#define OVR_CELL_MIN_V          0.0f
#define OVR_CELL_MAX_V          6.0f
#define OVR_BATT_MIN_V          0.0f
#define OVR_BATT_MAX_V          30.0f
#define OVR_TEMP_MIN_C          -60.0f
#define OVR_TEMP_MAX_C          150.0f
/******************</Defines>*****************/



/******************<Types>*****************/

// severity of a reading, higher = worse
typedef enum {
    BATT_LEVEL_GOOD = 0,
    BATT_LEVEL_WARNING,
    BATT_LEVEL_DANGER,
    BATT_LEVEL_SHUTDOWN
} batt_level_t;

// things that can be overridden (debug only), value = bit position in override mask
typedef enum {
    BATT_OVR_CELL1 = 0,
    BATT_OVR_CELL2,
    BATT_OVR_CELL3,
    BATT_OVR_CELL4,
    BATT_OVR_CELL5,
    BATT_OVR_BATT,
    BATT_OVR_TEMP,
    BATT_OVR_COUNT
} batt_ovr_target_t;

typedef struct {
    float cell_v[NUM_CELL_SLOTS];   // cell voltages (V), unpopulated cells = 0
    float batt_v;                   // total battery voltage (V)
    float temp_c;                   // battery temperature (deg C)
} batt_readings_t;

/*******batt_snapshot_t*******
 * Description
        > most recent battery readings and the status derived from them
 * Data
        > batt_readings_t real: latest values measured from the BQ769
        > batt_readings_t effective: real values with overrides applied (used for all decisions)
        > batt_level_t level: system status. Worst of cell_level, batt_level, temp_level, except that
            SHUTDOWN conditions are reported as DANGER while the shutdown countdown is running
        > bool countdown_active: true while shutdown conditions are present and the shutdown delay
            (shtdwn_delay setting) has not finished
        > bool latched: true = system is latched in shutdown (shtdwn_latch setting), it stays in
            shutdown until batt_unlatch() or a reset / power cycle
        > bool cell_imbalance: true = one cell differs too much from the others (cell_diff_* settings),
            counted as a SHUTDOWN condition in cell_level
        > uint32_t countdown_ms: time left (ms) until the system enters shutdown, only valid
            while countdown_active (filled in by batt_get_snapshot())
        > batt_level_t cell_level, batt_level, temp_level: status of each category
        > uint8_t ovr_mask: bit n set = override n (batt_ovr_target_t) active
        > uint32_t sample_count: number of real measurements taken
        > uint32_t update_count: number of times the status was recomputed
            (measurement OR override/settings change)
        > uint32_t timestamp_ms: time (ms since boot) of last recompute
        > bool have_data: false until the first measurement completes
*/
typedef struct {
    batt_readings_t real;
    batt_readings_t effective;

    batt_level_t level;
    bool countdown_active;
    bool latched;
    bool cell_imbalance;
    uint32_t countdown_ms;
    batt_level_t cell_level;
    batt_level_t batt_level;
    batt_level_t temp_level;

    uint8_t ovr_mask;
    uint32_t sample_count;
    uint32_t update_count;
    uint32_t timestamp_ms;
    bool have_data;
} batt_snapshot_t;
/******************</Types>*****************/



/******************<Function Declarations>*****************/
void check_bq769_booted(bool* status);
void init_batt_mon(uint32_t check_interval);
void start_status_check_timer(uint32_t interval_ms);
void stop_status_check_timer();

void check_values(float* cell_voltages_mv, float* temp_c);
void check_battery_status();
void batt_countdown_tick();
bool batt_unlatch();

// snapshot / re-evaluation
void batt_get_snapshot(batt_snapshot_t* out);
void batt_reevaluate();

// measurement interval
uint32_t batt_set_check_interval(uint32_t interval_ms);
uint32_t batt_get_check_interval();

// debug overrides
bool batt_override_set(batt_ovr_target_t target, float value);
void batt_override_clear(batt_ovr_target_t target);
void batt_override_clear_all();
uint8_t batt_override_get_mask();
bool batt_override_get_value(batt_ovr_target_t target, float* value);
/******************</Function Declarations>*****************/

#endif // BATTERY_MONITORING_H
