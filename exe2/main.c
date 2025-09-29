
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/adc.h"

const int LED_AM = 5;
const int LED_AZ = 9;

const int BTN = 28;

volatile bool btn_press = false;
volatile bool blinking = false;

repeating_timer_t timer_am;
repeating_timer_t timer_az;

void btn_callback(uint gpio, uint32_t events) {
    if (gpio == BTN && (events & GPIO_IRQ_EDGE_FALL)) {
        btn_press = true;
    }
}

bool repeating_timer_callback_am(repeating_timer_t *rt) {
    (void)rt;
    gpio_put(LED_AM, !gpio_get(LED_AM));
    return true;
}

bool repeating_timer_callback_az(repeating_timer_t *rt) {
    (void)rt;
    gpio_put(LED_AZ, !gpio_get(LED_AZ));
    return true;
}

int64_t alarm_5s_callback(alarm_id_t id, void *user_data) {
    (void)id;
    (void)user_data;

    cancel_repeating_timer(&timer_am);
    cancel_repeating_timer(&timer_az);

    gpio_put(LED_AM, 0);
    gpio_put(LED_AZ, 0);

    blinking = false;

    return 0;
}


int main() {
    stdio_init_all();

    gpio_init(LED_AM);
    gpio_set_dir(LED_AM, GPIO_OUT);
    gpio_init(LED_AZ);
    gpio_set_dir(LED_AZ, GPIO_OUT);

    gpio_init(BTN);
    gpio_set_dir(BTN, GPIO_IN);
    gpio_pull_up(BTN);

    gpio_set_irq_enabled_with_callback(BTN, GPIO_IRQ_EDGE_FALL, true, &btn_callback);

    while (true) {
        if(btn_press) {
            btn_press = false;

            if(!blinking) {
                blinking = true;

                add_repeating_timer_ms(500, repeating_timer_callback_am, NULL, &timer_am);
                
                add_repeating_timer_ms(150, repeating_timer_callback_az, NULL, &timer_az);

                add_alarm_in_ms(5000, alarm_5s_callback, NULL, false);
            }
        }
    }
}
