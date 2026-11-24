/******************<Dependencies>*****************/
#include "peripherals/safety_devices.h"
/******************</Dependencies>*****************/



/******************<Private Variables>*****************/
// Instances of buzzer_config struct for each buzzer
buzzer_config buzzer_1_config;
buzzer_config buzzer_2_config;

static bool buzzer_timer_running;
static uint8_t buzzer_timer_interval;

static repeating_timer_t buzzer_timer;
/******************</Private Variables>*****************/



/******************<Buzzer Beep ISR>*****************/
static bool buzzer_timer_callback(repeating_timer_t *rt){

    // toggle buzzer states
        buzzer_1_config.state = !buzzer_1_config.state;
        buzzer_2_config.state = !buzzer_2_config.state;
    
    // turn buzzers on/off
        set_buzzer(&buzzer_1_config, buzzer_1_config.state, 0.5f);
        set_buzzer(&buzzer_2_config, buzzer_2_config.state, 0.5f);

    // allow timer to continue running
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

        switch(id){
            case (1):{
                b_config->buzzer_pin = BUZZER_CONTROL_1_PIN;
                b_config->pwm_clkdiv = BUZZER_1_PWM_CLKDIV;
                b_config->pwm_wrap = BUZZER_1_PWM_WRAP;
                break;
            }

            case (2):{
                b_config->buzzer_pin = BUZZER_CONTROL_2_PIN;
                b_config->pwm_clkdiv = BUZZER_2_PWM_CLKDIV;
                b_config->pwm_wrap = BUZZER_2_PWM_WRAP;
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

    // initialize buzzer with its config
        pwm_config p_config = pwm_get_default_config();
        pwm_config_set_clkdiv(&p_config, b_config->pwm_clkdiv);
        pwm_config_set_wrap(&p_config, b_config->pwm_wrap);
        pwm_init(b_config->pwm_slice, &p_config, true);

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
        > enables or disables a buzzer at a specified duty cycle
 * Arguments
        > buzzer_config* config: pointer to config struct for this buzzer
        > bool state: true=turn buzzer ON, false=turn buzzer OFF
        > float duty_cycle: 0.0 to 1.0, sets duty cycle for buzzer
 * Returns
        > N/A
*/
void set_buzzer(buzzer_config* config, bool state, float duty_cycle){

    // validate input
        if (duty_cycle < 0.0 || duty_cycle > 1.0){
            printf("set_buzzer(): invalid duty cycle (%f)", duty_cycle);
        } 

    // set PWM wrap val
        uint16_t wrap_val = (uint16_t)( duty_cycle * (float)(config->pwm_wrap) );
        pwm_set_chan_level(config->pwm_slice, config->pwm_channel, state ? wrap_val : 0);
}



/*******start_buzzer_beeping*******
 * Description
        > beeps both buzzers at a fixed interval
 * Arguments
        > uint32_t interval_ms: buzzer will toggle on/off every 'interval_ms'
 * Returns
        > bool: true=new timer created, false=timer already exists
*/
bool start_buzzer_beeping(uint32_t interval_ms){

    // if there is currently a timer running, do not start a new one
        if (buzzer_timer_running){return false;}

    // create timer/counter
        add_repeating_timer_ms(interval_ms, buzzer_timer_callback, NULL, &buzzer_timer);
        buzzer_timer_running = true;
        buzzer_timer_interval = interval_ms;
        return true;
}



/*******get_buzzer_interval*******
 * Description
        > gets the current buzzer timer interval (in ms)
 * Arguments
        > uint32_t* interval_ms: pointer to location to store current interval
 * Returns
        > N/A
*/
void get_buzzer_interval(uint32_t* interval_ms){
    *interval_ms = buzzer_timer_interval;
}



/*******stop_buzzer_beeping*******
 * Description
        > stops the buzzer beep
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void stop_buzzer_beeping(){
    buzzer_timer_running = false;
    cancel_repeating_timer(&buzzer_timer);
    set_buzzer(&buzzer_1_config, false, 0.0f);
    set_buzzer(&buzzer_2_config, false, 0.0f);
}
/******************</Buzzer>*****************/



/******************<Battery ORing>*****************/


/*******set_batt_ORing*******
 * Description
        > enables or disables battery ORing for this battery
        > NOTE: disabling ORing will shut off this battery from the sub!
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void set_batt_ORing(bool state){
    gpio_put(ORING_EN_PIN, state ? GPIO_SET_HIGH : GPIO_SET_LOW);
}


/*******init_batt_ORing*******
 * Description
        > initializes GPIO relevant to the power ORing IC
            used on the Base Board
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void init_batt_ORing(){
    // init relevant GPIO
        gpio_init(ORING_EN_PIN);
        gpio_set_dir(ORING_EN_PIN, GPIO_OUT);

    // enable ORing on init
        set_batt_ORing(true);
}

/******************</Battery ORing>*****************/