/**
 * @file main.c
 * @brief Solução para o Exercício 1 do Simulado de Sistemas Embarcados.
 * * Funcionalidade:
 * - Uma barra de 5 LEDs exibe um contador de 0 a 5.
 * - Um botão (BTN) altera o valor do contador.
 * - Uma chave (SWITCH) define se o botão incrementa ou decrementa o contador.
 * * Arquitetura:
 * - 100% orientada a eventos, sem uso de gpio_get() no loop principal.
 * - Uma ISR (Interrupt Service Routine) unificada lida com os eventos do botão e da chave.
 * - Variáveis globais 'volatile' são usadas para comunicar o estado da ISR para o main.
 */

#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/gpio.h"

// --- Definição dos Pinos ---
// Constantes tornam o código mais legível e fácil de modificar.
const int LED_1 = 2;
const int LED_2 = 3;
const int LED_3 = 4;
const int LED_4 = 5;
const int LED_5 = 6;

// Um array para acessar os pinos dos LEDs de forma mais fácil nas funções.
const int led_pins[] = {LED_1, LED_2, LED_3, LED_4, LED_5};

const int BTN = 22;    // Pino para o botão de ação.
const int SWITCH = 28; // Pino para a chave de modo (incremento/decremento).

// --- Variáveis Globais de Estado ---
// A palavra-chave 'volatile' é ESSENCIAL. Ela avisa ao compilador que essas
// variáveis podem ser alteradas a qualquer momento por uma interrupção (ISR),
// impedindo otimizações que poderiam quebrar o código.

// Flag para sinalizar que o botão foi pressionado.
volatile bool btn_pressed_flag = false;
// Variável para armazenar o modo atual (decremento ou incremento).
// Inicia como 'true' (decremento) porque usamos pull-up na chave,
// então seu estado padrão (pino em nível alto) corresponde ao modo de decremento.
volatile bool decrement = true;

/**
 * @brief Callback unificada para todas as interrupções de GPIO.
 * Esta função é o coração da lógica orientada a eventos.
 */
void gpio_callback(uint gpio, uint32_t events) {
    // Verifica se a interrupção veio do pino do BOTÃO e se foi uma borda de descida.
    if (gpio == BTN && (events & GPIO_IRQ_EDGE_FALL)) {
        // Evento do botão: A ISR faz o mínimo possível. Apenas sinaliza
        // para o loop 'main' que o botão foi pressionado.
        btn_pressed_flag = true;
    } 
    // Verifica se a interrupção veio do pino da CHAVE.
    else if (gpio == SWITCH) {
        // Evento da chave: A ISR atualiza a variável de estado 'decrement'.
        if (events & GPIO_IRQ_EDGE_FALL) {
            // A chave foi para nível baixo (0), conectando o pino ao GND.
            // Conforme a regra, nível baixo = MODO INCREMENTO.
            decrement = false;
        } else if (events & GPIO_IRQ_EDGE_RISE) {
            // A chave foi para nível alto (1), desconectando do GND.
            // O pull-up interno eleva o pino. Nível alto = MODO DECREMENTO.
            decrement = true;
        }
    }
}

/**
 * @brief Inicializa os pinos da barra de LEDs como saídas.
 */
void bar_init() {
    for (int i=0; i < 5; i++) {
        gpio_init(led_pins[i]);
        gpio_set_dir(led_pins[i], GPIO_OUT);
    }
}

/**
 * @brief Atualiza a barra de LEDs para exibir um valor entre 0 e 5.
 * @param val O valor a ser exibido (0 a 5).
 */
void bar_display(int val) {
    // Primeiro, apaga todos os LEDs para limpar o estado anterior.
    for (int i=0; i<5; i++) {
        gpio_put(led_pins[i], 0);
    }

    // Acende os LEDs necessários. Se val=3, acende os LEDs 0, 1 e 2.
    for (int i=0; i<val; i++) {
        gpio_put(led_pins[i], 1);
    }
}

int main() {
    stdio_init_all();
    bar_init();

    // --- Configuração da Chave (SWITCH) ---
    gpio_init(SWITCH);
    gpio_set_dir(SWITCH, GPIO_IN);
    gpio_pull_up(SWITCH); // Pull-up para definir um estado padrão (alto) quando a chave está aberta.
    // Habilita a interrupção para a chave em AMBAS as bordas (subida e descida),
    // pois queremos saber toda vez que ela muda de posição.
    gpio_set_irq_enabled(SWITCH, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);

    // --- Configuração do Botão (BTN) ---
    gpio_init(BTN);
    gpio_set_dir(BTN, GPIO_IN);
    gpio_pull_up(BTN); // Pull-up para o botão. Ele lerá 'baixo' quando pressionado.
    // Habilita a interrupção para o botão e REGISTRA a função de callback.
    // A mesma 'gpio_callback' agora servirá tanto ao botão quanto à chave.
    gpio_set_irq_enabled_with_callback(BTN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    int counter = 0;
    bar_display(counter); // Exibe o estado inicial (contador = 0).

    while (true) {
        // O loop principal é muito simples e eficiente (não-bloqueante).
        // Ele apenas verifica se a ISR sinalizou um evento.
        if (btn_pressed_flag) {
            // "Consome" o evento, reiniciando a flag para evitar processá-lo novamente.
            btn_pressed_flag = false;

            // Em vez de ler o pino com gpio_get(), consultamos nossa variável de estado
            // que a ISR mantém atualizada.
            if (decrement) {
                counter--;
            } else {
                counter++;
            }

            // Garante que o contador não saia do intervalo válido [0, 5].
            // Isso é chamado de "saturação".
            if (counter > 5) {
                counter = 5;
            }
            if (counter < 0) {
                counter = 0;
            }

            // Atualiza o hardware para refletir o novo valor do contador.
            bar_display(counter);
        }
    }
}