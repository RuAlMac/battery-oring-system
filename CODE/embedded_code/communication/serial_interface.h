#ifndef SERIAL_INTERFACE_H
#define SERIAL_INTERFACE_H

/******************<Summary>*****************
 * Name: serial_interface.h
 * Purpose:
 *      > Line-based ASCII command interface over USB serial (stdio USB CDC)
 *      > Four modes:
 *          1) view / change / save settings (any mode)
 *          2) DEBUG mode: fast telemetry stream + overrides of cell voltage,
 *              battery voltage, and temperature to simulate conditions
 *          3) NORMAL mode (default at boot): Jetson polls with "status", plus a
 *              slow status heartbeat
 *          4) SILENT mode: nothing is sent automatically so the terminal stays
 *              readable; commands still work, use "status" / "telemetry" for a
 *              single reading. Overrides can be used (and stay active) here too
 *
 * Protocol (one command per line, '\\n' or '\\r' terminated, case-insensitive, max SERIAL_LINE_MAX chars)
 *      > Every command gets exactly one final reply line:
 *          "OK key=value ..."          success
 *          "ERR <code> <message>"      failure, codes: unknown_cmd bad_args bad_id range order eeprom mode nodata overflow state
 *        Exceptions: "status" replies with an S frame (or ERR nodata), "list" prints
 *        SET lines first then its OK line, "help" prints '#' lines first then OK
 *      > Lines the RP2350 can send at any time (parsers should ignore anything else,
 *          e.g. boot messages printed with printf):
 *          "T,..."   telemetry frame (DEBUG mode stream, and reply to "telemetry")
 *          "S,..."   status frame (NORMAL mode heartbeat, and reply to "status"); never sent automatically in SILENT
 *          "EVT ..." automatic event (override timeout, mode change on USB disconnect)
 *          "# ..."   comment
 *
 *      > S frame (all values are the EFFECTIVE values the system is using):
 *          S,t_ms,age_ms,level,batt_v,temp_c,c1,c2,c3,c4,c5,ovr_mask,cd_active,cd_ms,latched,imbalance
 *            t_ms    : ms since boot when frame was sent
 *            age_ms  : how old the underlying battery data is (large = monitor stalled)
 *            level   : 0=good 1=warning 2=danger 3=shutdown
 *            ovr_mask: non-zero means some values are SIMULATED (see below)
 *            cd_active: 1 = shutdown conditions are present and the shutdown countdown
 *                       (shtdwn_delay setting) is running; level stays 2 (danger) meanwhile
 *            cd_ms   : time left (ms) until the system enters shutdown (0 when cd_active=0)
 *            latched : 1 = system is latched in shutdown (shtdwn_latch setting), send "unlatch"
 *                      to let it re-check the battery, otherwise a reset / power cycle is needed
 *            imbalance: 1 = one cell differs too much from the others (counts as a shutdown condition)
 *      > T frame:
 *          T,t_ms,age_ms,seq,level,cell_lvl,batt_lvl,temp_lvl,ovr_mask,unsaved,
 *            batt_v,c1,c2,c3,c4,c5,temp_c,                      <- effective
 *            real_batt_v,rc1,rc2,rc3,rc4,rc5,real_temp_c,       <- measured
 *            cd_active,cd_ms,latched,imbalance                     <- as in the S frame
 *            seq     : increments every time status is recomputed (new data if it changed)
 *            unsaved : 1 if pending settings differ from active settings
 *      > ovr_mask bits: 0-4 = cell 1-5, 5 = battery voltage, 6 = temperature
 *      > Cell 4 is not populated on the 4S battery and always reads 0.
 *
 * Commands (type "help" for the list)
 *      > general:  help, ping, ver, info, mode [normal|silent|debug [stream_ms]], status, telemetry
 *      > settings: list, get <name>, set <name> <value>, validate, save, discard, defaults, reload
 *      > timing:   rate [ms] (how often the battery is measured), stream [ms] (frame period, 0=off)
 *      > shutdown: unlatch (override the shutdown latch once)
 *      > debug:    ovr, ovr cell <1-5> <V>, ovr batt <V>, ovr temp <C>, ovr clear [cell <n>|batt|temp|all]
 *
 * Safety rules for overrides (simulated values can hide a REAL problem)
 *      > overrides only work in DEBUG or SILENT mode (so you can quiet the stream
 *          while a simulated condition is active)
 *      > entering NORMAL mode (also on USB disconnect), or SERIAL_OVERRIDE_TIMEOUT_MS of
 *          silence from the host clears every override. Hosts should send "ping"
 *          periodically while overrides are active.
 *      > S frames report ovr_mask so the Jetson can tell data is simulated
 * Written / updated by:
 *      > Russell
 *      > Claude
******************</Summary>*****************/



/******************<Dependencies>*****************/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"

#include "battery_monitoring.h"
#include "battery_settings.h"
/******************</Dependencies>*****************/



/******************<Defines>*****************/
#define SERIAL_FW_VERSION           "0.1"
#define SERIAL_PROTOCOL_VERSION     3

#define SERIAL_LINE_MAX             96          // max command length (chars) including terminator
#define SERIAL_MAX_ARGS             6           // max words in one command
#define SERIAL_RX_CHARS_PER_POLL    64          // max chars read per serial_interface_poll() call

#define SERIAL_NORMAL_STREAM_MS     1000        // NORMAL mode heartbeat period (0 = off, Jetson polls only)
#define SERIAL_DEBUG_STREAM_MS      100         // DEBUG mode telemetry period
#define SERIAL_STREAM_MIN_MS        20          // fastest allowed stream period
#define SERIAL_STREAM_MAX_MS        10000       // slowest allowed stream period

#define SERIAL_DEBUG_CHECK_MS       MIN_BATT_CHECK_INTERVAL     // battery measurement interval while in DEBUG mode

#define SERIAL_OVERRIDE_TIMEOUT_MS  30000       // overrides cleared after this long with no host input
/******************</Defines>*****************/



/******************<Types>*****************/
typedef enum {
    SERIAL_MODE_NORMAL = 0,
    SERIAL_MODE_DEBUG,
    SERIAL_MODE_SILENT
} serial_mode_t;
/******************</Types>*****************/



/******************<Function Declarations>*****************/
void serial_interface_init();
void serial_interface_poll();
serial_mode_t serial_interface_get_mode();
/******************</Function Declarations>*****************/

#endif // SERIAL_INTERFACE_H
