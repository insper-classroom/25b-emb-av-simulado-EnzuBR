/**
 * @file main.c
 * @brief Solução para o Exercício 2 do Simulado de Sistemas Embarcados.
 *
 * ## Funcionalidade:
 * - Ao pressionar um botão, dois LEDs (amarelo e azul) piscam
 * independentemente por 5 segundos.
 * - Amarelo: período de 500ms.
 * - Azul: período de 150ms.
 * - Após 5s, ambos param e se apagam.
 *
 * ## Arquitetura:
 * - Bare-metal (sem RTOS), orientado a eventos com timers e interrupções.
 * - **3 Timers Independentes**:
 * 1. `repeating_timer` para o LED amarelo.
 * 2. `repeating_timer` para o LED azul.
 * 3. `alarm` (one-shot) para o timeout de 5 segundos que encerra a sequência.
 * - Uma **variável de estado** `blinking` controla o fluxo para evitar que a
 * sequência seja reativada enquanto já está em execução.
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h" // Necessário para usar timers e alarmes
#include "hardware/adc.h"   // (Não usado neste código, mas incluído no original)

// --- Definição dos Pinos ---
// Usar constantes para os pinos torna o código mais legível e fácil de manter.
const int LED_AM = 5; // LED Amarelo
const int LED_AZ = 9; // LED Azul
const int BTN = 28;   // Botão de início

// --- Variáveis Globais ---
// A palavra-chave 'volatile' é crucial para variáveis compartilhadas entre
// o código principal e as rotinas de interrupção (ISRs/callbacks).
// Ela impede que o compilador faça otimizações que poderiam quebrar a lógica.

// Flag para a ISR do botão sinalizar ao loop 'main' que um evento ocorreu.
volatile bool btn_press = false;
// Flag de estado para controlar se a sequência de pisca está ativa ou não.
volatile bool blinking = false;

// Handles (identificadores) para os timers. Precisam ser globais para que
// a callback do alarme possa acessá-los e cancelá-los.
repeating_timer_t timer_am;
repeating_timer_t timer_az;

/**
 * @brief Callback da interrupção do botão.
 * Executada quando o botão é pressionado (borda de descida).
 * A função é mínima e rápida, apenas define a flag para o 'main' processar.
 */
void btn_callback(uint gpio, uint32_t events) {
    if (gpio == BTN && (events & GPIO_IRQ_EDGE_FALL)) {
        btn_press = true;
    }
}

/**
 * @brief Callback do timer repetitivo para o LED amarelo.
 * Esta função é chamada a cada X milissegundos para inverter o estado do LED.
 */
bool repeating_timer_callback_am(repeating_timer_t *rt) {
    (void)rt; // Parâmetro não utilizado, '(void)rt;' evita avisos do compilador.
    gpio_put(LED_AM, !gpio_get(LED_AM));
    return true; // Retornar 'true' garante que o timer continue repetindo.
}

/**
 * @brief Callback do timer repetitivo para o LED azul.
 */
bool repeating_timer_callback_az(repeating_timer_t *rt) {
    (void)rt;
    gpio_put(LED_AZ, !gpio_get(LED_AZ));
    return true;
}

/**
 * @brief Callback do alarme de 5 segundos (disparo único).
 * Esta função é a "equipe de limpeza": é chamada uma única vez para
 * encerrar a sequência de pisca.
 */
int64_t alarm_5s_callback(alarm_id_t id, void *user_data) {
    (void)id;
    (void)user_data;

    // 1. Cancela os timers repetitivos para parar o pisca-pisca.
    cancel_repeating_timer(&timer_am);
    cancel_repeating_timer(&timer_az);

    // 2. Garante que ambos os LEDs terminem no estado "apagado".
    gpio_put(LED_AM, 0);
    gpio_put(LED_AZ, 0);

    // 3. Reseta a flag de estado, permitindo que uma nova sequência
    //    comece no próximo clique do botão.
    blinking = false;

    return 0; // Retornar 0 significa que o alarme não deve ser reagendado.
}


int main() {
    stdio_init_all();

    // --- Configuração dos GPIOs ---
    gpio_init(LED_AM);
    gpio_set_dir(LED_AM, GPIO_OUT);
    gpio_init(LED_AZ);
    gpio_set_dir(LED_AZ, GPIO_OUT);

    gpio_init(BTN);
    gpio_set_dir(BTN, GPIO_IN);
    gpio_pull_up(BTN); // Habilita resistor de pull-up para o botão.

    // Configura a interrupção para o botão, ligando o evento físico (pressão)
    // à nossa função de callback.
    gpio_set_irq_enabled_with_callback(BTN, GPIO_IRQ_EDGE_FALL, true, &btn_callback);

    // O loop principal é o "cérebro" do programa, respondendo aos eventos.
    while (true) {
        // Verifica se a ISR do botão sinalizou um evento.
        if(btn_press) {
            // "Consome" o evento, reiniciando a flag para não processá-lo de novo.
            btn_press = false;

            // Verifica a variável de estado: só inicia a sequência se não houver
            // outra em andamento.
            if(!blinking) {
                // Marca o sistema como "ocupado".
                blinking = true;

                // --- Inicia os 3 Timers Independentes ---
                // Nota: O período é o tempo de um ciclo ON+OFF. O intervalo do
                // timer que inverte o LED deve ser METADE do período.
                
                // 1. Timer do LED amarelo: período 500ms
                add_repeating_timer_ms(500, repeating_timer_callback_am, NULL, &timer_am);
                
                // 2. Timer do LED azul: período 150ms
                add_repeating_timer_ms(150, repeating_timer_callback_az, NULL, &timer_az);

                // 3. Alarme de 5 segundos para chamar a função de limpeza.
                add_alarm_in_ms(5000, alarm_5s_callback, NULL, false);
            }
        }
    }
}