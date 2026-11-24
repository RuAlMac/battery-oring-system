

#include "pico_pwm.h"


/******************<Public variables>*****************/
uint8_t buzzer_1_pwm_slice = 0;
uint8_t buzzer_1_pwm_channel = 0;      // 0=A, 1=B

uint8_t buzzer_2_pwm_slice = 0;
uint8_t buzzer_2_pwm_channel = 0;      // 0=A, 1=B
/******************</Public variables>*****************/



void pico_pwm_init(){
    // init gpio
		gpio_init(BUZZER_1_PIN);
        gpio_init(BUZZER_2_PIN);
		gpio_set_dir(BUZZER_1_PIN, GPIO_OUT);
		gpio_set_dir(BUZZER_2_PIN, GPIO_OUT);

    // initialize buzzer GPIO as PWM
		gpio_set_function(BUZZER_1_PIN, GPIO_FUNC_PWM);
        gpio_set_function(BUZZER_2_PIN, GPIO_FUNC_PWM);

	// set config for PWM pins
		pwm_config config = pwm_get_default_config();   	// get default PWM configuration
		pwm_config_set_clkdiv(&config, BUZZER_PWM_CLKDIV);  // set sys clock divider

	// get PWM slice and channel for buzzer pin
		buzzer_1_pwm_slice = pwm_gpio_to_slice_num(BUZZER_1_PIN);
		buzzer_1_pwm_channel = pwm_gpio_to_channel(BUZZER_1_PIN);      // 0=A, 1=B

        buzzer_2_pwm_slice = pwm_gpio_to_slice_num(BUZZER_2_PIN);
		buzzer_2_pwm_channel = pwm_gpio_to_channel(BUZZER_2_PIN);      // 0=A, 1=B

    // initialize buzzer 1 with its own config
        pwm_config config_1 = pwm_get_default_config();
        pwm_config_set_clkdiv(&config_1, BUZZER_PWM_CLKDIV);
        pwm_config_set_wrap(&config_1, BUZZER_1_PWM_WRAP);
        pwm_init(buzzer_1_pwm_slice, &config_1, true);

    // initialize buzzer 2 with its own config
        pwm_config config_2 = pwm_get_default_config();
        pwm_config_set_clkdiv(&config_2, BUZZER_PWM_CLKDIV);
        pwm_config_set_wrap(&config_2, BUZZER_2_PWM_WRAP);
        pwm_init(buzzer_2_pwm_slice, &config_2, true);

	// return
		return;
}


void set_buzzer(uint8_t target, bool enable){

    switch(target){
        case 1: {
            pwm_set_chan_level(buzzer_1_pwm_slice, buzzer_1_pwm_channel, enable ? BUZZER_1_PWM_WRAP / 2 : 0);
            break;
        }
        case 2: {
            pwm_set_chan_level(buzzer_2_pwm_slice, buzzer_2_pwm_channel, enable ? BUZZER_2_PWM_WRAP / 2 : 0);
            break;
        }
        default: {
            printf("set_buzzer(): invalid buzzer");
        }
    }

    return;
}