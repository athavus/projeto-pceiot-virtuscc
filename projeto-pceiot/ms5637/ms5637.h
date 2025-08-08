#ifndef MS5637_H
#define MS5637_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware/i2c.h"

// === CONFIGURAÇÃO DO HARDWARE ===
#define BAROMETRIC_I2C_PORT      i2c0
#define SENSOR_SDA_GPIO          4
#define SENSOR_SCL_GPIO          5
#define I2C_BAUDRATE             400000

// === ENDEREÇAMENTO ===
#define BAROMETRIC_SENSOR_ADDR   0x76

// === CÓDIGOS DE COMANDO ===
#define CMD_DEVICE_RESET         0x1E
#define CMD_PRESSURE_CONV_BASE   0x40
#define CMD_TEMP_CONV_BASE       0x50
#define CMD_ADC_READ             0x00
#define CMD_COEFF_READ_BASE      0xA0

// === PARÂMETROS DE CALIBRAÇÃO ===
#define CALIBRATION_CRC_POS      0
#define TOTAL_COEFFICIENTS       7

// === MODOS DE RESOLUÇÃO ===
typedef enum {
    RESOLUTION_LOW = 0,      // 256 samples
    RESOLUTION_MEDIUM_LOW,   // 512 samples  
    RESOLUTION_MEDIUM,       // 1024 samples
    RESOLUTION_MEDIUM_HIGH,  // 2048 samples
    RESOLUTION_HIGH,         // 4096 samples
    RESOLUTION_ULTRA_HIGH    // 8192 samples
} sensor_resolution_t;

// === CÓDIGOS DE STATUS ===
typedef enum {
    SENSOR_SUCCESS = 0,
    SENSOR_COMM_ERROR,
    SENSOR_CHECKSUM_FAIL
} sensor_result_t;

// === INTERFACE PÚBLICA ===
void barometric_sensor_setup(void);
sensor_result_t device_restart(void);
sensor_result_t get_barometric_readings(float *pressure_output);

#endif // MS5637_H