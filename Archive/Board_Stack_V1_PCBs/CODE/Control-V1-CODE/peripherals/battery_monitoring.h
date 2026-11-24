#ifndef BATTERY_MONITORING_H
#define BATTERY_MONITORING_H

/******************<Summary>*****************
 * Name: battery_monitoring.h
 * Purpose:
 *      > High-level functions that oversee battery monitoring
 *      > Interact with BQ769x library and IC
 * Written / updated by:
 *      > Russell
******************</Summary>*****************/



/******************<Dependencies>*****************/
#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"

#include "hardware/adc.h"

#include "libraries/bq769x/bq769x.h"

#include "peripherals/safety_devices.h"

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
/******************</Defines>*****************/



/******************<Battery Safety Definitions>*****************/

// If these levels are met, system will start sending WARNINGS to Jetson
#define MED_CELL_VOLTAGE        3.8f
#define MED_BATT_VOLTAGE        15.2f
#define MED_TEMP                45.0f

#define WARN_BUZZER_INTERVAL     250        // buzzer will beep this often (ms) when WARNING level reached

// If these levels are met, system will start sending DANGER to Jetson and begin battery shutdown
#define MIN_CELL_VOLTAGE        3.70f       // absolute minimum safe cell voltage
#define MIN_BATT_VOLTAGE        14.8f       // absolute minimum safe battery voltage
#define MAX_TEMP                50.0f       // absolute maximum safe temperature

#define DANGER_BUZZER_INTERVAL  100         // buzzer will beep this often (ms) when WARNING level reached

/******************</Battery Safety Definitions>*****************/



/******************<Function Declarations>*****************/
void check_bq769_booted(bool* status);
void init_batt_mon(uint32_t check_interval);
void start_status_check_timer(uint32_t interval_ms);
void stop_status_check_timer();

void check_values(float* batt_voltage, float* cell_voltages, float* temp_c);
void check_battery_status();
/******************</Function Declarations>*****************/

#endif // BATTERY_MONITORING_H
