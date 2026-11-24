/******************<Dependencies>*****************/
#include "battery_settings.h"
/******************</Dependencies>*****************/



/******************<Setting Table>*****************/
#define SETTINGS_EEPROM_ADDR    0               // where settings block starts
#define SETTINGS_MAGIC          0xB477E5E7u     // arbitrary marker (erased EEPROM reads 0xFFFFFFFF, never matches)
#define SETTINGS_VERSION        5u              // increase whenever settings are added/removed/reordered!

/* EEPROM block layout (little endian)
*   [0..3] magic
*   [4..5] version
*   [6..7] number of settings
*   [8..]  NUM_SETTINGS x float
*   [last 2] CRC-16/CCITT-FALSE over all prior blocks
*/
#define SETTINGS_HEADER_BYTES   8u
#define SETTINGS_PAYLOAD_BYTES  (NUM_SETTINGS * sizeof(float))
#define SETTINGS_CRC_OFFSET     (SETTINGS_HEADER_BYTES + SETTINGS_PAYLOAD_BYTES)
#define SETTINGS_BLOB_BYTES     (SETTINGS_CRC_OFFSET + 2u)

// ensure settings fit in area reserved for them
_Static_assert(SETTINGS_EEPROM_ADDR + SETTINGS_BLOB_BYTES <= EEPROM_SIZE_BYTES, "settings do not fit in the reserved EEPROM area");
/******************</Setting Table>*****************/



/******************<Private Variables>*****************/
// generated from SETTINGS_TABLE in battery_settings.h
static const setting_info_t setting_info[NUM_SETTINGS] = {
    #define X(id, name, units, def, lo, hi) [SETTING_##id] = {name, units, def, lo, hi},
        SETTINGS_TABLE(X)
    #undef X
};

// working settings
static float active_values[NUM_SETTINGS];

// pending settings
static float pending_values[NUM_SETTINGS];
/******************</Private Variables>*****************/



/******************<Private Functions>*****************/
/*******crc16_ccitt*******
 * Description
        > generates a CRC16-CCITT for error-checking
 * Arguments
        > const uint8_t* data: bytes to check
        > size_t len: number of bytes
 * Returns
        > uint16_t: generated CRC
*/
static uint16_t crc16_ccitt(const uint8_t* data, size_t len){
    uint16_t crc = 0xFFFF;

    for (size_t i = 0; i < len; i++){
        crc ^= (uint16_t)data[i] << 8;
        for (int bit = 0; bit < 8; bit++){
            if (crc & 0x8000){crc = (uint16_t)((crc << 1) ^ 0x1021);}
            else {crc = (uint16_t)(crc << 1);}
        }
    }

    return crc;
}


/*******check_range*******
 * Description
        > checks a value against a setting's min/max
        > NaN always fails
 * Arguments
        > setting_id_t id: setting to check against (must be valid)
        > float value: value to check
 * Returns
        > settings_status_t: BAD_ID, RANGE, or OK
*/
static settings_status_t check_range(setting_id_t id, float value){
    // validate ID
        if ((int)(id) < 0 || id >= NUM_SETTINGS){
            return SETTINGS_ERR_BAD_ID;
        }

    // validate value
        if ( !(value >= setting_info[id].min && value <= setting_info[id].max) ){
            return SETTINGS_ERR_RANGE;
        }

    // if ID and value good, return good
        return SETTINGS_OK;
}



/*******check_order*******
 * Description
        > checks values to ensure they make sense
            logically relative to others
        > checks voltage and temperature values
            warn voltage > danger voltage > shutdown voltages
            warn temp > danger temp > shutdown temp
 * Arguments
        > const float* settings: settings table
 * Returns
        > settings_status_t: ERR_ORDER or OK
*/
static settings_status_t check_order(const float* settings){
    // check cell voltage values
        bool cell_ok = settings[SETTING_SHTDWN_CELL_VOLTAGE] < settings[SETTING_DANGER_CELL_VOLTAGE]
                    && settings[SETTING_DANGER_CELL_VOLTAGE] < settings[SETTING_WARN_CELL_VOLTAGE];

    // check battery voltage values
        bool batt_ok = settings[SETTING_SHTDWN_BATT_VOLTAGE] < settings[SETTING_DANGER_BATT_VOLTAGE]
                    && settings[SETTING_DANGER_BATT_VOLTAGE] < settings[SETTING_WARN_BATT_VOLTAGE];

    // check temperature values
        bool temp_ok = settings[SETTING_DANGER_TEMP] < settings[SETTING_SHTDWN_TEMP]
                    && settings[SETTING_WARN_TEMP]   < settings[SETTING_DANGER_TEMP];

    // return appropriate error code
        if (!cell_ok || !batt_ok || !temp_ok){return SETTINGS_ERR_ORDER;}

        return SETTINGS_OK;        
}



/*******validate_values*******
 * Description
        > range checks every setting, also checks threshold order
 * Arguments
        > const float* settings: settings table
 * Returns
        > settings_status_t: ERR_ORDER or OK
*/
static settings_status_t validate_values(const float* settings){

    // check values to ensure within min/max
        for (int i = 0; i < NUM_SETTINGS; i++){
            settings_status_t res = check_range((setting_id_t)(i), settings[i]);
            if (res != SETTINGS_OK){return res;}
        }

    // check values to see if they are ordered correctly (e.g., warn voltage greater than danger voltage)
        return check_order(settings);
}



/*******fill_defaults*******
 * Description
        > copies every setting's default value into an array
 * Arguments
        > float* dest: array of NUM_SETTINGS floats to fill
 * Returns
        > N/A
*/
static void fill_defaults(float* dest){
    for (int i = 0; i < NUM_SETTINGS; i++){
        dest[i] = setting_info[i].default_value;
    }
}



/*******pack_blob*******
 * Description
        > builds the EEPROM image (header + values + CRC) from a value array
 * Arguments
        > uint8_t* buf: output buffer, SETTINGS_BLOB_BYTES long
        > const float* values: array of NUM_SETTINGS values to store
 * Returns
        > N/A
*/
static void pack_blob(uint8_t* buf, const float* values){
    uint32_t magic = SETTINGS_MAGIC;
    uint16_t version = SETTINGS_VERSION;
    uint16_t count = NUM_SETTINGS;

    memcpy(&buf[0], &magic, sizeof(magic));
    memcpy(&buf[4], &version, sizeof(version));
    memcpy(&buf[6], &count, sizeof(count));
    memcpy(&buf[SETTINGS_HEADER_BYTES], values, SETTINGS_PAYLOAD_BYTES);

    uint16_t crc = crc16_ccitt(buf, SETTINGS_CRC_OFFSET);
    memcpy(&buf[SETTINGS_CRC_OFFSET], &crc, sizeof(crc));
}



/*******unpack_blob*******
 * Description
        > validates an EEPROM image and extracts the values
        > checks magic number, version, count, and CRC, then the range
            and order of the values
 * Arguments
        > const uint8_t* buf: EEPROM image, SETTINGS_BLOB_BYTES long
        > float* out_values: array of NUM_SETTINGS floats, only written on success
 * Returns
        > settings_status_t: SETTINGS_OK if image valid, otherwise reason for rejection
*/
static settings_status_t unpack_blob(const uint8_t* buf, float* out_values){
    // define variables
        uint32_t magic;
        uint16_t version, count, stored_crc;
        float values[NUM_SETTINGS];

    // load setting strings
        memcpy(&magic, &buf[0], sizeof(magic));
        memcpy(&version, &buf[4], sizeof(version));
        memcpy(&count, &buf[6], sizeof(count));
        memcpy(&stored_crc, &buf[SETTINGS_CRC_OFFSET], sizeof(stored_crc));

    // validate EEPROM image
        if (magic != SETTINGS_MAGIC || version != SETTINGS_VERSION || count != NUM_SETTINGS){
            return SETTINGS_ERR_EEPROM;
        }

        if (stored_crc != crc16_ccitt(buf, SETTINGS_CRC_OFFSET)){
            return SETTINGS_ERR_EEPROM;
        }

    // if valid image, extract values
        memcpy(values, &buf[SETTINGS_HEADER_BYTES], SETTINGS_PAYLOAD_BYTES);

    // validate values
        settings_status_t s = validate_values(values);
        if (s != SETTINGS_OK){return s;}

        memcpy(out_values, values, SETTINGS_PAYLOAD_BYTES);
        return SETTINGS_OK;
}
/******************</Private Functions>*****************/



/******************<Public Functions>*****************/


/*******battery_settings_load*******
 * Description
        > reads settings from EEPROM into active_values AND pending_values
        > on failure, nothing is changed
 * Arguments
        > N/A
 * Returns
        > settings_status_t:
            SETTINGS_OK         -> good
            SETTINGS_ERR_EEPROM -> read failed, blank, bad CRC
            SETTINGS_ERR_RANGE  -> unacceptable values
            SETTINGS_ERR_ORDER  -> unacceptable values
*/
settings_status_t battery_settings_load(){
    // create variables to store loaded settings
        uint8_t buf[SETTINGS_BLOB_BYTES];
        float loaded[NUM_SETTINGS];

    // read from EEPROM
        if (!onboard_memory_read(SETTINGS_EEPROM_ADDR, buf, SETTINGS_BLOB_BYTES)){
            return SETTINGS_ERR_EEPROM;
        }

    // unpack settings
        settings_status_t s = unpack_blob(buf, loaded);
        if (s != SETTINGS_OK){return s;}

    // store settings in active and pending values
        memcpy(active_values, loaded, sizeof(active_values));
        memcpy(pending_values, loaded, sizeof(pending_values));

    // return if successful
        return SETTINGS_OK;
}



/*******init_battery_settings*******
 * Description
        > initializes the EEPROM and loads saved settings
        > if EEPROM is blank/corrupt/from another version, defaults from
            SETTINGS_TABLE are used (EEPROM will not be modified)
        > call before init_batt_mon()
 * Arguments
        > N/A
 * Returns
        > settings_status_t: SETTINGS_OK=loaded from EEPROM, otherwise reason
            for error
*/
settings_status_t init_battery_settings(){
    init_onboard_memory();

    fill_defaults(active_values);
    fill_defaults(pending_values);

    return battery_settings_load();
}



/*******battery_settings_save*******
 * Description
        > validates pending values, writes them to EEPROM, reads them back to
            verify, then copies them into active_values
        > only bytes that differ from what is already stored are written
        > BLOCKS for up to 5ms per changed byte (~0.3 sec in worst case)
        > EEPROM is write-protected before and after, pass or fail
        > validation failure leaves EEPROM and active_values untouched
 * Arguments
        > N/A
 * Returns
        > settings_status_t: SETTINGS_OK or reason for failure
*/
settings_status_t battery_settings_save(){
    // create variables to store current and new values
        uint8_t image[SETTINGS_BLOB_BYTES];
        uint8_t current[SETTINGS_BLOB_BYTES];

    // validate pending values
        settings_status_t s = validate_values(pending_values);
        if (s != SETTINGS_OK){return s;}

    // pack settings into blob
        pack_blob(image, pending_values);

    // double-check that current settings and EEPROM match, write new settings
        if (!onboard_memory_read(SETTINGS_EEPROM_ADDR, current, SETTINGS_BLOB_BYTES)){
            return SETTINGS_ERR_EEPROM;
        }

        bool ok = true;
        onboard_memory_set_write_enable(true);
        
        for (uint16_t i = 0; i < SETTINGS_BLOB_BYTES && ok; i++){
            if (current[i] != image[i]){
                ok = onboard_memory_write(SETTINGS_EEPROM_ADDR + i, &image[i], 1);
            }
        }

        onboard_memory_set_write_enable(false);
        if (!ok){return SETTINGS_ERR_EEPROM;}

    // read freshly written settings back to verify successful write
        if (!onboard_memory_read(SETTINGS_EEPROM_ADDR, current, SETTINGS_BLOB_BYTES)){
            return SETTINGS_ERR_EEPROM;
        }

        if (memcmp(current, image, SETTINGS_BLOB_BYTES) != 0){
            return SETTINGS_ERR_EEPROM;
        }

    // copy new settings into active_values
        memcpy(active_values, pending_values, sizeof(active_values));
        
    // return on success
        return SETTINGS_OK;
}



/*******battery_settings_get*******
 * Description
        > gets the active (working) value of a setting
 * Arguments
        > setting_id_t id: setting to read
 * Returns
        > float: active value, or 0 if id is invalid
*/
float battery_settings_get(setting_id_t id){
    if ((int)(id) < 0 || id >= NUM_SETTINGS){
        return 0.0f;
    }

    return active_values[id];
}



/*******battery_settings_get_pending*******
 * Description
        > gets the pending (not yet saved) value of a setting
 * Arguments
        > setting_id_t id: setting to read
 * Returns
        > float: pending value, or 0 if id is invalid
*/
float battery_settings_get_pending(setting_id_t id){
    if ((int)(id) < 0 || id >= NUM_SETTINGS){
        return 0.0f;
    }

    return pending_values[id];
}



/*******battery_settings_set_pending*******
 * Description
        > edits a pending value; active value not affected until save
        > only checks this one value's range, threshold order is checked
            later in battery_settings_validate_pending
 * Arguments
        > setting_id_t id: setting to edit
        > float value: new pending value
 * Returns
        > settings_status_t: SETTINGS_OK or reason for failure
*/
settings_status_t battery_settings_set_pending(setting_id_t id, float value){
    settings_status_t s = check_range(id, value);
    if (s != SETTINGS_OK){return s;}

    pending_values[id] = value;
    return SETTINGS_OK;
}



/*******battery_settings_validate_pending*******
 * Description
        > checks every pending value's range, then threshold order
        > does not modify anything
 * Arguments
        > N/A
 * Returns
        > settings_status_t: SETTINGS_OK or reason for failure
*/
settings_status_t battery_settings_validate_pending(){
    return validate_values(pending_values);
}



/*******battery_settings_has_unsaved*******
 * Description
        > checks whether pending_values differ from active_values
 * Arguments
        > N/A
 * Returns
        > bool: true=unsaved changes, false=pending matches active settings
*/
bool battery_settings_has_unsaved(){
    return memcmp(active_values, pending_values, sizeof(active_values)) != 0;
}



/*******battery_settings_discard_pending*******
 * Description
        > discards unsaved edits by copying active_values into pending_values
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void battery_settings_discard_pending(){
    memcpy(pending_values, active_values, sizeof(pending_values));
}



/*******battery_settings_load_defaults*******
 * Description
        > puts every new setting's default value into pending_values
        > nothing becomes active or is written to EEPROM until save
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void battery_settings_load_defaults(){
    fill_defaults(pending_values);
}



/*******battery_settings_get_info*******
 * Description
        > gets the name/units/default/min/max for a setting
 * Arguments
        > setting_id_t id: setting to look up
 * Returns
        > const setting_info_t*: pointer to record or NULL if invalid id
*/
const setting_info_t* battery_settings_get_info(setting_id_t id){
    if ((int)id < 0 || id >= NUM_SETTINGS){return NULL;}
    return &setting_info[id];
}




/*******battery_settings_get_name*******
 * Description
        > gets the lowercase serial name of a setting
 * Arguments
        > setting_id_t id: setting to look up
 * Returns
        > const char*: name string or NULL if invalid id
*/
const char* battery_settings_get_name(setting_id_t id){
    if ((int)id < 0 || id >= NUM_SETTINGS){return NULL;}
    return setting_info[id].name;
}



/*******battery_settings_find*******
 * Description
        > looks up a setting ID from its serial name (case-sensitive)
 * Arguments
        > const char* name: setting name (e.g., "warn_temp")
 * Returns
        > int: setting ID, or -1 if name is NULL or unknown
*/
int battery_settings_find(const char* name){
    if (name == NULL){return -1;}
    
    for (int i = 0; i < NUM_SETTINGS; i++){
        if (strcmp(name, setting_info[i].name) == 0){return i;}
    }

    return -1;
}
/******************</Public Functions>*****************/
