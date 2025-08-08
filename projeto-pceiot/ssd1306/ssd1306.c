#include "ssd1306.h"
#include <string.h>
#include <stdlib.h>
#include "fonts.h"

// ============================================================================
// FUNÇÕES INTERNAS DE COMUNICAÇÃO I2C
// ============================================================================

/**
 * @brief Envia comando único para o controlador
 * @param device Estrutura do dispositivo
 * @param command_byte Comando a ser transmitido
 * @return Status da operação
 */
static inline bool transmit_command(oled_device_t *device, uint8_t command_byte) {
    uint8_t transmission_buffer[2] = {OLED_CMD_CONTROL_BYTE, command_byte};
    int transfer_result = i2c_write_blocking(device->i2c_interface, device->device_address, 
                                           transmission_buffer, 2, false);
    return (transfer_result == 2);
}

/**
 * @brief Transmite sequência de comandos consecutivos
 * @param device Estrutura do dispositivo
 * @param command_array Array de comandos
 * @param command_count Número de comandos
 */
static void transmit_command_sequence(oled_device_t *device, const uint8_t *command_array, size_t command_count) {
    for (size_t cmd_index = 0; cmd_index < command_count; cmd_index++) {
        transmit_command(device, command_array[cmd_index]);
    }
}

/**
 * @brief Transfere buffer de dados com otimização de performance
 * @param device Estrutura do dispositivo
 * @param data_ptr Ponteiro para os dados
 * @param data_length Tamanho dos dados em bytes
 */
static bool transfer_video_data(oled_device_t *device, const uint8_t *data_ptr, size_t data_length) {
    // Aloca buffer temporário para controle + dados
    uint8_t *transfer_buffer = malloc(data_length + 1);
    if (!transfer_buffer) return false;
    
    transfer_buffer[0] = OLED_DATA_CONTROL_BYTE;
    memcpy(&transfer_buffer[1], data_ptr, data_length);
    
    int transfer_result = i2c_write_blocking(device->i2c_interface, device->device_address, 
                                           transfer_buffer, data_length + 1, false);
    free(transfer_buffer);
    
    return (transfer_result == (int)(data_length + 1));
}

// ============================================================================
// IMPLEMENTAÇÃO DAS FUNÇÕES PÚBLICAS
// ============================================================================

bool oled_initialize_display(oled_device_t *device, i2c_inst_t *i2c_bus, uint8_t address) {
    // Configuração inicial da estrutura
    device->i2c_interface = i2c_bus;
    device->device_address = address;
    device->screen_width = OLED_SCREEN_WIDTH;
    device->screen_height = OLED_SCREEN_HEIGHT;
    device->power_state = true;
    device->contrast_level = OLED_DEFAULT_BRIGHTNESS;
    device->inverted_colors = false;
    
    // Limpa o buffer de vídeo
    oled_clear_screen(device);
    
    // Sequência de inicialização do controlador SSD1306
    const uint8_t startup_sequence[] = {
        CMD_DISPLAY_DEACTIVATE,                 // Desliga display temporariamente
        CMD_SET_OSC_FREQUENCY, 0x80,            // Frequência do oscilador
        CMD_SET_MULTIPLEX_RATIO, 0x3F,          // Razão de multiplex (64-1)
        CMD_SET_VERTICAL_OFFSET, 0x00,          // Offset vertical zero
        CMD_SET_DISPLAY_START_LINE | 0x00,      // Linha inicial zero
        CMD_CHARGE_PUMP_CONTROL, 0x14,          // Bomba de carga ativada
        CMD_MEMORY_ADDRESSING_MODE, 0x00,       // Endereçamento horizontal
        CMD_SEGMENT_REMAP_FLIPPED,              // Remapeamento de segmentos
        CMD_COM_SCAN_DESCENDING,                // Direção de varredura COM
        CMD_COM_PINS_CONFIG, 0x12,              // Configuração dos pinos COM
        CMD_SET_BRIGHTNESS, 0xCF,               // Contraste inicial
        CMD_PRECHARGE_PERIOD, 0xF1,             // Período de pré-carga
        CMD_VCOM_DETECTION_LEVEL, 0x40,         // Nível de detecção VCOM
        CMD_DISPLAY_FROM_RAM,                   // Exibir conteúdo da RAM
        CMD_DISPLAY_NORMAL,                     // Modo de exibição normal
        CMD_DISPLAY_ACTIVATE                    // Liga o display
    };
    
    transmit_command_sequence(device, startup_sequence, sizeof(startup_sequence));
    return true;
}

void oled_clear_screen(oled_device_t *device) {
    memset(device->video_memory, 0x00, OLED_VIDEO_BUFFER_SIZE);
}

void oled_fill_screen(oled_device_t *device, uint8_t fill_pattern) {
    memset(device->video_memory, fill_pattern, OLED_VIDEO_BUFFER_SIZE);
}

bool oled_refresh_screen(oled_device_t *device) {
    // Configura janela de escrita para tela completa
    const uint8_t display_window_config[] = {
        CMD_COLUMN_ADDRESS_RANGE, 0x00, 0x7F,  // Colunas 0-127
        CMD_PAGE_ADDRESS_RANGE, 0x00, 0x07     // Páginas 0-7
    };
    
    transmit_command_sequence(device, display_window_config, sizeof(display_window_config));
    
    // Transmite o buffer completo de vídeo
    return transfer_video_data(device, device->video_memory, OLED_VIDEO_BUFFER_SIZE);
}

void oled_draw_pixel(oled_device_t *device, uint8_t x_pos, uint8_t y_pos, bool pixel_on) {
    if (x_pos >= OLED_SCREEN_WIDTH || y_pos >= OLED_SCREEN_HEIGHT) return;
    
    // Calcula posição no buffer: página = y/8, bit = y%8
    uint16_t buffer_index = x_pos + (y_pos >> 3) * OLED_SCREEN_WIDTH;
    uint8_t bit_position = 1 << (y_pos & 0x07);
    
    if (pixel_on) {
        device->video_memory[buffer_index] |= bit_position;
    } else {
        device->video_memory[buffer_index] &= ~bit_position;
    }
}

bool oled_read_pixel(oled_device_t *device, uint8_t x_pos, uint8_t y_pos) {
    if (x_pos >= OLED_SCREEN_WIDTH || y_pos >= OLED_SCREEN_HEIGHT) return false;
    
    uint16_t buffer_index = x_pos + (y_pos >> 3) * OLED_SCREEN_WIDTH;
    uint8_t bit_position = 1 << (y_pos & 0x07);
    
    return (device->video_memory[buffer_index] & bit_position) != 0;
}

void oled_draw_line_segment(oled_device_t *device, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool line_color) {
    // Implementação do algoritmo de Bresenham otimizado
    int delta_x = abs((int)x2 - (int)x1);
    int delta_y = abs((int)y2 - (int)y1);
    int step_x = (x1 < x2) ? 1 : -1;
    int step_y = (y1 < y2) ? 1 : -1;
    int error_term = delta_x - delta_y;
    
    int current_x = x1, current_y = y1;
    
    while (true) {
        oled_draw_pixel(device, current_x, current_y, line_color);
        
        if (current_x == x2 && current_y == y2) break;
        
        int error_double = 2 * error_term;
        if (error_double > -delta_y) { 
            error_term -= delta_y; 
            current_x += step_x; 
        }
        if (error_double < delta_x) { 
            error_term += delta_x; 
            current_y += step_y; 
        }
    }
}

void oled_draw_filled_rectangle(oled_device_t *device, uint8_t origin_x, uint8_t origin_y, 
                               uint8_t rect_width, uint8_t rect_height, bool fill_color) {
    for (uint8_t row = 0; row < rect_height; row++) {
        for (uint8_t col = 0; col < rect_width; col++) {
            oled_draw_pixel(device, origin_x + col, origin_y + row, fill_color);
        }
    }
}

void oled_draw_rectangle_outline(oled_device_t *device, uint8_t origin_x, uint8_t origin_y,
                                uint8_t rect_width, uint8_t rect_height, bool border_color) {
    // Desenha as quatro bordas do retângulo
    oled_draw_line_segment(device, origin_x, origin_y, origin_x + rect_width - 1, origin_y, border_color);                    // Topo
    oled_draw_line_segment(device, origin_x, origin_y + rect_height - 1, origin_x + rect_width - 1, origin_y + rect_height - 1, border_color); // Base
    oled_draw_line_segment(device, origin_x, origin_y, origin_x, origin_y + rect_height - 1, border_color);                  // Esquerda
    oled_draw_line_segment(device, origin_x + rect_width - 1, origin_y, origin_x + rect_width - 1, origin_y + rect_height - 1, border_color);   // Direita
}

void oled_draw_filled_circle(oled_device_t *device, uint8_t center_x, uint8_t center_y,
                            uint8_t radius, bool fill_color) {
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x*x + y*y <= radius*radius) {
                oled_draw_pixel(device, center_x + x, center_y + y, fill_color);
            }
        }
    }
}

void oled_draw_circle_outline(oled_device_t *device, uint8_t center_x, uint8_t center_y,
                             uint8_t radius, bool border_color) {
    // Algoritmo do ponto médio para círculos
    int x = 0;
    int y = radius;
    int decision_param = 1 - radius;
    
    while (x <= y) {
        // Desenha os 8 pontos simétricos
        oled_draw_pixel(device, center_x + x, center_y + y, border_color);
        oled_draw_pixel(device, center_x + y, center_y + x, border_color);
        oled_draw_pixel(device, center_x - x, center_y + y, border_color);
        oled_draw_pixel(device, center_x - y, center_y + x, border_color);
        oled_draw_pixel(device, center_x + x, center_y - y, border_color);
        oled_draw_pixel(device, center_x + y, center_y - x, border_color);
        oled_draw_pixel(device, center_x - x, center_y - y, border_color);
        oled_draw_pixel(device, center_x - y, center_y - x, border_color);
        
        x++;
        if (decision_param < 0) {
            decision_param += 2 * x + 1;
        } else {
            y--;
            decision_param += 2 * (x - y) + 1;
        }
    }
}

void oled_render_character(oled_device_t *device, uint8_t char_x, uint8_t char_y, char ascii_char) {
    // Verifica se o caractere está no intervalo ASCII válido
    if (ascii_char < 32 || ascii_char > 126) return;
    
    const uint8_t *character_bitmap = FONT_MATRIX_6x8[ascii_char - 32];
    
    // Renderiza cada coluna do caractere
    for (int column = 0; column < 6; column++) {
        uint8_t column_pattern = character_bitmap[column];
        
        // Renderiza cada bit da coluna
        for (int bit = 0; bit < 8; bit++) {
            bool pixel_state = (column_pattern >> bit) & 0x01;
            oled_draw_pixel(device, char_x + column, char_y + bit, pixel_state);
        }
    }
}

void oled_render_text_string(oled_device_t *device, uint8_t text_x, uint8_t text_y, const char *text_string) {
    uint8_t cursor_position = text_x;
    
    while (*text_string && cursor_position < OLED_SCREEN_WIDTH) {
        oled_render_character(device, cursor_position, text_y, *text_string);
        cursor_position += 7; // 6 pixels do caractere + 1 espaçamento
        text_string++;
    }
}

void oled_render_highlighted_text(oled_device_t *device, uint8_t text_x, uint8_t text_y, const char *text_string) {
    uint8_t text_pixel_width = oled_calculate_text_width(text_string);
    
    // Desenha fundo de destaque
    oled_draw_filled_rectangle(device, text_x - 1, text_y - 1, text_pixel_width + 1, 10, true);
    
    // Renderiza texto "apagando" pixels para criar efeito invertido
    uint8_t cursor_position = text_x;
    while (*text_string && cursor_position < OLED_SCREEN_WIDTH) {
        if (*text_string >= 32 && *text_string <= 126) {
            const uint8_t *character_bitmap = FONT_MATRIX_6x8[*text_string - 32];
            
            for (int column = 0; column < 6; column++) {
                uint8_t column_pattern = character_bitmap[column];
                for (int bit = 0; bit < 8; bit++) {
                    if ((column_pattern >> bit) & 0x01) {
                        oled_draw_pixel(device, cursor_position + column, text_y + bit, false);
                    }
                }
            }
        }
        cursor_position += 7;
        text_string++;
    }
}

uint16_t oled_calculate_text_width(const char *text_string) {
    return strlen(text_string) * 7; // 6 pixels por caractere + 1 espaçamento
}

void oled_adjust_brightness(oled_device_t *device, uint8_t brightness_level) {
    device->contrast_level = brightness_level;
    transmit_command(device, CMD_SET_BRIGHTNESS);
    transmit_command(device, brightness_level);
}

void oled_toggle_color_inversion(oled_device_t *device, bool enable_inversion) {
    device->inverted_colors = enable_inversion;
    transmit_command(device, enable_inversion ? CMD_DISPLAY_INVERT : CMD_DISPLAY_NORMAL);
}

void oled_set_power_mode(oled_device_t *device, bool power_on) {
    device->power_state = power_on;
    transmit_command(device, power_on ? CMD_DISPLAY_ACTIVATE : CMD_DISPLAY_DEACTIVATE);
}

void oled_set_display_rotation(oled_device_t *device, bool flip_horizontal, bool flip_vertical) {
    transmit_command(device, flip_horizontal ? CMD_SEGMENT_REMAP_FLIPPED : CMD_SEGMENT_REMAP_NORMAL);
    transmit_command(device, flip_vertical ? CMD_COM_SCAN_DESCENDING : CMD_COM_SCAN_ASCENDING);
}

void oled_fade_effect(oled_device_t *device, bool fade_in, uint32_t duration_ms) {
    uint8_t steps = 20;
    uint32_t step_delay = duration_ms / steps;
    
    for (int i = 0; i < steps; i++) {
        uint8_t brightness = fade_in ? (i * 255 / steps) : (255 - (i * 255 / steps));
        oled_adjust_brightness(device, brightness);
        sleep_ms(step_delay);
    }
    
    // Restaura brilho original
    if (fade_in) {
        oled_adjust_brightness(device, device->contrast_level);
    }
}

void oled_horizontal_scroll(oled_device_t *device, bool scroll_left, uint16_t speed_ms, uint16_t distance_pixels) {
    // Implementação básica de scroll
    uint8_t backup_buffer[OLED_VIDEO_BUFFER_SIZE];
    
    for (uint16_t step = 0; step < distance_pixels; step++) {
        memcpy(backup_buffer, device->video_memory, OLED_VIDEO_BUFFER_SIZE);
        
        for (int page = 0; page < OLED_MEMORY_PAGES; page++) {
            for (int col = 0; col < OLED_SCREEN_WIDTH; col++) {
                int src_col = scroll_left ? (col + 1) % OLED_SCREEN_WIDTH : (col - 1 + OLED_SCREEN_WIDTH) % OLED_SCREEN_WIDTH;
                device->video_memory[page * OLED_SCREEN_WIDTH + col] = backup_buffer[page * OLED_SCREEN_WIDTH + src_col];
            }
        }
        
        oled_refresh_screen(device);
        sleep_ms(speed_ms);
    }
}