#include "mic_adc.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include <stdio.h> // Para debug, pode ser removido em produção
#include <math.h>

// Define a GPIO que será usada para o ADC do microfone
#define MIC_ADC_GPIO 27
#define MIC_ADC_CHANNEL 1 // GPIO27 é o ADC Channel 1

// Define a voltagem de referência do ADC (Vref) da Pico W
#define ADC_VREF_MV 3300.0f // 3.3V em milivolts

// Define o número de bits do ADC (12 bits)
#define ADC_BITS 12
#define ADC_MAX_VALUE ((1 << ADC_BITS) - 1) // 2^12 - 1 = 4095

// Variável estática para armazenar o limiar (threshold)
static float mic_threshold_mv = 0.0f;

bool mic_adc_init() {
    adc_init();
    adc_gpio_init(MIC_ADC_GPIO);
    adc_select_input(MIC_ADC_CHANNEL);
    return true;
}

uint16_t mic_adc_read_raw() {
    return adc_read();
}

float mic_adc_raw_to_mv(uint16_t raw_value) {
    return (raw_value / (float)ADC_MAX_VALUE) * ADC_VREF_MV;
}

// --- Novas Implementações ---

float mic_adc_read_avg_mv(uint32_t sample_count, uint32_t delay_ms) {
    if (sample_count == 0) return 0.0f;

    float sum_mv = 0.0f;
    for (uint32_t i = 0; i < sample_count; ++i) {
        sum_mv += mic_adc_raw_to_mv(mic_adc_read_raw());
        if (delay_ms > 0) {
            sleep_ms(delay_ms);
        }
    }
    return sum_mv / sample_count;
}

float mic_adc_read_peak_mv(uint32_t duration_ms, uint32_t sample_interval_ms) {
    float peak_mv = 0.0f;
    uint32_t start_time = to_ms_since_boot(get_absolute_time());
    uint32_t current_time;

    do {
        float current_mv = mic_adc_raw_to_mv(mic_adc_read_raw());
        if (current_mv > peak_mv) {
            peak_mv = current_mv;
        }
        sleep_ms(sample_interval_ms);
        current_time = to_ms_since_boot(get_absolute_time());
    } while ((current_time - start_time) < duration_ms);

    return peak_mv;
}

void mic_adc_set_threshold_mv(float threshold_mv) {
    mic_threshold_mv = threshold_mv;
}

bool mic_adc_check_threshold_exceeded() {
    float current_mv = mic_adc_raw_to_mv(mic_adc_read_raw());
    // Considerando que o microfone tem um offset DC,
    // estamos interessados na variação do sinal (amplitude).
    // Uma forma simples é verificar o quão longe o sinal está do ponto médio (ADC_VREF_MV / 2).
    // Você pode precisar ajustar isso para o seu hardware de microfone específico.
    float mid_point_mv = ADC_VREF_MV / 2.0f;
    float deviation_mv = fabsf(current_mv - mid_point_mv); // Calcula a amplitude absoluta da variação

    return deviation_mv > mic_threshold_mv;
}

void mic_adc_read_buffer(uint16_t *buffer, uint32_t buffer_size, uint32_t sample_delay_us) {
    if (buffer == NULL || buffer_size == 0) return;

    for (uint32_t i = 0; i < buffer_size; ++i) {
        buffer[i] = mic_adc_read_raw();
        if (sample_delay_us > 0) {
            sleep_us(sample_delay_us);
        }
    }
}