/**
 * @file warning_system.h
 * @brief Warning / Kill State Machine — Public API
 *
 * Implements the "Don't Kill the Sub Instantly" escalation policy.
 * States progress: NORMAL → WARN1 → WARN2 → WARN3 → KILL
 * The KILL state is latching — requires physical reset to recover.
 */

#ifndef WARNING_SYSTEM_H
#define WARNING_SYSTEM_H

#include "bq76920.h"
#include <stdint.h>

/** BMS warning/kill states in escalation order */
typedef enum {
    BMS_STATE_NORMAL = 0,   /* All clear                              */
    BMS_STATE_WARN1,        /* Red LED solid                          */
    BMS_STATE_WARN2,        /* Red LED solid + buzzer                 */
    BMS_STATE_WARN3,        /* Red LED flashing + buzzer              */
    BMS_STATE_KILL          /* MOSFET open — power disconnected       */
} bms_state_t;

/**
 * Initialize warning system GPIO outputs.
 * Configures LED, buzzer, and MOSFET pins as outputs.
 * MOSFET starts ON (power connected). LED and buzzer start OFF.
 *
 * @param led_pin     GPIO for red warning LED
 * @param buzzer_pin  GPIO for piezo buzzer
 * @param mosfet_pin  GPIO for MOSFET gate (HIGH = power on)
 */
void warning_system_init(uint led_pin, uint buzzer_pin, uint mosfet_pin);

/**
 * Update the state machine with new sensor data.
 * Call this once per main loop iteration (~10 Hz).
 * Evaluates thresholds, applies debounce, manages GPIO outputs,
 * and handles non-blocking LED flashing.
 *
 * @param data  Pointer to the latest sensor reading
 */
void warning_system_update(const bq_sensor_data_t *data);

/**
 * Get the current state of the warning system.
 * @return Current bms_state_t value
 */
bms_state_t warning_system_get_state(void);

/**
 * Get a human-readable string for the current state.
 * Useful for serial debug output and data logging.
 * @return Static string like "NORMAL", "WARN1", etc.
 */
const char *warning_system_state_name(void);

#endif /* WARNING_SYSTEM_H */
