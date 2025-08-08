#ifndef MIC_ADC_H
#define MIC_ADC_H

#include <stdint.h>
#include <stdbool.h> // Para 'bool'

/**
 * @brief Inicializa o ADC na GPIO27 para leitura do microfone.
 * @return true se a inicialização for bem-sucedida, false caso contrário.
 */
bool mic_adc_init();

/**
 * @brief Lê o valor bruto do ADC do microfone.
 * @return O valor de 12 bits lido do ADC (0-4095).
 */
uint16_t mic_adc_read_raw();

/**
 * @brief Converte o valor bruto do ADC em milivolts.
 * @param raw_value O valor bruto de 12 bits lido do ADC.
 * @return O valor em milivolts.
 */
float mic_adc_raw_to_mv(uint16_t raw_value);

// --- Novas Funções Úteis ---

/**
 * @brief Realiza uma amostragem do ADC por um período de tempo e retorna a média.
 * @param sample_count O número de amostras a coletar.
 * @param delay_ms O atraso em milissegundos entre as amostras.
 * @return O valor médio de milivolts das amostras.
 */
float mic_adc_read_avg_mv(uint32_t sample_count, uint32_t delay_ms);

/**
 * @brief Lê o pico de amplitude (valor máximo) em milivolts durante um período.
 * @param duration_ms A duração em milissegundos para monitorar o pico.
 * @param sample_interval_ms O intervalo em milissegundos entre as amostras.
 * @return O valor máximo de milivolts detectado.
 */
float mic_adc_read_peak_mv(uint32_t duration_ms, uint32_t sample_interval_ms);

/**
 * @brief Configura um limiar de disparo (threshold) para detecção de som.
 * @param threshold_mv O valor em milivolts que o sinal deve exceder para disparar.
 */
void mic_adc_set_threshold_mv(float threshold_mv);

/**
 * @brief Verifica se o sinal do microfone excedeu o limiar configurado.
 * @return true se o sinal excedeu o limiar, false caso contrário.
 */
bool mic_adc_check_threshold_exceeded();

/**
 * @brief Coleta um buffer de amostras do ADC.
 * @param buffer Ponteiro para o array onde as amostras brutas serão armazenadas.
 * @param buffer_size O número de amostras a coletar.
 * @param sample_delay_us O atraso em microssegundos entre as amostras.
 */
void mic_adc_read_buffer(uint16_t *buffer, uint32_t buffer_size, uint32_t sample_delay_us);

#endif // MIC_ADC_H