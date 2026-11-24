/******************<Dependencies>*****************/
#include "onboard_peripherals.h"
/******************</Dependencies>*****************/



/******************<Private Variables>*****************/
// Instances of buzzer_config struct for each buzzer
buzzer_config buzzer_1_config;
buzzer_config buzzer_2_config;

static bool buzzer_timer_running = false;
static uint32_t buzzer_timer_interval = 0;      // time (ms) of each phase of the beep pattern, 0 when not beeping
static volatile uint8_t buzzer_phase = 0;       // 0: both buzzers sound, 1: silent
static bool buzzers_enabled = true;             // false = buzzers are muted (debug feature)

static repeating_timer_t buzzer_timer;
/******************</Private Variables>*****************/



/******************<Buzzer Beep ISR>*****************/
/*******buzzer_apply_phase*******
 * Description
        > drives the two buzzers for one phase of the beep pattern
            phase 0: both buzzers sound at the same time (maximum volume)
            phase 1: silence
 * Arguments
        > uint8_t phase: 0 or 1
 * Returns
        > N/A
*/
static void buzzer_apply_phase(uint8_t phase){
    set_buzzer(&buzzer_1_config, phase == 0);
    set_buzzer(&buzzer_2_config, phase == 0);
}



/*******buzzer_timer_callback*******
 * Description
        > timer interrupt: toggles the beep pattern between sounding and silent
 * Arguments
        > repeating_timer_t* rt: timer that fired (unused)
 * Returns
        > bool: true=keep the timer running
*/
static bool buzzer_timer_callback(repeating_timer_t *rt){
    buzzer_phase = (buzzer_phase + 1) & 0x01;
    buzzer_apply_phase(buzzer_phase);
    return true;
}
/******************</Buzzer Beep ISR>*****************/




/******************<LEDs>*****************/

/*******set_led*******
 * Description
        > enables/disables a certain LED
 * Arguments
        > char led: LED to target ('g', 'y', 'r')
        > bool state: true=LED on, false=LED off
 * Returns
        > N/A
*/
void set_led(char led, bool state){

    uint8_t led_pin = 0;

    switch(led){
        case 'g': {led_pin = LED_G_PIN; break;}
        case 'y': {led_pin = LED_Y_PIN; break;}
        case 'r': {led_pin = LED_R_PIN; break;}
        default: {printf("set_led(): invalid LED '%c'", led); return;}
    }

    gpio_put(led_pin, state);
}



/*******set_led_byte*******
 * Description
        > enables/disables LEDs based on the value of a passed-in byte
 * Arguments
        > uint8_t: value
            bit 0: green LED
            bit 1: yellow LED
            bit 2: red LED
 * Returns
        > N/A
*/
void set_led_byte(uint8_t value){

    // disregard bits 3-7
        uint8_t filtered_val = value & 0b00000111;

    // extract LED states
        bool green_state  = (filtered_val >> 0) & 0x01;
        bool yellow_state = (filtered_val >> 1) & 0x01;
        bool red_state    = (filtered_val >> 2) & 0x01;

    // set LED states
        set_led('g', green_state);
        set_led('y', yellow_state);
        set_led('r', red_state);
}



/*******set_all_led*******
 * Description
        > enables/disables all LEDs
 * Arguments
        > bool state: true=LED on, false=LED off
 * Returns
        > N/A
*/
void set_all_led(bool state){
    set_led('g', state);
    set_led('y', state);
    set_led('r', state);
}



/*******init_leds*******
 * Description
        > initializes onboard LEDs
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void init_leds(){
    // initialize GPIO
		gpio_init(LED_G_PIN);
		gpio_init(LED_Y_PIN);
		gpio_init(LED_R_PIN);

	// set GPIO as outputs
		gpio_set_dir(LED_G_PIN, GPIO_OUT);
		gpio_set_dir(LED_Y_PIN, GPIO_OUT);
		gpio_set_dir(LED_R_PIN, GPIO_OUT);

	// set LEDs high
		set_all_led(true);

    // delay
        sleep_ms(250);

    // set LEDs low
        set_all_led(false);
}


/******************</LEDs>*****************/



/******************<Buzzer>*****************/
/*******init_buzzer*******
 * Description
        > initializes one buzzer
 * Arguments
        > buzzer_config* config: pointer to struct containing info on buzzer config
        > uint8_t id: id to assign to this buzzer
 * Returns
        > N/A
*/
void init_buzzer(buzzer_config* b_config, uint8_t id){

    // validate ID
        if (id < 1 || id > NUM_BUZZERS){
            printf("init_buzzer(): invalid buzzer ID! Must be 1 up to NUM_BUZZERS\n");
            return;
        }

    // get info for this buzzer
        b_config->buzzer_id = id;
        b_config->state = false;

        switch(id){
            case (1):{
                b_config->buzzer_pin = BUZZER_CONTROL_1_PIN;
                break;
            }

            case (2):{
                b_config->buzzer_pin = BUZZER_CONTROL_2_PIN;
                break;
            }
        }

    // initialize GPIO
		gpio_init(b_config->buzzer_pin);

	// set GPIO as output
		gpio_set_dir(b_config->buzzer_pin, GPIO_OUT);

	// set buzzer pin function as PWM
        gpio_set_function(b_config->buzzer_pin, GPIO_FUNC_PWM);

    // get PWM slice and channel for buzzer pin
		b_config->pwm_slice = pwm_gpio_to_slice_num(b_config->buzzer_pin);
		b_config->pwm_channel = pwm_gpio_to_channel(b_config->buzzer_pin);      // 0=A, 1=B

    // initialize buzzer with its config (silent until set_buzzer() is called)
        pwm_config p_config = pwm_get_default_config();
        pwm_config_set_clkdiv(&p_config, BUZZER_PWM_CLKDIV);
        pwm_config_set_wrap(&p_config, BUZZER_PWM_WRAP);
        pwm_init(b_config->pwm_slice, &p_config, true);
        pwm_set_chan_level(b_config->pwm_slice, b_config->pwm_channel, 0);

    // print message
        printf("init_buzzer(): initialized buzzer %u\n", b_config->buzzer_id);
}



/*******init_all_buzzers*******
 * Description
        > initializes both onboard buzzers
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void init_all_buzzers(){
    init_buzzer(&buzzer_1_config, 1);
    init_buzzer(&buzzer_2_config, 2);
}



/*******set_buzzer*******
 * Description
        > turns a buzzer on (fixed tone, exactly 50% duty cycle) or off
        > does nothing (buzzer stays silent) while the buzzers are muted
 * Arguments
        > buzzer_config* config: pointer to config struct for this buzzer
        > bool state: true=turn buzzer ON, false=turn buzzer OFF
 * Returns
        > N/A
*/
void set_buzzer(buzzer_config* config, bool state){

    // muted buzzers never sound
        if (!buzzers_enabled){state = false;}

    // set PWM level
        config->state = state;
        pwm_set_chan_level(config->pwm_slice, config->pwm_channel, state ? BUZZER_PWM_LEVEL : 0);
}



/*******start_buzzer_beeping*******
 * Description
        > starts the beep pattern: both buzzers sound together, silence, ... every phase lasts 'beep_ms'
        > calling this again with the same beep_ms does nothing (the pattern keeps running)
        > calling it with a different beep_ms restarts the pattern at the new speed
        > does nothing while the buzzers are muted
 * Arguments
        > uint32_t beep_ms: duration of each phase of the pattern, in ms
 * Returns
        > bool: true=pattern (re)started, false=nothing changed
*/
bool start_buzzer_beeping(uint32_t beep_ms){

    if (!buzzers_enabled || beep_ms == 0){return false;}

    // already beeping at this speed
        if (buzzer_timer_running && beep_ms == buzzer_timer_interval){return false;}

    // different speed: replace the timer
        if (buzzer_timer_running){
            cancel_repeating_timer(&buzzer_timer);
            buzzer_timer_running = false;
        }

    // first phase right away, then let the timer step through the pattern
        buzzer_phase = 0;
        buzzer_apply_phase(buzzer_phase);

        buzzer_timer_running = add_repeating_timer_ms(beep_ms, buzzer_timer_callback, NULL, &buzzer_timer);
        buzzer_timer_interval = buzzer_timer_running ? beep_ms : 0;
        return buzzer_timer_running;
}



/*******get_buzzer_interval*******
 * Description
        > gets the duration of each phase of the current beep pattern
 * Arguments
        > uint32_t* interval_ms: pointer to location to store current interval (0 = not beeping)
 * Returns
        > N/A
*/
void get_buzzer_interval(uint32_t* interval_ms){
    *interval_ms = buzzer_timer_interval;
}



/*******stop_buzzer_beeping*******
 * Description
        > stops the beep pattern and silences both buzzers
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void stop_buzzer_beeping(){
    if (buzzer_timer_running){
        cancel_repeating_timer(&buzzer_timer);
        buzzer_timer_running = false;
    }
    buzzer_timer_interval = 0;
    set_buzzer(&buzzer_1_config, false);
    set_buzzer(&buzzer_2_config, false);
}



/*******set_buzzers_enabled*******
 * Description
        > mutes or un-mutes the buzzers
        > muting stops any beeping immediately; after un-muting the beeping restarts the next
            time the battery status is evaluated (batt_reevaluate())
 * Arguments
        > bool enabled: true=buzzers allowed to sound, false=buzzers muted
 * Returns
        > N/A
*/
void set_buzzers_enabled(bool enabled){
    buzzers_enabled = enabled;
    if (!enabled){stop_buzzer_beeping();}
}



/*******get_buzzers_enabled*******
 * Description
        > gets whether the buzzers are allowed to sound
 * Arguments
        > N/A
 * Returns
        > bool: true=enabled, false=muted
*/
bool get_buzzers_enabled(){
    return buzzers_enabled;
}
/******************</Buzzer>*****************/
