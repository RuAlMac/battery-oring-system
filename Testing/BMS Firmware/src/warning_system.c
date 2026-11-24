/**
 * @file warning_system.c
 * @brief Warning / Kill State Machine Implementation
 *
 * Core safety logic for the BMS. Evaluates sensor data against
 * configurable thresholds and drives LED, buzzer, and MOSFET outputs.
 *
 * Design decisions:
 *   - Debounce: state transitions require DEBOUNCE_COUNT consecutive
 *     threshold violations to prevent noise-induced false triggers.
 *   - Hysteresis: recovery (downward transition) requires voltage to
 *     exceed the threshold + HYSTERESIS_MV.
 *   - KILL is latching: once triggered, only a hardware reset recovers.
 *   - LED flashing uses free-running timer — fully non-blocking.
 */

#include "warning_system.h"
#include "config.h"
#include "pico/stdlib.h"
#include <stdlib.h>  /* abs() */

/* ======================== MODULE STATE ==================================== */

static bms_state_t s_state        = BMS_STATE_NORMAL;
static bms_state_t s_pending      = BMS_STATE_NORMAL;
static uint8_t     s_debounce_cnt = 0;

static uint s_led_pin;
static uint s_buzzer_pin;
static uint s_mosfet_pin;

/* ======================== STATE NAME TABLE ================================ */

static const char *STATE_NAMES[] = {
    "NORMAL", "WARN1", "WARN2", "WARN3", "KILL"
};

/* ======================== GPIO HELPERS ==================================== */

/** Apply outputs for the current state. Non-blocking LED flash for WARN3/KILL. */
static void apply_outputs(void) {
    uint32_t now = to_ms_since_boot(get_absolute_time());

    switch (s_state) {

    case BMS_STATE_NORMAL:
        gpio_put(s_led_pin,    0);   /* LED off     */
        gpio_put(s_buzzer_pin, 0);   /* Buzzer off  */
        gpio_put(s_mosfet_pin, 1);   /* Power ON    */
        break;

    case BMS_STATE_WARN1:
        gpio_put(s_led_pin,    1);   /* LED solid   */
        gpio_put(s_buzzer_pin, 0);   /* Buzzer off  */
        gpio_put(s_mosfet_pin, 1);   /* Power ON    */
        break;

    case BMS_STATE_WARN2:
        gpio_put(s_led_pin,    1);   /* LED solid   */
        gpio_put(s_buzzer_pin, 1);   /* Buzzer ON   */
        gpio_put(s_mosfet_pin, 1);   /* Power ON    */
        break;

    case BMS_STATE_WARN3: {
        /* Non-blocking LED flash using modulo on free-running timer */
        bool led_on = ((now / (LED_FLASH_PERIOD_MS / 2)) % 2) == 0;
        gpio_put(s_led_pin,    led_on);
        gpio_put(s_buzzer_pin, 1);   /* Buzzer ON   */
        gpio_put(s_mosfet_pin, 1);   /* Power ON    */
        break;
    }

    case BMS_STATE_KILL: {
        /* Fast flash to signal critical state even after disconnect */
        bool led_on = ((now / (LED_FLASH_FAST_MS / 2)) % 2) == 0;
        gpio_put(s_led_pin,    led_on);
        gpio_put(s_buzzer_pin, 1);   /* Buzzer ON   */
        gpio_put(s_mosfet_pin, 0);   /* Power OFF — MOSFET OPEN */
        break;
    }
    }
}

/* ======================== THRESHOLD EVALUATION ============================ */

/**
 * Determine the target state based on raw sensor values.
 * KILL conditions are checked first (highest priority).
 * Escalation (upward) uses raw thresholds.
 * Recovery (downward) requires threshold + hysteresis to be satisfied.
 */
static bms_state_t evaluate_target(const bq_sensor_data_t *data) {
    int32_t v = data->voltage_mv;
    int32_t i = data->current_ma;

    /* --- KILL conditions: always checked, regardless of current state --- */
    if (v < KILL_VOLTAGE_MV || abs((int)i) > KILL_CURRENT_MA) {
        return BMS_STATE_KILL;
    }

    /* --- Escalation (forward): use thresholds directly --- */
    if (v < WARN3_THRESHOLD_MV) return BMS_STATE_WARN3;
    if (v < WARN2_THRESHOLD_MV) return BMS_STATE_WARN2;
    if (v < WARN1_THRESHOLD_MV) return BMS_STATE_WARN1;

    /*
     * --- Recovery (backward): require hysteresis ---
     * Only recover to a lower state if voltage is above
     * the current state's threshold PLUS the hysteresis band.
     */
    switch (s_state) {
    case BMS_STATE_WARN3:
        if (v >= WARN3_THRESHOLD_MV + HYSTERESIS_MV) return BMS_STATE_WARN2;
        return BMS_STATE_WARN3;

    case BMS_STATE_WARN2:
        if (v >= WARN2_THRESHOLD_MV + HYSTERESIS_MV) return BMS_STATE_WARN1;
        return BMS_STATE_WARN2;

    case BMS_STATE_WARN1:
        if (v >= WARN1_THRESHOLD_MV + HYSTERESIS_MV) return BMS_STATE_NORMAL;
        return BMS_STATE_WARN1;

    default:
        return BMS_STATE_NORMAL;
    }
}

/* ======================== PUBLIC API ====================================== */

void warning_system_init(uint led_pin, uint buzzer_pin, uint mosfet_pin) {
    s_led_pin    = led_pin;
    s_buzzer_pin = buzzer_pin;
    s_mosfet_pin = mosfet_pin;

    /* Configure all outputs */
    gpio_init(s_led_pin);
    gpio_set_dir(s_led_pin, GPIO_OUT);
    gpio_put(s_led_pin, 0);

    gpio_init(s_buzzer_pin);
    gpio_set_dir(s_buzzer_pin, GPIO_OUT);
    gpio_put(s_buzzer_pin, 0);

    gpio_init(s_mosfet_pin);
    gpio_set_dir(s_mosfet_pin, GPIO_OUT);
    gpio_put(s_mosfet_pin, 1);         /* Start with power connected */

    s_state        = BMS_STATE_NORMAL;
    s_pending      = BMS_STATE_NORMAL;
    s_debounce_cnt = 0;
}

void warning_system_update(const bq_sensor_data_t *data) {
    /* If already killed, stay killed. KILL is latching. */
    if (s_state == BMS_STATE_KILL) {
        apply_outputs();
        return;
    }

    /* Skip evaluation if sensor data is invalid — hold current state */
    if (!data->valid) {
        apply_outputs();
        return;
    }

    bms_state_t target = evaluate_target(data);

    /*
     * KILL transitions bypass debounce — they are immediate.
     * This is a safety-critical path: we cannot afford to wait
     * 500ms if an overcurrent event is happening.
     */
    if (target == BMS_STATE_KILL) {
        s_state = BMS_STATE_KILL;
        apply_outputs();
        return;
    }

    /*
     * Debounce logic for all other transitions:
     * The target state must persist for DEBOUNCE_COUNT consecutive
     * calls before the actual transition occurs.
     */
    if (target != s_state) {
        if (target == s_pending) {
            s_debounce_cnt++;
            if (s_debounce_cnt >= DEBOUNCE_COUNT) {
                s_state        = target;
                s_pending      = target;
                s_debounce_cnt = 0;
            }
        } else {
            /* New pending target — reset counter */
            s_pending      = target;
            s_debounce_cnt = 1;
        }
    } else {
        /* Stable in current state — reset pending */
        s_pending      = s_state;
        s_debounce_cnt = 0;
    }

    apply_outputs();
}

bms_state_t warning_system_get_state(void) {
    return s_state;
}

const char *warning_system_state_name(void) {
    return STATE_NAMES[s_state];
}
