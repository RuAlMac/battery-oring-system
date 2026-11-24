#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include "communication/pico_i2c.h"
#include "peripherals/bq769.h"

#include "pins.h"


int main() {
    stdio_init_all();

    // delay to give time to open serial monitor for debugging
    sleep_ms(2000);

    // init I2C
    init_pico_i2c(BATT_MON_I2C_BUS, SDA_PIN, SCL_PIN, I2C_BAUDRATE, true);

    // create instance of bq769_config struct
        bq769_config batt_mon_config;

    // initialize battery monitor
        init_bq769(BATT_MON_I2C_BUS, bq7692006pwr_addr, &batt_mon_config);


    while (true) {

        // read cell voltages
            float voltage = 0;
            for (int i = 1; i <= 5; i++){
                bq769_measure_cell(&batt_mon_config, i, &voltage);
                sleep_ms(50);
            }

        // read battery voltage
            float batt_voltage = 0.0f;
            bq769_measure_battery(&batt_mon_config, &batt_voltage);

        // read temperature
            float temp_c = 0.0f;
            bq760_get_temperature(&batt_mon_config, &temp_c);

        sleep_ms(500);
    }
}
