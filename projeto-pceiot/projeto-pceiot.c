#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <math.h>

#include "ssd1306/ssd1306.h"
#include "ms5637/ms5637.h"
#include "micro-adc/mic_adc.h"

// === CONFIGURAÇÕES ===
#define SOUND_THRESHOLD_MV  1000.0f  // Ajuste conforme sensibilidade do microfone
#define ALERT_DURATION_MS   3000    // Tempo de exibição do alerta (3 segundos)
#define MV_REFERENCE        100.0f   // Referência para cálculo de dB (ajuste conforme seu microfone)

// Instância global do display
static oled_device_t oled;

// Função corrigida para conversão mV para dB
float mv_to_db(float mv_value) {
    if (mv_value <= 0.0f) {
        return -60.0f; // Retorna um valor baixo para sinais muito fracos
    }
    
    // Calcula dB usando uma referência fixa
    // Para microfones, tipicamente usamos uma referência de 100mV = 0dB
    // Ajuste MV_REFERENCE conforme as características do seu microfone
    return 20.0f * log10f(mv_value / MV_REFERENCE);
}

// Função alternativa que mapeia mV para uma escala de dB mais intuitiva
float mv_to_db_scaled(float mv_value) {
    if (mv_value <= 0.0f) {
        return 30.0f;
    }
    
    // Mapeia 0-3300mV para 30-85dB de forma logarítmica
    // Escala mais realista para sons do ambiente
    float normalized = mv_value / 3300.0f; // Normaliza para 0-1
    if (normalized <= 0.001f) return 30.0f; // Mínimo de 30dB para ambiente silencioso
    
    // Fórmula ajustada: 30dB a 85dB (mais realista)
    return 30.0f + 55.0f * log10f(normalized * 9.0f + 1.0f) / log10f(10.0f);
}

void display_welcome_screen() {
    oled_clear_screen(&oled);
    
    // Desenha bordas decorativas
    oled_draw_rectangle_outline(&oled, 0, 0, 128, 64, true);
    oled_draw_rectangle_outline(&oled, 2, 2, 124, 60, true);
    
    // Título principal
    oled_render_text_string(&oled, 15, 8, "MONITOR DE SOM");
    
    // Linha decorativa
    oled_draw_line_segment(&oled, 10, 20, 118, 20, true);
    
    // Informações do sistema
    oled_render_text_string(&oled, 25, 28, "Sistema Ativo");
    oled_render_text_string(&oled, 20, 40, "Aguardando Som");
    
    // Indicador visual
    oled_draw_filled_circle(&oled, 64, 50, 3, true);
    
    oled_refresh_screen(&oled);
}

void display_sound_alert(float db_value, float pressure) {
    oled_clear_screen(&oled);
    
    // Efeito de alerta - bordas duplas piscantes
    oled_draw_filled_rectangle(&oled, 0, 0, 128, 64, true);
    oled_draw_filled_rectangle(&oled, 4, 4, 120, 56, false);
    
    // Desenha ícone de alerta (triângulo) na mesma linha do título
    oled_draw_line_segment(&oled, 6, 16, 11, 6, true);
    oled_draw_line_segment(&oled, 11, 6, 16, 16, true);
    oled_draw_line_segment(&oled, 6, 16, 16, 16, true);
    oled_render_character(&oled, 9, 11, '!');
    
    // Título de alerta alinhado à esquerda, após o símbolo
    oled_render_highlighted_text(&oled, 20, 6, "SOM ALTO!");
    
    // Informações em formato organizado, alinhadas à esquerda
    char db_buffer[20];
    char pressure_buffer[20];
    snprintf(db_buffer, sizeof(db_buffer), "%.1f dB", db_value);
    snprintf(pressure_buffer, sizeof(pressure_buffer), "%.1f mbar", pressure);
    
    // Labels e valores alinhados à esquerda
    oled_render_text_string(&oled, 6, 24, "Nivel:");
    oled_render_text_string(&oled, 50, 24, db_buffer);
    
    oled_render_text_string(&oled, 6, 34, "Press:");
    oled_render_text_string(&oled, 50, 34, pressure_buffer);
    
    // Barra de nível visual
    uint8_t bar_width = (uint8_t)((db_value - 30.0f) / 55.0f * 100.0f);
    if (bar_width > 100) bar_width = 100;
    
    oled_draw_rectangle_outline(&oled, 6, 45, 116, 8, true);
    if (bar_width > 0) {
        uint8_t actual_bar_width = (bar_width * 114) / 100; // Ajusta para o tamanho da barra
        oled_draw_filled_rectangle(&oled, 7, 46, actual_bar_width, 6, true);
    }
    
    oled_refresh_screen(&oled);
}

void display_error_screen() {
    oled_clear_screen(&oled);
    
    // Borda de erro
    oled_draw_rectangle_outline(&oled, 0, 0, 128, 64, true);
    
    // Ícone de erro (X) na mesma linha do título
    oled_draw_line_segment(&oled, 6, 15, 16, 25, true);
    oled_draw_line_segment(&oled, 16, 15, 6, 25, true);
    
    // Título alinhado à esquerda
    oled_render_text_string(&oled, 20, 15, "SOM ALTO!");
    oled_render_text_string(&oled, 6, 35, "Erro Pressao");
    
    // Linha de separação
    oled_draw_line_segment(&oled, 6, 30, 122, 30, true);
    
    oled_refresh_screen(&oled);
}

int main() {
    stdio_init_all();

    // === Inicializa I2C (para OLED e Sensor de Pressão) ===
    i2c_init(i2c0, 400000);
    gpio_set_function(4, GPIO_FUNC_I2C);
    gpio_set_function(5, GPIO_FUNC_I2C);
    gpio_pull_up(4);
    gpio_pull_up(5);

    // === Inicializa OLED ===
    if (!oled_initialize_display(&oled, i2c0, OLED_I2C_PRIMARY_ADDR)) {
        printf("Falha ao inicializar OLED\n");
        while (1);
    }
    
    // Configura brilho otimizado
    oled_adjust_brightness(&oled, OLED_DEFAULT_BRIGHTNESS);
    
    // Exibe tela de boas-vindas
    display_welcome_screen();

    // === Inicializa Sensor de Pressão ===
    barometric_sensor_setup();
    device_restart();

    // === Inicializa Microfone ===
    if (!mic_adc_init()) {
        printf("Falha ao inicializar ADC do microfone\n");
        while (1);
    }
    mic_adc_set_threshold_mv(SOUND_THRESHOLD_MV);

    sleep_ms(2000); // Mostra tela inicial por 2 segundos

    // === Loop Principal ===
    while (true) {
        float peak = mic_adc_read_peak_mv(2000, 10);
        float db_value = mv_to_db_scaled(peak);
        printf("Pico: %.3f mV | %.1f dB\n", peak, db_value);

        // Verifica se o pico está acima de 2800mV
        if (peak > 2800.0f) {
            float pressure;
            if (get_barometric_readings(&pressure) == SENSOR_SUCCESS) {
                // Efeito visual de alerta com fade
                oled_fade_effect(&oled, false, 200);
                
                display_sound_alert(db_value, pressure);
                
                // Efeito de inversão piscante
                for (int i = 0; i < 3; i++) {
                    oled_toggle_color_inversion(&oled, true);
                    oled_refresh_screen(&oled);
                    sleep_ms(300);
                    oled_toggle_color_inversion(&oled, false);
                    oled_refresh_screen(&oled);
                    sleep_ms(300);
                }
                
                // Mantém alerta por 3 segundos
                sleep_ms(ALERT_DURATION_MS - 1800); // Subtrai tempo do efeito piscante
                
                // Fade de volta para tela inicial
                oled_fade_effect(&oled, true, 500);
                display_welcome_screen();
                
            } else {
                display_error_screen();
                sleep_ms(2000);
                display_welcome_screen();
            }
        }
        
        sleep_ms(50); // debounce de detecção
    }
}