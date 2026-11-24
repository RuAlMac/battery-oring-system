/******************<Dependencies>*****************/
#include "peripherals/safety_devices.h"
/******************</Dependencies>*****************/




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
        case 'g': {led_pin = LED_G; break;}
        case 'y': {led_pin = LED_Y; break;}
        case 'r': {led_pin = LED_R; break;}
        default: {printf("set_led(): invalid LED '%c'", led); return;}
    }

    gpio_put(led_pin, state);
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
		gpio_init(LED_G);
		gpio_init(LED_Y);
		gpio_init(LED_R);

	// set GPIO as outputs
		gpio_set_dir(LED_G, GPIO_OUT);
		gpio_set_dir(LED_Y, GPIO_OUT);
		gpio_set_dir(LED_R, GPIO_OUT);

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
        > initializes onboard buzzer
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void init_buzzer(){
    // initialize GPIO
		gpio_init(BUZZER_CONTROL);

	// set GPIO as outputs
		gpio_set_dir(BUZZER_CONTROL, GPIO_OUT);

	// set buzzer high
        gpio_put(BUZZER_CONTROL, GPIO_SET_HIGH);
        sleep_ms(100);
        gpio_put(BUZZER_CONTROL, GPIO_SET_LOW);

}
/******************</Buzzer>*****************/