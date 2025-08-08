# Monitor de Som com Alerta de Pressão Atmosférica

Este projeto implementa um sistema embarcado utilizando Raspberry Pi Pico para monitorar níveis de som ambiente e exibir alertas no display OLED quando picos acima de um certo limite são detectados.  
Além disso, o sistema mede a pressão atmosférica no momento do alerta, combinando dados acústicos e ambientais.

## Funcionalidades

- Medição contínua do som usando microfone analógico (ADC).
- Conversão de tensão (mV) para níveis de dB ajustados para sons ambientais.
- Detecção de picos sonoros e exibição de alerta visual no OLED.
- Leitura da pressão atmosférica no momento do alerta.
- Interface visual com:
  - Tela de boas-vindas.
  - Tela de alerta com barra de intensidade.
  - Tela de erro quando a leitura de pressão falha.
- Efeitos visuais de fade e inversão para destaque de alertas.

## Hardware Utilizado

- Placa: Raspberry Pi Pico.
- Display: OLED SSD1306 via I2C.
- Sensor de Pressão: MS5637 via I2C.
- Microfone Analógico: Conectado ao ADC do Pico.
- Resistores de pull-up para barramento I2C.

## Estrutura do Código

- `mv_to_db()` e `mv_to_db_scaled()` — Conversão de mV para decibéis.
- `display_welcome_screen()` — Tela inicial do sistema.
- `display_sound_alert()` — Alerta com nível de som e pressão.
- `display_error_screen()` — Mensagem de erro de leitura de pressão.
- Loop principal que:
  1. Lê pico de som (mV).
  2. Converte para dB.
  3. Verifica limite de detecção.
  4. Obtém pressão e mostra alerta.

## Limiares e Configurações

No código, os parâmetros principais são:

```c
#define SOUND_THRESHOLD_MV  1000.0f  // Limiar para detecção de som
#define ALERT_DURATION_MS   3000     // Tempo do alerta (ms)
#define MV_REFERENCE        100.0f   // Referência para cálculo de dB
```

Além disso, a detecção de alerta ocorre se `peak > 2800.0f`.

## Fluxo de Operação

1. Inicialização:
   - Configura I2C para OLED e sensor de pressão.
   - Inicializa ADC do microfone.
   - Exibe tela de boas-vindas.
   
2. Monitoramento Contínuo:
   - Mede o pico do som por 2 segundos.
   - Calcula dB ajustados para escala ambiente.

3. Evento de Alerta:
   - Caso o pico seja alto o suficiente:
     - Lê pressão atmosférica.
     - Mostra alerta visual com dados.
     - Aplica efeitos de destaque.
   - Em caso de falha na leitura de pressão:
     - Mostra tela de erro.

4. Retorno ao Estado Inicial.

## Como Compilar e Executar

1. Instale o Pico SDK e configure seu ambiente.
2. Baixe as dependências (`ssd1306`, `ms5637`, `mic_adc`).
3. Clone este repositório:
   ```bash
   git clone https://github.com/SEU_USUARIO/monitor-som-pressao.git
   ```
4. Compile o código.

5. Carregue o `.uf2` na Raspberry Pi Pico.

## Possíveis Melhorias

- Ajuste dinâmico de sensibilidade do microfone.
- Registro histórico dos eventos com data/hora.
- Integração com módulo Wi-Fi/Bluetooth para enviar alertas.
