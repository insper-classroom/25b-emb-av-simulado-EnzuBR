#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/gpio.h"

const int LED_1 = 2;
const int LED_2 = 3;
const int LED_3 = 4;
const int LED_4 = 5;
const int LED_5 = 6;

const int led_pins[] = {LED_1, LED_2, LED_3, LED_4, LED_5};

const int BTN = 22;

const int SWITCH = 28;

volatile bool btn_pressed_flag = false;
volatile bool decrement = true;

void gpio_callback(uint gpio, uint32_t events) {
    if (gpio == BTN && (events & GPIO_IRQ_EDGE_FALL)) {
        // Evento do botão: apenas sinaliza para o main.
        btn_pressed_flag = true;
    } else if (gpio == SWITCH) {
        // Evento da chave: atualiza a variável de estado.
        if (events & GPIO_IRQ_EDGE_FALL) {
            // A chave foi para nível baixo (0) -> Modo Incremento
            decrement = false;
        } else if (events & GPIO_IRQ_EDGE_RISE) {
            // A chave foi para nível alto (1) -> Modo Decremento
            decrement = true;
        }
    }
}

void bar_init() {
    for (int i=0; i < 5; i++) {
        gpio_init(led_pins[i]);
        gpio_set_dir(led_pins[i], GPIO_OUT);
    }
}

void bar_display(int val) {
    for (int i=0; i<5; i++) {
        gpio_put(led_pins[i], 0);
    }

    for (int i=0; i<val; i++) {
        gpio_put(led_pins[i], 1);
    }
}

int main() {
    stdio_init_all();
    bar_init();

    gpio_init(SWITCH);
    gpio_set_dir(SWITCH, GPIO_IN);
    gpio_pull_up(SWITCH);

    gpio_set_irq_enabled(SWITCH, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);

    gpio_init(BTN);
    gpio_set_dir(BTN, GPIO_IN);
    gpio_pull_up(BTN);

    gpio_set_irq_enabled_with_callback(BTN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    int counter = 0;
    bar_display(counter);

    while (true) {
        if (btn_pressed_flag) {
            btn_pressed_flag = false;

            if (decrement) {
                counter--;
            } else {
                counter++;
            }

            if (counter > 5) {
                counter = 5;
            }
            if (counter < 0) {
                counter = 0;
            }

            bar_display(counter);
        }

    }
}
