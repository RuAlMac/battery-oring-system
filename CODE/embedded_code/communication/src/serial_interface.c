/******************<Dependencies>*****************/
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#include "pico/stdio_usb.h"

#include "serial_interface.h"
/******************</Dependencies>*****************/



/******************<Private Types>*****************/
typedef void (*serial_cmd_handler_t)(int argc, char* argv[]);

typedef struct {
    const char* name;               // command word
    serial_cmd_handler_t handler;   // function that runs the command
    const char* help;               // usage / description shown by "help"
} serial_cmd_t;
/******************</Private Types>*****************/



/******************<Private Variables>*****************/
static const char* const level_names[] = {"good", "warning", "danger", "shutdown"};

static serial_mode_t mode = SERIAL_MODE_NORMAL;

static char line_buf[SERIAL_LINE_MAX];
static size_t line_len = 0;
static bool line_overflow = false;

static uint32_t stream_ms = SERIAL_NORMAL_STREAM_MS;    // time between S/T frames, 0 = off
static uint32_t next_stream_ms = 0;                     // time next frame is due
static uint32_t last_activity_ms = 0;                   // last time a character arrived from host
static uint32_t normal_check_ms = 250;                  // battery check interval to restore when returning to NORMAL
static bool fast_rate = false;                          // true while the fast DEBUG measurement interval is in use
/******************</Private Variables>*****************/



/******************<Forward Declarations>*****************/
static void cmd_help(int argc, char* argv[]);
/******************</Forward Declarations>*****************/



/******************<Helper Functions>*****************/

/*******now_ms*******
 * Description
        > gets milliseconds since boot
 * Arguments
        > N/A
 * Returns
        > uint32_t: milliseconds since boot
*/
static uint32_t now_ms(){
    return to_ms_since_boot(get_absolute_time());
}



/*******reply_ok*******
 * Description
        > sends a success reply line: "OK <formatted text>"
 * Arguments
        > const char* fmt: printf-style format for the text after OK (may be empty)
        > ...: values for fmt
 * Returns
        > N/A
*/
static void reply_ok(const char* fmt, ...){
    va_list args;
    va_start(args, fmt);

    printf("OK");
    if (fmt != NULL && fmt[0] != '\0'){
        printf(" ");
        vprintf(fmt, args);
    }
    printf("\n");

    va_end(args);
}



/*******reply_err*******
 * Description
        > sends a failure reply line: "ERR <code> <formatted text>"
 * Arguments
        > const char* code: short machine-readable error code
        > const char* fmt: printf-style format for the message (may be empty)
        > ...: values for fmt
 * Returns
        > N/A
*/
static void reply_err(const char* code, const char* fmt, ...){
    va_list args;
    va_start(args, fmt);

    printf("ERR %s", code);
    if (fmt != NULL && fmt[0] != '\0'){
        printf(" ");
        vprintf(fmt, args);
    }
    printf("\n");

    va_end(args);
}



/*******status_to_code*******
 * Description
        > converts a settings status into the error code used in ERR replies
 * Arguments
        > settings_status_t status: status to convert
 * Returns
        > const char*: "ok", "bad_id", "range", "order", or "eeprom"
*/
static const char* status_to_code(settings_status_t status){
    switch (status){
        case SETTINGS_OK:           {return "ok";}
        case SETTINGS_ERR_BAD_ID:   {return "bad_id";}
        case SETTINGS_ERR_RANGE:    {return "range";}
        case SETTINGS_ERR_ORDER:    {return "order";}
        case SETTINGS_ERR_EEPROM:   {return "eeprom";}
        default:                    {return "eeprom";}
    }
}



/*******mode_name*******
 * Description
        > gets the text name of a mode
 * Arguments
        > serial_mode_t m: mode
 * Returns
        > const char*: "normal", "debug", or "silent"
*/
static const char* mode_name(serial_mode_t m){
    if (m == SERIAL_MODE_DEBUG){return "debug";}
    if (m == SERIAL_MODE_SILENT){return "silent";}
    return "normal";
}



/*******format_value*******
 * Description
        > formats a setting value as text (whole numbers for ms, 3 decimals otherwise)
 * Arguments
        > char* buf: output buffer
        > size_t size: size of buf
        > float value: value to format
        > const setting_info_t* info: setting the value belongs to
 * Returns
        > N/A
*/
static void format_value(char* buf, size_t size, float value, const setting_info_t* info){
    if (strcmp(info->units, "ms") == 0 || strcmp(info->units, "Hz") == 0 || strcmp(info->units, "s") == 0 || strcmp(info->units, "mV") == 0 || strcmp(info->units, "bool") == 0){snprintf(buf, size, "%.0f", (double)value);}
    else {snprintf(buf, size, "%.3f", (double)value);}
}



/*******parse_float*******
 * Description
        > converts text to a finite float (rejects nan, inf, and trailing junk)
 * Arguments
        > const char* text: text to convert
        > float* out: location to store result
 * Returns
        > bool: true=valid number, false=invalid
*/
static bool parse_float(const char* text, float* out){
    char* end;
    float value = strtof(text, &end);

    if (end == text || *end != '\0' || !isfinite(value)){return false;}

    *out = value;
    return true;
}



/*******parse_uint*******
 * Description
        > converts text to an unsigned integer (rejects negatives and trailing junk)
 * Arguments
        > const char* text: text to convert
        > uint32_t* out: location to store result
 * Returns
        > bool: true=valid number, false=invalid
*/
static bool parse_uint(const char* text, uint32_t* out){
    char* end;

    if (text[0] == '\0' || text[0] == '-'){return false;}

    unsigned long value = strtoul(text, &end, 10);
    if (*end != '\0' || value > 0xFFFFFFFFul){return false;}

    *out = (uint32_t)value;
    return true;
}



/*******valid_stream_ms*******
 * Description
        > checks that a stream period is allowed
 * Arguments
        > uint32_t value: period in ms
 * Returns
        > bool: true=0 (off) or within SERIAL_STREAM_MIN_MS..SERIAL_STREAM_MAX_MS
*/
static bool valid_stream_ms(uint32_t value){
    return (value == 0) || (value >= SERIAL_STREAM_MIN_MS && value <= SERIAL_STREAM_MAX_MS);
}



/*******tokenize*******
 * Description
        > splits a command line into lowercase words (in place)
 * Arguments
        > char* line: command line, modified
        > char* argv[]: output array of word pointers
        > int max_args: size of argv
 * Returns
        > int: number of words, or -1 if there are more than max_args
*/
static int tokenize(char* line, char* argv[], int max_args){
    int argc = 0;
    char* p = line;

    while (*p != '\0'){
        while (*p == ' ' || *p == '\t'){p++;}
        if (*p == '\0'){break;}

        if (argc >= max_args){return -1;}
        argv[argc++] = p;

        while (*p != '\0' && *p != ' ' && *p != '\t'){
            *p = (char)tolower((unsigned char)*p);
            p++;
        }

        if (*p != '\0'){
            *p = '\0';
            p++;
        }
    }

    return argc;
}



/*******send_status_frame*******
 * Description
        > sends one S frame (see serial_interface.h) with the effective values
 * Arguments
        > const batt_snapshot_t* s: snapshot to send
 * Returns
        > N/A
*/
static void send_status_frame(const batt_snapshot_t* s){
    uint32_t now = now_ms();

    printf("S,%lu,%lu,%d,%.3f,%.2f,%.3f,%.3f,%.3f,%.3f,%.3f,%u,%d,%lu,%d,%d\n",
        (unsigned long)now,
        (unsigned long)(now - s->timestamp_ms),
        (int)s->level,
        (double)s->effective.batt_v,
        (double)s->effective.temp_c,
        (double)s->effective.cell_v[0], (double)s->effective.cell_v[1], (double)s->effective.cell_v[2],
        (double)s->effective.cell_v[3], (double)s->effective.cell_v[4],
        (unsigned)s->ovr_mask,
        s->countdown_active ? 1 : 0,
        (unsigned long)(s->countdown_active ? s->countdown_ms : 0),
        s->latched ? 1 : 0,
        s->cell_imbalance ? 1 : 0);
}



/*******send_telemetry_frame*******
 * Description
        > sends one T frame (see serial_interface.h) with effective AND real values
 * Arguments
        > const batt_snapshot_t* s: snapshot to send
 * Returns
        > N/A
*/
static void send_telemetry_frame(const batt_snapshot_t* s){
    uint32_t now = now_ms();

    printf("T,%lu,%lu,%lu,%d,%d,%d,%d,%u,%d,",
        (unsigned long)now,
        (unsigned long)(now - s->timestamp_ms),
        (unsigned long)s->update_count,
        (int)s->level, (int)s->cell_level, (int)s->batt_level, (int)s->temp_level,
        (unsigned)s->ovr_mask,
        battery_settings_has_unsaved() ? 1 : 0);

    printf("%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.2f,",
        (double)s->effective.batt_v,
        (double)s->effective.cell_v[0], (double)s->effective.cell_v[1], (double)s->effective.cell_v[2],
        (double)s->effective.cell_v[3], (double)s->effective.cell_v[4],
        (double)s->effective.temp_c);

    printf("%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.2f,%d,%lu,%d,%d\n",
        (double)s->real.batt_v,
        (double)s->real.cell_v[0], (double)s->real.cell_v[1], (double)s->real.cell_v[2],
        (double)s->real.cell_v[3], (double)s->real.cell_v[4],
        (double)s->real.temp_c,
        s->countdown_active ? 1 : 0,
        (unsigned long)(s->countdown_active ? s->countdown_ms : 0),
        s->latched ? 1 : 0,
        s->cell_imbalance ? 1 : 0);
}



/*******enter_mode*******
 * Description
        > switches between NORMAL, DEBUG, and SILENT mode
        > entering NORMAL clears all overrides and restores the normal battery
            measurement interval
        > entering DEBUG speeds up battery measurement (if not already fast) and
            starts the telemetry stream
        > entering SILENT stops all automatic S/T frames; commands still work and
            overrides stay active, the measurement interval is left as it was
 * Arguments
        > serial_mode_t new_mode: mode to switch to
        > const char* reason: if not NULL, an "EVT mode=..." line is sent (use for
            automatic changes, commands get an OK reply instead)
 * Returns
        > N/A
*/
static void enter_mode(serial_mode_t new_mode, const char* reason){
    if (new_mode == mode){return;}

    if (new_mode == SERIAL_MODE_DEBUG){
        if (!fast_rate){
            normal_check_ms = batt_get_check_interval();
            batt_set_check_interval(SERIAL_DEBUG_CHECK_MS);
            fast_rate = true;
        }
        stream_ms = SERIAL_DEBUG_STREAM_MS;
    }
    else if (new_mode == SERIAL_MODE_SILENT){
        stream_ms = 0;
    }
    else {
        set_buzzers_enabled(true);          // the buzzers can only be muted in debug / silent mode
        batt_override_clear_all();
        if (fast_rate){
            batt_set_check_interval(normal_check_ms);
            fast_rate = false;
        }
        stream_ms = SERIAL_NORMAL_STREAM_MS;
    }

    mode = new_mode;
    next_stream_ms = now_ms();

    if (reason != NULL){printf("EVT mode=%s reason=%s\n", mode_name(mode), reason);}
}
/******************</Helper Functions>*****************/



/******************<Command Handlers>*****************/

/*******cmd_ping*******
 * Description
        > "ping": keep-alive, resets the override timeout
 * Arguments
        > int argc: number of words in command
        > char* argv[]: words in command
 * Returns
        > N/A
*/
static void cmd_ping(int argc, char* argv[]){
    reply_ok("t_ms=%lu", (unsigned long)now_ms());
}



/*******cmd_ver*******
 * Description
        > "ver": firmware and protocol version
 * Arguments
        > int argc: number of words in command
        > char* argv[]: words in command
 * Returns
        > N/A
*/
static void cmd_ver(int argc, char* argv[]){
    reply_ok("fw=%s proto=%d", SERIAL_FW_VERSION, SERIAL_PROTOCOL_VERSION);
}



/*******cmd_info*******
 * Description
        > "info": system summary (mode, timing, overrides, unsaved settings)
 * Arguments
        > int argc: number of words in command
        > char* argv[]: words in command
 * Returns
        > N/A
*/
static void cmd_info(int argc, char* argv[]){
    batt_snapshot_t s;
    batt_get_snapshot(&s);

    reply_ok("mode=%s uptime_ms=%lu check_ms=%lu stream_ms=%lu ovr=0x%02x buzzers=%d unsaved=%d samples=%lu have_data=%d level=%s fw=%s proto=%d",
        mode_name(mode),
        (unsigned long)now_ms(),
        (unsigned long)batt_get_check_interval(),
        (unsigned long)stream_ms,
        (unsigned)batt_override_get_mask(),
        get_buzzers_enabled() ? 1 : 0,
        battery_settings_has_unsaved() ? 1 : 0,
        (unsigned long)s.sample_count,
        s.have_data ? 1 : 0,
        s.have_data ? level_names[s.level] : "none",
        SERIAL_FW_VERSION,
        SERIAL_PROTOCOL_VERSION);
}



/*******cmd_mode*******
 * Description
        > "mode": show mode
        > "mode normal": NORMAL mode (clears overrides)
        > "mode debug [stream_ms]": DEBUG mode with optional telemetry period
        > "mode silent": SILENT mode, nothing is sent unless you ask
            (use "status" / "telemetry" to read data once)
 * Arguments
        > int argc: number of words in command
        > char* argv[]: words in command
 * Returns
        > N/A
*/
static void cmd_mode(int argc, char* argv[]){
    if (argc == 1){
        reply_ok("mode=%s", mode_name(mode));
        return;
    }

    if (strcmp(argv[1], "normal") == 0 && argc == 2){
        enter_mode(SERIAL_MODE_NORMAL, NULL);
        reply_ok("mode=%s stream_ms=%lu", mode_name(mode), (unsigned long)stream_ms);
        return;
    }

    if (strcmp(argv[1], "silent") == 0 && argc == 2){
        enter_mode(SERIAL_MODE_SILENT, NULL);
        reply_ok("mode=%s stream_ms=%lu ovr=0x%02x", mode_name(mode), (unsigned long)stream_ms, (unsigned)batt_override_get_mask());
        return;
    }

    if (strcmp(argv[1], "debug") == 0 && argc <= 3){
        uint32_t period = SERIAL_DEBUG_STREAM_MS;

        if (argc == 3){
            if (!parse_uint(argv[2], &period)){
                reply_err("bad_args", "stream_ms must be a whole number");
                return;
            }
            if (!valid_stream_ms(period)){
                reply_err("range", "stream_ms must be 0 or %d-%d", SERIAL_STREAM_MIN_MS, SERIAL_STREAM_MAX_MS);
                return;
            }
        }

        enter_mode(SERIAL_MODE_DEBUG, NULL);
        stream_ms = period;
        reply_ok("mode=%s stream_ms=%lu check_ms=%lu", mode_name(mode), (unsigned long)stream_ms, (unsigned long)batt_get_check_interval());
        return;
    }

    reply_err("bad_args", "usage: mode [normal|silent|debug [stream_ms]]");
}



/*******cmd_telemetry*******
 * Description
        > "telemetry": replies with one T frame (effective and real values),
            handy in SILENT mode (or ERR nodata before the first measurement)
 * Arguments
        > int argc: number of words in command
        > char* argv[]: words in command
 * Returns
        > N/A
*/
static void cmd_telemetry(int argc, char* argv[]){
    batt_snapshot_t s;
    batt_get_snapshot(&s);

    if (!s.have_data){
        reply_err("nodata", "no measurement yet");
        return;
    }

    send_telemetry_frame(&s);
}



/*******cmd_status*******
 * Description
        > "status": replies with one S frame (or ERR nodata before the first measurement)
 * Arguments
        > int argc: number of words in command
        > char* argv[]: words in command
 * Returns
        > N/A
*/
static void cmd_status(int argc, char* argv[]){
    batt_snapshot_t s;
    batt_get_snapshot(&s);

    if (!s.have_data){
        reply_err("nodata", "no measurement yet");
        return;
    }

    send_status_frame(&s);
}



/*******cmd_list*******
 * Description
        > "list": one SET line per setting, then OK
 * Arguments
        > int argc: number of words in command
        > char* argv[]: words in command
 * Returns
        > N/A
*/
static void cmd_list(int argc, char* argv[]){
    char active[16], pending[16], def[16], lo[16], hi[16];

    for (int i = 0; i < NUM_SETTINGS; i++){
        const setting_info_t* info = battery_settings_get_info((setting_id_t)i);

        format_value(active, sizeof(active), battery_settings_get((setting_id_t)i), info);
        format_value(pending, sizeof(pending), battery_settings_get_pending((setting_id_t)i), info);
        format_value(def, sizeof(def), info->default_value, info);
        format_value(lo, sizeof(lo), info->min, info);
        format_value(hi, sizeof(hi), info->max, info);

        printf("SET idx=%d name=%s units=%s active=%s pending=%s default=%s min=%s max=%s\n",
            i, info->name, info->units, active, pending, def, lo, hi);
    }

    reply_ok("count=%d unsaved=%d", NUM_SETTINGS, battery_settings_has_unsaved() ? 1 : 0);
}



/*******cmd_get*******
 * Description
        > "get <name>": active and pending value of one setting
 * Arguments
        > int argc: number of words in command
        > char* argv[]: words in command
 * Returns
        > N/A
*/
static void cmd_get(int argc, char* argv[]){
    if (argc != 2){
        reply_err("bad_args", "usage: get <name>");
        return;
    }

    int id = battery_settings_find(argv[1]);
    if (id < 0){
        reply_err("bad_id", "unknown setting '%s' (try: list)", argv[1]);
        return;
    }

    const setting_info_t* info = battery_settings_get_info((setting_id_t)id);
    char active[16], pending[16];
    format_value(active, sizeof(active), battery_settings_get((setting_id_t)id), info);
    format_value(pending, sizeof(pending), battery_settings_get_pending((setting_id_t)id), info);

    reply_ok("name=%s active=%s pending=%s units=%s", info->name, active, pending, info->units);
}



/*******cmd_set*******
 * Description
        > "set <name> <value>": changes a PENDING setting (use "save" to keep it)
 * Arguments
        > int argc: number of words in command
        > char* argv[]: words in command
 * Returns
        > N/A
*/
static void cmd_set(int argc, char* argv[]){
    if (argc != 3){
        reply_err("bad_args", "usage: set <name> <value>");
        return;
    }

    int id = battery_settings_find(argv[1]);
    if (id < 0){
        reply_err("bad_id", "unknown setting '%s' (try: list)", argv[1]);
        return;
    }

    const setting_info_t* info = battery_settings_get_info((setting_id_t)id);
    bool is_bool = (strcmp(info->units, "bool") == 0);

    // boolean settings also accept on/off/true/false
        float value;
        if (is_bool && (strcmp(argv[2], "on") == 0 || strcmp(argv[2], "true") == 0)){value = 1.0f;}
        else if (is_bool && (strcmp(argv[2], "off") == 0 || strcmp(argv[2], "false") == 0)){value = 0.0f;}
        else if (!parse_float(argv[2], &value)){
            reply_err("bad_args", "'%s' is not a number", argv[2]);
            return;
        }

    if (is_bool && value != 0.0f && value != 1.0f){
        reply_err("range", "%s must be 0 or 1 (off / on)", info->name);
        return;
    }

    settings_status_t status = battery_settings_set_pending((setting_id_t)id, value);

    if (status != SETTINGS_OK){
        char lo[16], hi[16];
        format_value(lo, sizeof(lo), info->min, info);
        format_value(hi, sizeof(hi), info->max, info);
        reply_err(status_to_code(status), "%s must be %s to %s %s", info->name, lo, hi, info->units);
        return;
    }

    char pending[16];
    format_value(pending, sizeof(pending), battery_settings_get_pending((setting_id_t)id), info);
    reply_ok("name=%s pending=%s unsaved=%d", info->name, pending, battery_settings_has_unsaved() ? 1 : 0);
}



/*******cmd_validate*******
 * Description
        > "validate": checks pending settings (ranges and threshold order) without saving
 * Arguments
        > int argc: number of words in command
        > char* argv[]: words in command
 * Returns
        > N/A
*/
static void cmd_validate(int argc, char* argv[]){
    settings_status_t status = battery_settings_validate_pending();

    if (status == SETTINGS_OK){reply_ok("valid");}
    else if (status == SETTINGS_ERR_ORDER){reply_err("order", "thresholds must satisfy: shutdown < danger < warn (voltages), warn < danger < shutdown (temps)");}
    else {reply_err(status_to_code(status), "a pending value is out of range");}
}



/*******cmd_save*******
 * Description
        > "save": validates pending settings, writes them to the EEPROM, verifies, and makes them active
        > blocks for up to ~0.3 s while the EEPROM is written
 * Arguments
        > int argc: number of words in command
        > char* argv[]: words in command
 * Returns
        > N/A
*/
static void cmd_save(int argc, char* argv[]){
    settings_status_t status = battery_settings_save();

    if (status == SETTINGS_OK){
        batt_reevaluate();          // react to the new thresholds right away
        reply_ok("saved");
    }
    else if (status == SETTINGS_ERR_EEPROM){reply_err("eeprom", "EEPROM write or verify failed");}
    else if (status == SETTINGS_ERR_ORDER){reply_err("order", "not saved: thresholds must satisfy shutdown < danger < warn (voltages), warn < danger < shutdown (temps)");}
    else {reply_err(status_to_code(status), "not saved: a pending value is out of range");}
}



/*******cmd_discard*******
 * Description
        > "discard": throws away unsaved edits (pending = active)
 * Arguments
        > int argc: number of words in command
        > char* argv[]: words in command
 * Returns
        > N/A
*/
static void cmd_discard(int argc, char* argv[]){
    battery_settings_discard_pending();
    reply_ok("unsaved=%d", battery_settings_has_unsaved() ? 1 : 0);
}



/*******cmd_defaults*******
 * Description
        > "defaults": puts the default values into the pending settings (use "save" to keep them)
 * Arguments
        > int argc: number of words in command
        > char* argv[]: words in command
 * Returns
        > N/A
*/
static void cmd_defaults(int argc, char* argv[]){
    battery_settings_load_defaults();
    reply_ok("unsaved=%d", battery_settings_has_unsaved() ? 1 : 0);
}



/*******cmd_reload*******
 * Description
        > "reload": reads settings back from the EEPROM into active and pending
 * Arguments
        > int argc: number of words in command
        > char* argv[]: words in command
 * Returns
        > N/A
*/
static void cmd_reload(int argc, char* argv[]){
    settings_status_t status = battery_settings_load();

    if (status == SETTINGS_OK){
        batt_reevaluate();
        reply_ok("reloaded");
    }
    else {reply_err(status_to_code(status), "EEPROM empty or invalid, settings unchanged");}
}



/*******cmd_rate*******
 * Description
        > "rate [ms]": show or set how often the battery is measured
 * Arguments
        > int argc: number of words in command
        > char* argv[]: words in command
 * Returns
        > N/A
*/
static void cmd_rate(int argc, char* argv[]){
    if (argc == 1){
        reply_ok("check_ms=%lu", (unsigned long)batt_get_check_interval());
        return;
    }

    uint32_t value;
    if (argc != 2 || !parse_uint(argv[1], &value)){
        reply_err("bad_args", "usage: rate [ms]");
        return;
    }

    uint32_t applied = batt_set_check_interval(value);
    reply_ok("check_ms=%lu", (unsigned long)applied);
}



/*******cmd_stream*******
 * Description
        > "stream [ms]": show or set the time between S/T frames (0 = off)
 * Arguments
        > int argc: number of words in command
        > char* argv[]: words in command
 * Returns
        > N/A
*/
static void cmd_stream(int argc, char* argv[]){
    if (argc == 1){
        reply_ok("stream_ms=%lu", (unsigned long)stream_ms);
        return;
    }

    if (mode == SERIAL_MODE_SILENT){
        reply_err("mode", "silent mode sends nothing automatically (use: mode normal|debug)");
        return;
    }

    uint32_t value;
    if (argc != 2 || !parse_uint(argv[1], &value)){
        reply_err("bad_args", "usage: stream [ms]");
        return;
    }

    if (!valid_stream_ms(value)){
        reply_err("range", "stream_ms must be 0 or %d-%d", SERIAL_STREAM_MIN_MS, SERIAL_STREAM_MAX_MS);
        return;
    }

    stream_ms = value;
    next_stream_ms = now_ms();
    reply_ok("stream_ms=%lu", (unsigned long)stream_ms);
}



/*******ovr_show*******
 * Description
        > replies with the override mask and the value of every active override
 * Arguments
        > N/A
 * Returns
        > N/A
*/
static void ovr_show(){
    char text[128];
    size_t used = 0;
    float value;

    used += (size_t)snprintf(text + used, sizeof(text) - used, "mask=0x%02x", (unsigned)batt_override_get_mask());

    for (int i = 0; i < BATT_OVR_COUNT; i++){
        if (!batt_override_get_value((batt_ovr_target_t)i, &value)){continue;}
        if (used >= sizeof(text)){break;}

        if (i <= BATT_OVR_CELL5){used += (size_t)snprintf(text + used, sizeof(text) - used, " cell%d=%.3f", i + 1, (double)value);}
        else if (i == BATT_OVR_BATT){used += (size_t)snprintf(text + used, sizeof(text) - used, " batt=%.3f", (double)value);}
        else {used += (size_t)snprintf(text + used, sizeof(text) - used, " temp=%.2f", (double)value);}
    }

    reply_ok("%s", text);
}



/*******cmd_buzzer*******
 * Description
        > "buzzer": show whether the buzzers are enabled
        > "buzzer on" / "buzzer off": un-mute / mute the buzzers
        > muting is only allowed in DEBUG or SILENT mode, the buzzers are always enabled in
            NORMAL mode (entering NORMAL mode un-mutes them)
 * Arguments
        > int argc: number of words in command
        > char* argv[]: words in command
 * Returns
        > N/A
*/
static void cmd_buzzer(int argc, char* argv[]){
    if (argc == 1){
        reply_ok("buzzers=%d", get_buzzers_enabled() ? 1 : 0);
        return;
    }

    if (argc == 2 && strcmp(argv[1], "on") == 0){
        set_buzzers_enabled(true);
        batt_reevaluate();                  // restart beeping right away if the status needs it
        reply_ok("buzzers=1");
        return;
    }

    if (argc == 2 && strcmp(argv[1], "off") == 0){
        if (mode == SERIAL_MODE_NORMAL){
            reply_err("mode", "buzzers can only be muted in debug or silent mode (send: mode debug)");
            return;
        }
        set_buzzers_enabled(false);
        reply_ok("buzzers=0");
        return;
    }

    reply_err("bad_args", "usage: buzzer [on|off]");
}



/*******cmd_unlatch*******
 * Description
        > "unlatch": overrides the shutdown latch once (shtdwn_latch setting)
        > the system leaves the latched shutdown and re-checks the battery, if shutdown
            conditions are still present the shutdown countdown starts again (or the system
            goes straight back into shutdown if there is no delay)
        > allowed in every mode
 * Arguments
        > int argc: number of words in command
        > char* argv[]: words in command
 * Returns
        > N/A
*/
static void cmd_unlatch(int argc, char* argv[]){
    if (argc != 1){
        reply_err("bad_args", "usage: unlatch");
        return;
    }

    if (!batt_unlatch()){
        reply_err("state", "system is not latched in shutdown");
        return;
    }

    batt_snapshot_t snap;
    batt_get_snapshot(&snap);
    reply_ok("level=%d latched=%d", (int)snap.level, snap.latched ? 1 : 0);
}



/*******cmd_ovr*******
 * Description
        > DEBUG or SILENT mode only: simulates a battery condition
        > "ovr": show active overrides
        > "ovr cell <1-5> <V>", "ovr batt <V>", "ovr temp <C>": override a value
        > "ovr clear [cell <n>|batt|temp|all]": remove overrides
        > the system reacts immediately (LEDs, buzzer, display, telemetry)
 * Arguments
        > int argc: number of words in command
        > char* argv[]: words in command
 * Returns
        > N/A
*/
static void cmd_ovr(int argc, char* argv[]){
    if (mode == SERIAL_MODE_NORMAL){
        reply_err("mode", "overrides need debug or silent mode (send: mode debug)");
        return;
    }

    if (argc == 1){
        ovr_show();
        return;
    }

    // ovr clear ...
    if (strcmp(argv[1], "clear") == 0){
        if (argc == 2 || (argc == 3 && strcmp(argv[2], "all") == 0)){
            batt_override_clear_all();
            ovr_show();
            return;
        }
        if (argc == 3 && strcmp(argv[2], "batt") == 0){
            batt_override_clear(BATT_OVR_BATT);
            ovr_show();
            return;
        }
        if (argc == 3 && strcmp(argv[2], "temp") == 0){
            batt_override_clear(BATT_OVR_TEMP);
            ovr_show();
            return;
        }
        if (argc == 4 && strcmp(argv[2], "cell") == 0){
            uint32_t cell;
            if (!parse_uint(argv[3], &cell) || cell < 1 || cell > NUM_CELL_SLOTS){
                reply_err("bad_args", "cell must be 1-%d", NUM_CELL_SLOTS);
                return;
            }
            batt_override_clear((batt_ovr_target_t)(BATT_OVR_CELL1 + (int)cell - 1));
            ovr_show();
            return;
        }

        reply_err("bad_args", "usage: ovr clear [cell <n>|batt|temp|all]");
        return;
    }

    // ovr cell <n> <V>
    if (strcmp(argv[1], "cell") == 0){
        uint32_t cell;
        float value;

        if (argc != 4 || !parse_uint(argv[2], &cell) || cell < 1 || cell > NUM_CELL_SLOTS || !parse_float(argv[3], &value)){
            reply_err("bad_args", "usage: ovr cell <1-%d> <volts>", NUM_CELL_SLOTS);
            return;
        }

        if (!CELL_SLOT_POPULATED((int)cell - 1)){
            reply_err("bad_args", "cell %lu is not populated on this battery", (unsigned long)cell);
            return;
        }

        if (!batt_override_set((batt_ovr_target_t)(BATT_OVR_CELL1 + (int)cell - 1), value)){
            reply_err("range", "cell voltage must be %.1f to %.1f V", (double)OVR_CELL_MIN_V, (double)OVR_CELL_MAX_V);
            return;
        }

        ovr_show();
        return;
    }

    // ovr batt <V>
    if (strcmp(argv[1], "batt") == 0){
        float value;

        if (argc != 3 || !parse_float(argv[2], &value)){
            reply_err("bad_args", "usage: ovr batt <volts>");
            return;
        }

        if (!batt_override_set(BATT_OVR_BATT, value)){
            reply_err("range", "battery voltage must be %.1f to %.1f V", (double)OVR_BATT_MIN_V, (double)OVR_BATT_MAX_V);
            return;
        }

        ovr_show();
        return;
    }

    // ovr temp <C>
    if (strcmp(argv[1], "temp") == 0){
        float value;

        if (argc != 3 || !parse_float(argv[2], &value)){
            reply_err("bad_args", "usage: ovr temp <degC>");
            return;
        }

        if (!batt_override_set(BATT_OVR_TEMP, value)){
            reply_err("range", "temperature must be %.0f to %.0f C", (double)OVR_TEMP_MIN_C, (double)OVR_TEMP_MAX_C);
            return;
        }

        ovr_show();
        return;
    }

    reply_err("bad_args", "usage: ovr [cell <n> <V>|batt <V>|temp <C>|clear [...]]");
}
/******************</Command Handlers>*****************/



/******************<Command Table>*****************/
static const serial_cmd_t commands[] = {
    {"help",     cmd_help,     "list commands"},
    {"ping",     cmd_ping,     "keep-alive (resets override timeout)"},
    {"ver",      cmd_ver,      "firmware / protocol version"},
    {"info",     cmd_info,     "mode, timing, overrides, unsaved settings"},
    {"mode",     cmd_mode,     "mode [normal|silent|debug [stream_ms]]"},
    {"status",   cmd_status,   "one S frame with the current battery status"},
    {"telemetry", cmd_telemetry, "one T frame (effective + real values)"},
    {"list",     cmd_list,     "all settings (active, pending, default, limits)"},
    {"get",      cmd_get,      "get <name>"},
    {"set",      cmd_set,      "set <name> <value>  (pending until saved)"},
    {"validate", cmd_validate, "check pending settings"},
    {"save",     cmd_save,     "write pending settings to EEPROM and activate them"},
    {"discard",  cmd_discard,  "drop unsaved edits"},
    {"defaults", cmd_defaults, "load default values into pending"},
    {"reload",   cmd_reload,   "re-read settings from EEPROM"},
    {"rate",     cmd_rate,     "rate [ms]  (how often battery is measured)"},
    {"stream",   cmd_stream,   "stream [ms]  (S/T frame period, 0=off)"},
    {"buzzer",   cmd_buzzer,   "debug/silent only: buzzer [on|off]  (mute the buzzers)"},
    {"unlatch",  cmd_unlatch,  "override the shutdown latch once (system re-checks the battery)"},
    {"ovr",      cmd_ovr,      "debug/silent only: ovr [cell <n> <V>|batt <V>|temp <C>|clear [...]]"},
};

#define NUM_COMMANDS    (sizeof(commands) / sizeof(commands[0]))



/*******cmd_help*******
 * Description
        > "help": lists commands as '#' comment lines, then OK
 * Arguments
        > int argc: number of words in command
        > char* argv[]: words in command
 * Returns
        > N/A
*/
static void cmd_help(int argc, char* argv[]){
    for (size_t i = 0; i < NUM_COMMANDS; i++){
        printf("# %-9s %s\n", commands[i].name, commands[i].help);
    }

    printf("# frames: S,t_ms,age_ms,level,batt_v,temp_c,c1..c5,ovr_mask  (level 0=good 1=warn 2=danger 3=shutdown)\n");
    printf("# frames: T,t_ms,age_ms,seq,level,cell_lvl,batt_lvl,temp_lvl,ovr_mask,unsaved,[effective: batt_v,c1..c5,temp_c],[real: batt_v,c1..c5,temp_c]\n");
    reply_ok("proto=%d", SERIAL_PROTOCOL_VERSION);
}
/******************</Command Table>*****************/



/******************<Line Handling>*****************/

/*******process_line*******
 * Description
        > splits a complete line into words and runs the matching command
 * Arguments
        > char* line: command line (modified)
 * Returns
        > N/A
*/
static void process_line(char* line){
    char* argv[SERIAL_MAX_ARGS];
    int argc = tokenize(line, argv, SERIAL_MAX_ARGS);

    if (argc == 0){return;}

    if (argc < 0){
        reply_err("bad_args", "too many words (max %d)", SERIAL_MAX_ARGS);
        return;
    }

    for (size_t i = 0; i < NUM_COMMANDS; i++){
        if (strcmp(argv[0], commands[i].name) == 0){
            commands[i].handler(argc, argv);
            return;
        }
    }

    reply_err("unknown_cmd", "'%s' (try: help)", argv[0]);
}



/*******handle_char*******
 * Description
        > adds one received character to the line buffer, runs the command
            when the line ends
 * Arguments
        > int c: received character
 * Returns
        > N/A
*/
static void handle_char(int c){
    if (c == '\r' || c == '\n'){
        if (line_overflow){
            reply_err("overflow", "line longer than %d characters, ignored", SERIAL_LINE_MAX - 1);
        }
        else if (line_len > 0){
            line_buf[line_len] = '\0';
            process_line(line_buf);
        }

        line_len = 0;
        line_overflow = false;
        return;
    }

    if (line_len < SERIAL_LINE_MAX - 1){line_buf[line_len++] = (char)c;}
    else {line_overflow = true;}
}
/******************</Line Handling>*****************/



/******************<Public Functions>*****************/

/*******serial_interface_init*******
 * Description
        > starts the serial command interface in NORMAL mode
        > call after init_batt_mon() and init_battery_settings()
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void serial_interface_init(){
    mode = SERIAL_MODE_NORMAL;
    fast_rate = false;
    stream_ms = SERIAL_NORMAL_STREAM_MS;
    line_len = 0;
    line_overflow = false;

    last_activity_ms = now_ms();
    next_stream_ms = last_activity_ms;

    printf("# serial interface ready, proto %d, mode normal (send: help)\n", SERIAL_PROTOCOL_VERSION);
}



/*******serial_interface_poll*******
 * Description
        > call as often as possible from the main loop, never blocks
        > reads and runs received commands
        > returns to NORMAL mode if the host disconnects (from DEBUG or SILENT)
        > clears overrides if the host goes silent for SERIAL_OVERRIDE_TIMEOUT_MS
        > sends S (NORMAL) or T (DEBUG) frames at the stream period (none in SILENT)
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void serial_interface_poll(){
    uint32_t now = now_ms();

    // read and handle received characters
        for (int i = 0; i < SERIAL_RX_CHARS_PER_POLL; i++){
            int c = getchar_timeout_us(0);
            if (c == PICO_ERROR_TIMEOUT){break;}

            last_activity_ms = now;
            handle_char(c);
        }

    // host disconnected: leave DEBUG / SILENT mode (this also clears overrides)
        bool connected = stdio_usb_connected();

        if (!connected && mode != SERIAL_MODE_NORMAL){
            enter_mode(SERIAL_MODE_NORMAL, "disconnect");
        }

    // host silent while overrides are active: clear them
        now = now_ms();
        if (batt_override_get_mask() != 0 && (uint32_t)(now - last_activity_ms) > SERIAL_OVERRIDE_TIMEOUT_MS){
            batt_override_clear_all();
            printf("EVT ovr_cleared reason=timeout\n");
        }

    // periodic frames
        if (connected && mode != SERIAL_MODE_SILENT && stream_ms != 0 && (int32_t)(now - next_stream_ms) >= 0){
            next_stream_ms = now + stream_ms;

            batt_snapshot_t s;
            batt_get_snapshot(&s);

            if (s.have_data){
                if (mode == SERIAL_MODE_DEBUG){send_telemetry_frame(&s);}
                else {send_status_frame(&s);}
            }
        }
}



/*******serial_interface_get_mode*******
 * Description
        > gets the current mode
 * Arguments
        > N/A
 * Returns
        > serial_mode_t: SERIAL_MODE_NORMAL, SERIAL_MODE_DEBUG, or SERIAL_MODE_SILENT
*/
serial_mode_t serial_interface_get_mode(){
    return mode;
}

/******************</Public Functions>*****************/
