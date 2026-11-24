/******************<Dependencies>*****************/
#include "indication_board.h"
/******************</Dependencies>*****************/



/******************<Private Variables>*****************/
// digit_to_segment: segments to enable/disable based on value to display
static const uint8_t digit_to_segment[10] = {       // DP, G, F, E, D, C, B, A
    0b00111111, // 0: A B C D E F
    0b00000110, // 1: B C
    0b01011011, // 2: A B D E G
    0b01001111, // 3: A B C D G
    0b01100110, // 4: B C F G
    0b01101101, // 5: A C D F G
    0b01111101, // 6: A C D E F G
    0b00000111, // 7: A B C
    0b01111111, // 8: all
    0b01101111, // 9: A B C D F G
};


// seg_state: stores active segment states for each display
static uint8_t seg_state[NUM_SEGMENTS];

// seg_enabled: false = outputs of the shift registers are disabled (OE high), displays are completely off
static bool seg_enabled = true;


// LED bank state
static led_bank_mode_t led_bank_mode = LED_BANK_OFF;    // current behavior of the LED banks
static repeating_timer_t led_bank_timer;                // toggles the banks while blinking
static uint32_t led_bank_blink_interval = 0;            // time (ms) each blink phase lasts, 0 when not blinking
static volatile bool led_bank_blink_state = false;      // true = bank 1 on / bank 2 off, false = bank 1 off / bank 2 on
static bool led_bank_timer_running = false;             // true=led_bank_timer is active
/******************</Private Variables>*****************/



/******************<LED Bank ISR>*****************/
/*******led_bank_timer_callback*******
 * Description
        > blink timer interrupt: swaps which LED bank is on (the banks are always inverses of each other)
 * Arguments
        > repeating_timer_t* rt: timer that fired (unused)
 * Returns
        > bool: true=keep the timer running
*/
static bool led_bank_timer_callback(repeating_timer_t* rt){
    led_bank_blink_state = !led_bank_blink_state;

    gpio_put(EXTERNAL_LED_1_PIN, led_bank_blink_state);
    gpio_put(EXTERNAL_LED_2_PIN, !led_bank_blink_state);

    return true;
}
/******************</LED Bank ISR>*****************/



/******************<Private Functions>*****************/
/*******sevenseg_shift_out_data*******
 * Description
        > shifts data out over the shift registers for the 
            seven-seg displays
 * Arguments
        > N/A
 * Returns
        > N/A
*/
static void sevenseg_shift_out_data(){
    // send data to seg2, then seg1, then seg0
        uint8_t tx_buffer[NUM_SEGMENTS];

        for (int i = 1; i <= NUM_SEGMENTS; i++){
            tx_buffer[i-1] = seg_state[NUM_SEGMENTS - i];
        }

        spi_write_blocking(SEG_SPI_BUS, tx_buffer, NUM_SEGMENTS);

    // pulse latch enable: the outputs only change when LE is high, so LE must be LOW while data is
    // shifting in (otherwise every segment flickers as the bits pass through the registers)
        sleep_us(1);
        gpio_put(SEG_LE_PIN, 1);
        sleep_us(1);
        gpio_put(SEG_LE_PIN, 0);
}
/******************</Private Functions>*****************/




/******************<Segment Displays>*****************/
/*******init_sevenseg*******
 * Description
        > initializes the shift-registers used for driving
            the seven-segment displays on the indication board
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void init_segment_displays(){
    // latch enable
        gpio_init(SEG_LE_PIN);
        gpio_set_dir(SEG_LE_PIN, GPIO_OUT);
        gpio_put(SEG_LE_PIN, GPIO_SET_LOW);

    // output enable
        gpio_init(SEG_OE_PIN);
        gpio_set_dir(SEG_OE_PIN, GPIO_OUT);
        gpio_put(SEG_OE_PIN, GPIO_SET_LOW);

    // init SPI in Mode 0 (clk idles low, data sampled on rising edge, MSB first)
        spi_init(SEG_SPI_BUS, SEG_SPI_BAUDRATE);
        spi_set_format(SEG_SPI_BUS, NUM_DATA_BITS, CLK_OUT_POLARITY, CLK_OUT_PHASE, SPI_MSB_FIRST);
        gpio_set_function(SEG_SCK_PIN, GPIO_FUNC_SPI);
        gpio_set_function(SEG_MOSI_PIN, GPIO_FUNC_SPI);
        gpio_set_function(SEG_MISO_PIN, GPIO_FUNC_SPI);

    // set all segments low, latch on, enable outputs (OE=active low)
        for (int i = 0; i < NUM_SEGMENTS; i++){
            seg_state[i] = 0x00;
        }

        sevenseg_shift_out_data();
        gpio_put(SEG_OE_PIN, GPIO_SET_LOW);
}



/*******sevenseg_write*******
 * Description
        > writes a number to a seven-seg display
 * Arguments
        > uint8_t segment: segment to update (0, 1, or 2)
        >   uint8_t digit: digit to write
        >         bool dp: 1=decimal point on, 0=decimal point off
 * Returns
        > N/A
*/
void sevenseg_write(uint8_t segment, uint8_t digit, bool dp){

    // validate inputs
        if (segment > NUM_SEGMENTS){
            printf("sevenseg_write(): invalid segment [%u]\n", segment);
            return;
        }

        if (digit > 9){
            printf("sevenseg_write(): invalid digit [%u]\n", digit);
            return;
        }

    // determine which segments to enable/disable based on digit
        uint8_t active_segments = digit_to_segment[digit];
        if (dp){active_segments |= SEG_DP_BIT;}

    // write to target seven-seg display (nothing to do if it already shows this)
        if (seg_state[segment] == active_segments){return;}
        seg_state[segment] = active_segments;
        sevenseg_shift_out_data();

    return;
}



/*******sevenseg_set_blank*******
 * Description
        > disables all segments on a seven-seg display
 * Arguments
        > uint8_t segment: segment to update (0, 1, or 2)
 * Returns
        > N/A
*/
void sevenseg_set_blank(uint8_t segment){
    // validate inputs
        if (segment > NUM_SEGMENTS){
            printf("sevenseg_write(): invalid segment [%u]\n", segment);
            return;
        }

    // write out to target display
        if (seg_state[segment] == 0x00){return;}
        seg_state[segment] = 0x00;
        sevenseg_shift_out_data();

    return;
}
/*******sevenseg_set_enabled*******
 * Description
        > turns all seven-seg displays completely off (disabled) or back on
        > disabling blanks every segment and disables the shift register outputs (OE high)
        > enabling re-enables the outputs, the displays stay blank until sevenseg_write() is called
 * Arguments
        > bool enabled: true=displays can light, false=displays completely off
 * Returns
        > N/A
*/
void sevenseg_set_enabled(bool enabled){
    if (enabled == seg_enabled){return;}

    if (!enabled){
        for (int i = 0; i < NUM_SEGMENTS; i++){seg_state[i] = 0x00;}
        sevenseg_shift_out_data();
        gpio_put(SEG_OE_PIN, 1);        // OE is active low, high = outputs off
    }
    else {
        gpio_put(SEG_OE_PIN, 0);
    }

    seg_enabled = enabled;
}



/*******sevenseg_get_enabled*******
 * Description
        > gets whether the seven-seg displays are enabled
 * Arguments
        > N/A
 * Returns
        > bool: true=enabled, false=completely off
*/
bool sevenseg_get_enabled(){
    return seg_enabled;
}
/******************</Segment Displays>*****************/



/******************<LED Banks>*****************/
/*******init_led_banks*******
 * Description
        > initializes the GPIO that drive the two LED banks and turns both banks off
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void init_led_banks(){
    gpio_init(EXTERNAL_LED_1_PIN);
    gpio_init(EXTERNAL_LED_2_PIN);

    gpio_set_dir(EXTERNAL_LED_1_PIN, GPIO_OUT);
    gpio_set_dir(EXTERNAL_LED_2_PIN, GPIO_OUT);

    led_bank_mode = LED_BANK_SOLID_ON;      // force set_led_bank_mode() to apply OFF below
    set_led_bank_mode(LED_BANK_OFF, 0);
}



/*******set_led_bank*******
 * Description
        > turns a single LED bank on or off
        > does not touch the blink timer, use set_led_bank_mode() for normal operation
 * Arguments
        > uint8_t bank: bank to change (1 or 2)
        > bool state: true=bank on, false=bank off
 * Returns
        > N/A
*/
void set_led_bank(uint8_t bank, bool state){
    switch (bank){
        case 1: {gpio_put(EXTERNAL_LED_1_PIN, state); break;}
        case 2: {gpio_put(EXTERNAL_LED_2_PIN, state); break;}
        default: {printf("set_led_bank(): invalid bank [%u]\n", bank); break;}
    }
}



/*******set_led_bank_mode*******
 * Description
        > sets how the LED banks behave (off, blinking, solid on)
        > blinking: the banks are inverses of each other, bank 1 on while bank 2 is off, then they swap
            every blink_interval_ms (starts with bank 1 on)
        > starts / stops the blink timer as needed
        > calling this again with the same mode (and, for blinking, the same interval) does nothing,
            so it can be called after every battery measurement without restarting the blink phase
        > if only the interval changed, the blink timer is restarted with the new interval
 * Arguments
        > led_bank_mode_t mode: new behavior
        > uint32_t blink_interval_ms: time (ms) each blink phase lasts, only used by LED_BANK_BLINK
            (limited to at least LED_BANK_MIN_BLINK_MS)
 * Returns
        > N/A
*/
void set_led_bank_mode(led_bank_mode_t mode, uint32_t blink_interval_ms){
    if (blink_interval_ms < LED_BANK_MIN_BLINK_MS){blink_interval_ms = LED_BANK_MIN_BLINK_MS;}
    if (mode != LED_BANK_BLINK){blink_interval_ms = 0;}

    // nothing to do if nothing changed
        if (mode == led_bank_mode && blink_interval_ms == led_bank_blink_interval){return;}

    // stop blinking
        if (led_bank_timer_running){
            cancel_repeating_timer(&led_bank_timer);
            led_bank_timer_running = false;
        }

    // apply new mode
        led_bank_mode = mode;
        led_bank_blink_interval = blink_interval_ms;

        switch (mode){
            case LED_BANK_BLINK: {
                led_bank_blink_state = true;                        // start with bank 1 on, bank 2 off
                gpio_put(EXTERNAL_LED_1_PIN, true);
                gpio_put(EXTERNAL_LED_2_PIN, false);

                led_bank_timer_running = add_repeating_timer_ms(blink_interval_ms, led_bank_timer_callback, NULL, &led_bank_timer);
                break;
            }

            case LED_BANK_SOLID_ON: {
                gpio_put(EXTERNAL_LED_1_PIN, true);
                gpio_put(EXTERNAL_LED_2_PIN, true);
                break;
            }

            case LED_BANK_OFF:
            default: {
                gpio_put(EXTERNAL_LED_1_PIN, false);
                gpio_put(EXTERNAL_LED_2_PIN, false);
                led_bank_mode = LED_BANK_OFF;
                break;
            }
        }
}



/*******get_led_bank_mode*******
 * Description
        > gets the current LED bank behavior
 * Arguments
        > N/A
 * Returns
        > led_bank_mode_t: current mode
*/
led_bank_mode_t get_led_bank_mode(){
    return led_bank_mode;
}



/*******get_led_bank_blink_interval*******
 * Description
        > gets the time each blink phase lasts
 * Arguments
        > N/A
 * Returns
        > uint32_t: blink interval in ms, 0 if the banks are not blinking
*/
uint32_t get_led_bank_blink_interval(){
    return led_bank_blink_interval;
}
/******************</LED Banks>*****************/
