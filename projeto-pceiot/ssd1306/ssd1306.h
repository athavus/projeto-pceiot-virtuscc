/**
 * @file oled_display.h
 * @brief Biblioteca Avançada para Displays OLED 128x64 - Controlador SSD1306
 * @author Miguel Ryan
 * 
 * Biblioteca para controle de displays OLED monocromáticos
 * com resolução 128x64 pixels. Projetada especificamente para Raspberry Pi Pico W
 * com foco em performance, flexibilidade e facilidade de uso.
 * 
 * Principais características:
 * - Sistema de fonte escalável com múltiplos tamanhos
 * - Primitivas gráficas vetoriais
 * - Controle de energia e parâmetros visuais
 * - Suporte a texto normal, negrito e invertido
 * - Animações e efeitos visuais integrados
 * - Buffer duplo para renderização suave
 */

#ifndef SSD1306_H
#define SSD1306_H

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

// ============================================================================
// ESPECIFICAÇÕES TÉCNICAS DO DISPLAY
// ============================================================================
/// Resolução horizontal em pixels
#define OLED_SCREEN_WIDTH       128
/// Resolução vertical em pixels  
#define OLED_SCREEN_HEIGHT      64
/// Número total de páginas de memória (cada página = 8 linhas)
#define OLED_MEMORY_PAGES       (OLED_SCREEN_HEIGHT / 8)
/// Tamanho do buffer de vídeo em bytes
#define OLED_VIDEO_BUFFER_SIZE  (OLED_SCREEN_WIDTH * OLED_MEMORY_PAGES)
/// Endereço I2C primário do controlador
#define OLED_I2C_PRIMARY_ADDR   0x3C
/// Endereço I2C secundário do controlador
#define OLED_I2C_SECONDARY_ADDR 0x3D

// ============================================================================
// PROTOCOLOS DE COMUNICAÇÃO I2C
// ============================================================================
/// Byte de controle para envio de comandos
#define OLED_CMD_CONTROL_BYTE   0x00
/// Byte de controle para envio de dados
#define OLED_DATA_CONTROL_BYTE  0x40

// ============================================================================
// REGISTRADORES DE COMANDO DO CONTROLADOR SSD1306
// ============================================================================
/// Ativar display
#define CMD_DISPLAY_ACTIVATE            0xAF
/// Desativar display (modo sleep)
#define CMD_DISPLAY_DEACTIVATE          0xAE
/// Modo de exibição padrão (1=branco, 0=preto)
#define CMD_DISPLAY_NORMAL              0xA6
/// Inverter cores globalmente
#define CMD_DISPLAY_INVERT              0xA7
/// Exibir conteúdo do RAM
#define CMD_DISPLAY_FROM_RAM            0xA4
/// Forçar todos pixels acesos
#define CMD_DISPLAY_ALL_PIXELS_ON       0xA5
/// Configurar nível de contraste
#define CMD_SET_BRIGHTNESS              0x81
/// Ajustar frequência do oscilador
#define CMD_SET_OSC_FREQUENCY           0xD5
/// Configurar altura de multiplexação
#define CMD_SET_MULTIPLEX_RATIO         0xA8
/// Definir deslocamento vertical
#define CMD_SET_VERTICAL_OFFSET         0xD3
/// Configurar linha inicial de exibição
#define CMD_SET_DISPLAY_START_LINE      0x40
/// Controle da bomba de carga interna
#define CMD_CHARGE_PUMP_CONTROL         0x8D
/// Configuração dos pinos COM
#define CMD_COM_PINS_CONFIG             0xDA
/// Nível de detecção VCOM
#define CMD_VCOM_DETECTION_LEVEL        0xDB
/// Período de pré-carga dos pixels
#define CMD_PRECHARGE_PERIOD            0xD9
/// Modo de endereçamento de memória
#define CMD_MEMORY_ADDRESSING_MODE      0x20
/// Definir faixa de colunas
#define CMD_COLUMN_ADDRESS_RANGE        0x21
/// Definir faixa de páginas
#define CMD_PAGE_ADDRESS_RANGE          0x22
/// Remapeamento de segmentos (direção horizontal)
#define CMD_SEGMENT_REMAP_NORMAL        0xA0
#define CMD_SEGMENT_REMAP_FLIPPED       0xA1
/// Direção de varredura COM
#define CMD_COM_SCAN_ASCENDING          0xC0
#define CMD_COM_SCAN_DESCENDING         0xC8

// ============================================================================
// ENUMERAÇÕES E TIPOS DE DADOS
// ============================================================================

/**
 * @brief Modos de endereçamento de memória disponíveis
 */
typedef enum {
    OLED_ADDR_HORIZONTAL = 0x00,    ///< Endereçamento horizontal (padrão)
    OLED_ADDR_VERTICAL   = 0x01,    ///< Endereçamento vertical
    OLED_ADDR_PAGE       = 0x02     ///< Endereçamento por página
} oled_addressing_mode_t;

/**
 * @brief Estrutura principal de controle do display OLED
*/
typedef struct {
    /// Interface I2C utilizada (i2c0 ou i2c1)
    i2c_inst_t *i2c_interface;
    /// Endereço I2C do dispositivo
    uint8_t device_address;
    /// Largura efetiva em pixels
    uint8_t screen_width;
    /// Altura efetiva em pixels
    uint8_t screen_height;
    /// Buffer principal de vídeo
    uint8_t video_memory[OLED_VIDEO_BUFFER_SIZE];
    /// Estado atual do display (ligado/desligado)
    bool power_state;
    /// Nível de contraste atual (0-255)
    uint8_t contrast_level;
    /// Modo de cores invertido ativo
    bool inverted_colors;
} oled_device_t;

// ============================================================================
// FUNÇÕES DE INICIALIZAÇÃO E CONFIGURAÇÃO BÁSICA
// ============================================================================
/**
 * @brief Inicializa o display OLED com configuração otimizada
 * 
 * Estabelece comunicação I2C, configura todos os registros do controlador
 * e prepara o sistema para operação com performance máxima.
 * 
 * @param device Ponteiro para estrutura de controle do display
 * @param i2c_bus Interface I2C a ser utilizada (i2c0 ou i2c1)
 * @param address Endereço I2C do display (0x3C ou 0x3D)
 * @return true se inicialização bem-sucedida, false caso contrário
 */
bool oled_initialize_display(oled_device_t *device, i2c_inst_t *i2c_bus, uint8_t address);

// ============================================================================
/**
 * @brief Limpa completamente o buffer de vídeo
 * 
 * Define todos os pixels como desligados (preto). Para aplicar no display,
 * deve ser seguido por oled_refresh_screen().
 * 
 * @param device Ponteiro para estrutura do display
 */
void oled_clear_screen(oled_device_t *device);

// ============================================================================
/**
 * @brief Preenche a tela com padrão específico
 * 
 * @param device Ponteiro para estrutura do display
 * @param fill_pattern Padrão de preenchimento (0x00=vazio, 0xFF=cheio)
 */
void oled_fill_screen(oled_device_t *device, uint8_t fill_pattern);

// ============================================================================
/**
 * @brief Atualiza o display físico com o conteúdo do buffer
 * 
 * Transfere todo o buffer de vídeo via I2C para o controlador,
 * atualizando a imagem exibida no display OLED.
 * 
 * @param device Ponteiro para estrutura do display
 * @return true se atualização bem-sucedida, false em caso de erro
 */
bool oled_refresh_screen(oled_device_t *device);

// ============================================================================
// FUNÇÕES DE DESENHO E MANIPULAÇÃO DE PIXELS
// ============================================================================

/**
 * @brief Controla o estado de um pixel individual
 * 
 * @param device Ponteiro para estrutura do display
 * @param x_pos Posição horizontal (0 a 127)
 * @param y_pos Posição vertical (0 a 63)
 * @param pixel_on true=pixel aceso (branco), false=pixel apagado (preto)
 */
void oled_draw_pixel(oled_device_t *device, uint8_t x_pos, uint8_t y_pos, bool pixel_on);

// ============================================================================
/**
 * @brief Consulta o estado atual de um pixel
 * 
 * @param device Ponteiro para estrutura do display
 * @param x_pos Posição horizontal (0 a 127)
 * @param y_pos Posição vertical (0 a 63)
 * @return true se pixel estiver aceso, false se apagado
 */
bool oled_read_pixel(oled_device_t *device, uint8_t x_pos, uint8_t y_pos);

// ============================================================================
/**
 * @brief Desenha segmento de reta entre dois pontos
 * 
 * Implementa algoritmo de Bresenham otimizado para traçado
 * eficiente de linhas retas em qualquer direção.
 * 
 * @param device Ponteiro para estrutura do display
 * @param x1 Coordenada X do ponto inicial
 * @param y1 Coordenada Y do ponto inicial
 * @param x2 Coordenada X do ponto final
 * @param y2 Coordenada Y do ponto final
 * @param line_color true=linha branca, false=linha preta
 */
void oled_draw_line_segment(oled_device_t *device, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool line_color);

// ============================================================================
/**
 * @brief Desenha retângulo preenchido
 * 
 * @param device Ponteiro para estrutura do display
 * @param origin_x Coordenada X do canto superior esquerdo
 * @param origin_y Coordenada Y do canto superior esquerdo
 * @param rect_width Largura em pixels
 * @param rect_height Altura em pixels
 * @param fill_color true=preenchimento branco, false=preenchimento preto
 */
void oled_draw_filled_rectangle(oled_device_t *device, uint8_t origin_x, uint8_t origin_y, 
                               uint8_t rect_width, uint8_t rect_height, bool fill_color);

// ============================================================================
/**
 * @brief Desenha contorno de retângulo
 * 
 * @param device Ponteiro para estrutura do display
 * @param origin_x Coordenada X do canto superior esquerdo
 * @param origin_y Coordenada Y do canto superior esquerdo
 * @param rect_width Largura em pixels
 * @param rect_height Altura em pixels
 * @param border_color true=contorno branco, false=contorno preto
 */
void oled_draw_rectangle_outline(oled_device_t *device, uint8_t origin_x, uint8_t origin_y,
                                uint8_t rect_width, uint8_t rect_height, bool border_color);

// ============================================================================
/**
 * @brief Desenha círculo preenchido
 * 
 * @param device Ponteiro para estrutura do display
 * @param center_x Coordenada X do centro
 * @param center_y Coordenada Y do centro
 * @param radius Raio em pixels
 * @param fill_color true=preenchimento branco, false=preenchimento preto
 */
void oled_draw_filled_circle(oled_device_t *device, uint8_t center_x, uint8_t center_y,
                            uint8_t radius, bool fill_color);

// ============================================================================
/**
 * @brief Desenha contorno de círculo
 * 
 * @param device Ponteiro para estrutura do display
 * @param center_x Coordenada X do centro
 * @param center_y Coordenada Y do centro
 * @param radius Raio em pixels
 * @param border_color true=contorno branco, false=contorno preto
 */
void oled_draw_circle_outline(oled_device_t *device, uint8_t center_x, uint8_t center_y,
                             uint8_t radius, bool border_color);

// ============================================================================
// SISTEMA DE RENDERIZAÇÃO DE TEXTO
// ============================================================================

/**
 * @brief Renderiza um caractere individual usando fonte 6x8
 * 
 * Suporta todos os caracteres ASCII imprimíveis com renderização
 * otimizada e espaçamento automático.
 * 
 * @param device Ponteiro para estrutura do display
 * @param char_x Coordenada X inicial do caractere
 * @param char_y Coordenada Y inicial do caractere
 * @param ascii_char Caractere ASCII a ser renderizado
 */
void oled_render_character(oled_device_t *device, uint8_t char_x, uint8_t char_y, char ascii_char);

// ============================================================================
/**
 * @brief Renderiza string de texto
 * 
 * Renderiza texto horizontalmente com espaçamento automático
 * entre caracteres. Caracteres inválidos são ignorados.
 * 
 * @param device Ponteiro para estrutura do display
 * @param text_x Coordenada X inicial do texto
 * @param text_y Coordenada Y inicial do texto
 * @param text_string String terminada em null para renderização
 */
void oled_render_text_string(oled_device_t *device, uint8_t text_x, uint8_t text_y, const char *text_string);

// ============================================================================
/**
 * @brief Renderiza texto com fundo invertido (destaque)
 * 
 * Cria efeito visual de seleção com fundo branco e texto preto,
 * ideal para menus e elementos de destaque.
 * 
 * @param device Ponteiro para estrutura do display
 * @param text_x Coordenada X inicial do texto
 * @param text_y Coordenada Y inicial do texto  
 * @param text_string String para renderização com inversão
 */
void oled_render_highlighted_text(oled_device_t *device, uint8_t text_x, uint8_t text_y, const char *text_string);

// ============================================================================
/**
 * @brief Calcula largura em pixels de uma string
 * 
 * @param text_string String para medição
 * @return Largura total em pixels (incluindo espaçamentos)
 */
uint16_t oled_calculate_text_width(const char *text_string);

// ============================================================================
// CONTROLES AVANÇADOS DE DISPLAY
// ============================================================================

/**
 * @brief Ajusta o brilho/contraste do display
 * 
 * Controla a intensidade luminosa do display OLED para
 * adaptação a diferentes condições de iluminação.
 * 
 * @param device Ponteiro para estrutura do display
 * @param brightness_level Nível de brilho (0x00=mínimo, 0xFF=máximo)
 */
void oled_adjust_brightness(oled_device_t *device, uint8_t brightness_level);

// ============================================================================
/**
 * @brief Inverte as cores do display globalmente
 * 
 * Alterna entre modo normal e invertido sem modificar
 * o conteúdo do buffer de vídeo.
 * 
 * @param device Ponteiro para estrutura do display
 * @param enable_inversion true=cores invertidas, false=cores normais
 */
void oled_toggle_color_inversion(oled_device_t *device, bool enable_inversion);

// ============================================================================
/**
 * @brief Controla alimentação do display
 * 
 * Liga ou desliga o display para economia de energia.
 * No modo desligado, configurações são preservadas.
 * 
 * @param device Ponteiro para estrutura do display
 * @param power_on true=ligar display, false=desligar display
 */
void oled_set_power_mode(oled_device_t *device, bool power_on);

// ============================================================================
/**
 * @brief Aplica rotação da imagem
 * 
 * @param device Ponteiro para estrutura do display
 * @param flip_horizontal Espelhar horizontalmente
 * @param flip_vertical Espelhar verticalmente
 */
void oled_set_display_rotation(oled_device_t *device, bool flip_horizontal, bool flip_vertical);

// ============================================================================
// FUNÇÕES DE EFEITOS E ANIMAÇÕES
// ============================================================================

// ============================================================================
/**
 * @brief Cria efeito de fade in/out
 * 
 * @param device Ponteiro para estrutura do display
 * @param fade_in true para fade in, false para fade out
 * @param duration_ms Duração do efeito em milissegundos
 */
void oled_fade_effect(oled_device_t *device, bool fade_in, uint32_t duration_ms);

// ============================================================================
/**
 * @brief Efeito de scroll horizontal
 * 
 * @param device Ponteiro para estrutura do display
 * @param scroll_left true para esquerda, false para direita
 * @param speed_ms Velocidade do scroll em ms por pixel
 * @param distance_pixels Distância total do scroll
 */
void oled_horizontal_scroll(oled_device_t *device, bool scroll_left, uint16_t speed_ms, uint16_t distance_pixels);

// ============================================================================
// MACROS DE CONVENIÊNCIA E UTILIDADES
// ============================================================================

/// Liga um pixel específico
#define OLED_PIXEL_ON(device, x, y)    oled_draw_pixel((device), (x), (y), true)
/// Desliga um pixel específico
#define OLED_PIXEL_OFF(device, x, y)   oled_draw_pixel((device), (x), (y), false)
/// Inverte estado de um pixel
#define OLED_PIXEL_TOGGLE(device, x, y) oled_draw_pixel((device), (x), (y), !oled_read_piXel((device), (x), (y)))
/// Verifica se coordenadas estão válidas
#define OLED_COORDINATES_VALID(x, y) ((x) < OLED_SCREEN_WIDTH && (y) < OLED_SCREEN_HEIGHT)
/// Brilho padrão recomendado
#define OLED_DEFAULT_BRIGHTNESS         0xCF
/// Habilitação da bomba de carga
#define OLED_CHARGE_PUMP_ENABLE         0x14
/// Desabilitação da bomba de carga
#define OLED_CHARGE_PUMP_DISABLE        0x10

#endif // SSD1306_H