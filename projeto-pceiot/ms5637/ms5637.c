/**
 * @file ms5637.c
 * @brief Driver para sensor barométrico MS5637 usando Raspberry Pi Pico
 * @author Sistema Embarcado
 * @version 2.0
 * @date 2024
 */

#include "ms5637.h"
#include "pico/stdlib.h"
#include <math.h>
#include <stdio.h>

// === VARIÁVEIS GLOBAIS ===
static uint16_t calibration_data[8];
static sensor_resolution_t active_resolution = RESOLUTION_ULTRA_HIGH;

// Tabela de tempos de conversão por resolução (em milissegundos)
static const uint8_t conversion_delays[] = {1, 2, 3, 5, 9, 17};

// === IMPLEMENTAÇÕES INTERNAS ===

/**
 * @brief Executa leitura de valor ADC após conversão
 * @param adc_value Ponteiro para armazenar valor lido
 * @return Status da operação
 */
static sensor_result_t fetch_adc_data(uint32_t *adc_value) {
    uint8_t read_cmd = CMD_ADC_READ;
    uint8_t buffer[3];
    
    if (i2c_write_blocking(BAROMETRIC_I2C_PORT, BAROMETRIC_SENSOR_ADDR, &read_cmd, 1, true) != 1)
        return SENSOR_COMM_ERROR;
    
    if (i2c_read_blocking(BAROMETRIC_I2C_PORT, BAROMETRIC_SENSOR_ADDR, buffer, 3, false) != 3)
        return SENSOR_COMM_ERROR;
    
    *adc_value = (buffer[0] << 16) | (buffer[1] << 8) | buffer[2];
    return SENSOR_SUCCESS;
}

/**
 * @brief Inicia processo de conversão no sensor
 * @param conversion_cmd Comando específico de conversão
 * @return Status da operação  
 */
static sensor_result_t trigger_conversion(uint8_t conversion_cmd) {
    return i2c_write_blocking(BAROMETRIC_I2C_PORT, BAROMETRIC_SENSOR_ADDR, &conversion_cmd, 1, false) == 1
           ? SENSOR_SUCCESS : SENSOR_COMM_ERROR;
}

/**
 * @brief Calcula checksum CRC4 dos coeficientes de calibração
 * @param coeff_array Array com coeficientes
 * @return Valor CRC4 calculado
 */
static uint8_t compute_crc4_checksum(uint16_t *coeff_array) {
    uint16_t remainder = 0x00;
    coeff_array[0] &= 0x0FFF;
    coeff_array[7] = 0;
    
    for (int bit_pos = 0; bit_pos < 16; bit_pos++) {
        if (bit_pos % 2 == 1)
            remainder ^= coeff_array[bit_pos >> 1] & 0x00FF;
        else
            remainder ^= coeff_array[bit_pos >> 1] >> 8;
            
        for (int shift_count = 8; shift_count > 0; shift_count--) {
            if (remainder & 0x8000)
                remainder = (remainder << 1) ^ 0x3000;
            else
                remainder <<= 1;
        }
    }
    return (remainder >> 12) & 0xF;
}

/**
 * @brief Lê coeficientes de calibração da PROM do sensor
 * @return Status da leitura e validação
 */
static sensor_result_t load_calibration_coefficients(void) {
    for (int coeff_index = 0; coeff_index < 8; coeff_index++) {
        uint8_t read_addr = CMD_COEFF_READ_BASE + (coeff_index * 2);
        uint8_t raw_data[2];
        
        if (i2c_write_blocking(BAROMETRIC_I2C_PORT, BAROMETRIC_SENSOR_ADDR, &read_addr, 1, true) != 1)
            return SENSOR_COMM_ERROR;
            
        if (i2c_read_blocking(BAROMETRIC_I2C_PORT, BAROMETRIC_SENSOR_ADDR, raw_data, 2, false) != 2)
            return SENSOR_COMM_ERROR;
            
        calibration_data[coeff_index] = (raw_data[0] << 8) | raw_data[1];
    }
    
    if (compute_crc4_checksum(calibration_data) != (calibration_data[0] >> 12))
        return SENSOR_CHECKSUM_FAIL;
        
    return SENSOR_SUCCESS;
}

// === INTERFACE PÚBLICA ===

/**
 * @brief Reinicia o dispositivo sensor
 * @return Status da operação de reset
 */
sensor_result_t device_restart(void) {
    uint8_t reset_cmd = CMD_DEVICE_RESET;
    return i2c_write_blocking(BAROMETRIC_I2C_PORT, BAROMETRIC_SENSOR_ADDR, &reset_cmd, 1, false) == 1
           ? SENSOR_SUCCESS : SENSOR_COMM_ERROR;
}

/**
 * @brief Configuração inicial do sistema sensor
 */
void barometric_sensor_setup(void) {
    // Inicialização da comunicação I2C
    i2c_init(BAROMETRIC_I2C_PORT, I2C_BAUDRATE);
    gpio_set_function(SENSOR_SDA_GPIO, GPIO_FUNC_I2C);
    gpio_set_function(SENSOR_SCL_GPIO, GPIO_FUNC_I2C);
    gpio_pull_up(SENSOR_SDA_GPIO);
    gpio_pull_up(SENSOR_SCL_GPIO);

    // Reset inicial e carregamento dos coeficientes
    device_restart();
    sleep_ms(20);
    load_calibration_coefficients();
}

/**
 * @brief Realiza leitura de pressão barométrica
 * @param pressure_output Ponteiro para valor de pressão em mbar
 * @return Status da operação de leitura
 */
sensor_result_t get_barometric_readings(float *pressure_output) {
    uint32_t raw_pressure = 0, raw_temperature = 0;
    int32_t delta_temp, calculated_temp;
    int64_t offset_value, sensitivity_value, final_pressure;
    int64_t offset_correction = 0, sens_correction = 0;

    // === CONVERSÃO DE TEMPERATURA (necessária para compensação) ===
    uint8_t temp_cmd = CMD_TEMP_CONV_BASE + (active_resolution * 2);
    if (trigger_conversion(temp_cmd) != SENSOR_SUCCESS) 
        return SENSOR_COMM_ERROR;
    
    sleep_ms(conversion_delays[active_resolution]);
    
    if (fetch_adc_data(&raw_temperature) != SENSOR_SUCCESS) 
        return SENSOR_COMM_ERROR;

    // === CONVERSÃO DE PRESSÃO ===
    uint8_t pressure_cmd = CMD_PRESSURE_CONV_BASE + (active_resolution * 2);
    if (trigger_conversion(pressure_cmd) != SENSOR_SUCCESS) 
        return SENSOR_COMM_ERROR;
    
    sleep_ms(conversion_delays[active_resolution]);
    
    if (fetch_adc_data(&raw_pressure) != SENSOR_SUCCESS) 
        return SENSOR_COMM_ERROR;

    // === CÁLCULOS DE COMPENSAÇÃO ===
    
    // Diferença de temperatura (necessária para correção da pressão)
    delta_temp = raw_temperature - ((int32_t)calibration_data[5] << 8);
    calculated_temp = 2000 + ((int64_t)delta_temp * calibration_data[6]) / 8388608;

    // Cálculo de offset e sensibilidade
    offset_value = ((int64_t)calibration_data[2] << 17) + ((int64_t)calibration_data[4] * delta_temp) / 64;
    sensitivity_value = ((int64_t)calibration_data[1] << 16) + ((int64_t)calibration_data[3] * delta_temp) / 128;

    // Aplicação de correções para baixas temperaturas
    if (calculated_temp < 2000) {
        offset_correction = 5 * ((calculated_temp - 2000) * (calculated_temp - 2000)) / 2;
        sens_correction = 5 * ((calculated_temp - 2000) * (calculated_temp - 2000)) / 4;
        
        if (calculated_temp < -1500) {
            offset_correction += 7 * ((calculated_temp + 1500) * (calculated_temp + 1500));
            sens_correction += (11 * ((calculated_temp + 1500) * (calculated_temp + 1500))) / 2;
        }
    }

    // Aplicação das correções
    offset_value -= offset_correction;
    sensitivity_value -= sens_correction;

    // Cálculo final da pressão
    final_pressure = (((raw_pressure * sensitivity_value) >> 21) - offset_value) >> 15;

    // Conversão para unidades finais (mbar)
    *pressure_output = final_pressure / 100.0f;

    return SENSOR_SUCCESS;
}