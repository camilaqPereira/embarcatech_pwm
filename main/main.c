#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/timer.h"

/* Constants declaration*/
const uint GPIO_SERVOMOTOR = 22;    // Servomotor gpio
const float PWM_clock_div = 50.0;   // PWM clock divisor
const uint16_t PWM_wrap = 50000;    // PWM wrap
const uint16_t step = 12;           // Second stage: step of +5us on Tpwm
const uint16_t increment_top_limit = 5990;  // Largest active time in sequence -> 

/* Global variables*/
struct repeating_timer servomotor_timer;    //Timer for second

uint16_t PWM_active_time[] = {6000, 3675, 1250};  //duty cycle of 12% -> 0.12*50000 = 6000
                                                  //duty cycle of 7.35% -> 0.0735 * 50000 = 3675
                                                  //duty cycle of 2.5% -> 0.025 * 50000 = 1250
size_t active_time_size = 3;

static volatile uint16_t level = 1250;  // Current active time in sequence
static volatile bool increment = true;  // Movement orientation
static volatile ssize_t sequence_index = 0; // Active time selector


/* Function prototypes */

uint pwm_setup();
int64_t servomotor_alarm_callback(alarm_id_t id, void* user_data);
bool servomotor_timer_callback(struct repeating_timer *t);

int main()
{
    stdio_init_all();
    uint slice_num = pwm_setup();

    /* Set up alarm for first fase*/
    //Begin in 5ms
    alarm_id_t id = add_alarm_in_ms(5, &servomotor_alarm_callback, NULL, false);


    while (true) {
        //tight_loop_contents();
        sleep_ms(1000);
    }

}

/*
*   @brief Fnction for servomotor pwm setup
*/

uint pwm_setup(){
    uint slice;
    gpio_set_function(GPIO_SERVOMOTOR, GPIO_FUNC_PWM);
    slice = pwm_gpio_to_slice_num(GPIO_SERVOMOTOR);

    pwm_set_clkdiv(slice, PWM_clock_div);
    pwm_set_wrap(slice, PWM_wrap);
    pwm_set_gpio_level(GPIO_SERVOMOTOR, PWM_active_time[2]);
    
    pwm_set_enabled(slice, true);
    
    return slice;
}

/*
*   @brief Callback for second fase timer
*/
bool servomotor_timer_callback(struct repeating_timer *t){
    /* Modify pwm duty cycle by step*/
    if(increment){
        level+=12; // Increment if moving from 0->180
    }else{
        level-=12; //Decrement if moving from 180->0
    }

    pwm_set_gpio_level(GPIO_SERVOMOTOR, level);
    
    /* Toggle movement orientation at limits*/
    if(level == increment_top_limit || level == PWM_active_time[2]){ //Switch directions in got to limits
        increment = !increment;
    }

    t->delay_us = 10000; //redefine timer alarm time

    return true; //reset timer

}

/*
    Callback for first fase alarm
*/
int64_t servomotor_alarm_callback(alarm_id_t id, void* user_data){
    /* Next duty cycle in sequence*/
    pwm_set_gpio_level(GPIO_SERVOMOTOR, PWM_active_time[sequence_index++]);

    /* If the sequence is completed, setup timer for second fase*/
    if (sequence_index == 3){
        add_repeating_timer_ms(5000, &servomotor_timer_callback, NULL, &servomotor_timer);
        return 0; //stop alarm
    }else{
        /* Setup alarm for another shot*/
        return 5000000;
    }
    
}




