/******************<Dependencies>*****************/
#include "peripherals/led_matrix.h"
/******************</Dependencies>*****************/


/******************<Private Variables>*****************/
static led_matrix_config* active_matrix = NULL;     // placeholder for active config struct instance
static int current_row = 0;
static repeating_timer_t scan_timer;                // timer used for scanning through each row
/******************</Private Variables>*****************/


/******************<Timer ISR>*****************/

/*******matrix_scan_callback*******
 * Description
        > updates the next row in the LED matrix
        > vertically scans through the matrix, allows for
            proper MUXing of LEDs so that each LED can be
            enabled or disabled without affecting others
 * Type
        > Repeating Timer/Counter ISR
*/
static bool matrix_scan_callback(){
    // if no matrix has been selected, end ISR immediately
        if (active_matrix == NULL){return true;}

    // turn all rows off to prevent artifacts
        for (int i = 0; i < 8; i++){
            gpio_put(active_matrix->row[i], GPIO_SET_HIGH);
        }

    // turn on columns for LEDs that should be on in this row
        for (int this_col = 0; this_col < 8; this_col++){
            if (active_matrix->pixels[current_row][this_col]){
                gpio_put(active_matrix->col[this_col], GPIO_SET_HIGH);       // turn col on
            }
            else {
                gpio_put(active_matrix->col[this_col], GPIO_SET_LOW);        // turn col off
            }
        }

    // turn on current row
        gpio_put(active_matrix->row[current_row], GPIO_SET_LOW);

    // increment current col, wrap back to 0 if past 7
        current_row = (current_row + 1) % 8;

    // end ISR, allow timer to continue (return true)
        return true;
}
/******************</Timer ISR>*****************/


/******************<Public Functions>*****************/

/*******init_led_matrix*******
 * Description
        > initializes GPIO connected to the 8x8 LED matrix
 * Arguments
        > led_matrix_config* config: pointer to instance of
            matrix config struct
 * Returns
        > N/A
*/
void init_led_matrix(led_matrix_config* config){
    // add pin numbers to row and col arrays
        config->row[0] = ROW_0_PIN;
        config->row[1] = ROW_1_PIN;
        config->row[2] = ROW_2_PIN;
        config->row[3] = ROW_3_PIN;
        config->row[4] = ROW_4_PIN;
        config->row[5] = ROW_5_PIN;
        config->row[6] = ROW_6_PIN;
        config->row[7] = ROW_7_PIN;

        config->col[0] = COL_A_PIN;
        config->col[1] = COL_B_PIN;
        config->col[2] = COL_C_PIN;
        config->col[3] = COL_D_PIN;
        config->col[4] = COL_E_PIN;
        config->col[5] = COL_F_PIN;
        config->col[6] = COL_G_PIN;
        config->col[7] = COL_H_PIN;

    // init relevant GPIO
        for (int i = 0;  i < 8; i++){
            gpio_init(config->col[i]);
            gpio_set_dir(config->col[i], GPIO_OUT);

            gpio_init(config->row[i]);
            gpio_set_dir(config->row[i], GPIO_OUT);

            // turn off all LEDs
            gpio_put(config->row[i], GPIO_SET_HIGH);         // set all rows high
            gpio_put(config->col[i], GPIO_SET_LOW);        // set all cols low
        }

    // init pixel matrix
        for (int x = 0; x < 8; x++){
            for (int y = 0; y < 8; y++){
                config->pixels[x][y] = false; // all LEDs are initially off
            }
        }
}



/*******start_led_matrix*******
 * Description
        > starts vertically scanning through the LED matrix
            to turn individual LEDs on or off
 * Arguments
        > led_matrix_config* config: pointer to instance of
            matrix config struct
 * Returns
        > N/A
*/
void start_led_matrix(led_matrix_config* config){
    // set current LED matrix config
        active_matrix = config;
    
    // create timer/counter interrupt, associate ISR
        add_repeating_timer_ms(TIME_BETWEEN_SCANS_MS, matrix_scan_callback, NULL, &scan_timer);
}



/*******set_matrix_led*******
 * Description
        > enables or disables a single LED within the LED
            matrix
 * Arguments
        > led_matrix_config* config: pointer to instance of
            matrix config struct
        > uint8_t x: x-coordinate of targeted LED (column)
        > uint8_t y: y-coordinate of targeted LED (row)
        > bool state: true=turn LED on, false=turn LED off
 * Returns
        > N/A
*/
void set_matrix_led(led_matrix_config* config, uint8_t x, uint8_t y, bool state){
    // update pixels array in passed-in matrix config struct
        config->pixels[x][y] = state;
}

/******************</Public Functions>*****************/

