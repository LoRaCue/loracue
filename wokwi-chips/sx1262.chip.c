#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// SX126X Commands
#define SX126X_CMD_GET_STATUS 0xC0
#define SX126X_CMD_WRITE_REGISTER 0x0D
#define SX126X_CMD_READ_REGISTER 0x1D
#define SX126X_CMD_WRITE_BUFFER 0x0E
#define SX126X_CMD_READ_BUFFER 0x1E
#define SX126X_CMD_SET_STANDBY 0x80
#define SX126X_CMD_SET_RX 0x82
#define SX126X_CMD_SET_TX 0x83
#define SX126X_CMD_SET_RF_FREQUENCY 0x86
#define SX126X_CMD_SET_PACKET_TYPE 0x8A
#define SX126X_CMD_SET_MODULATION_PARAMS 0x8B
#define SX126X_CMD_SET_PACKET_PARAMS 0x8C
#define SX126X_CMD_SET_TX_PARAMS 0x8E
#define SX126X_CMD_SET_BUFFER_BASE_ADDRESS 0x8F
#define SX126X_CMD_SET_PA_CONFIG 0x95
#define SX126X_CMD_SET_REGULATOR_MODE 0x96
#define SX126X_CMD_SET_DIO3_AS_TCXO_CTRL 0x97
#define SX126X_CMD_CALIBRATE_IMAGE 0x98
#define SX126X_CMD_CALIBRATE 0x89
#define SX126X_CMD_SET_DIO2_AS_RF_SWITCH_CTRL 0x9D
#define SX126X_CMD_STOP_TIMER_ON_PREAMBLE 0x9F
#define SX126X_CMD_SET_LORA_SYMB_NUM_TIMEOUT 0xA0
#define SX126X_CMD_GET_IRQ_STATUS 0x12
#define SX126X_CMD_CLEAR_IRQ_STATUS 0x02
#define SX126X_CMD_SET_DIO_IRQ_PARAMS 0x08
#define SX126X_CMD_GET_RX_BUFFER_STATUS 0x13
#define SX126X_CMD_GET_PACKET_STATUS 0x14
#define SX126X_CMD_GET_RSSI_INST 0x15
#define SX126X_CMD_SET_CAD_PARAMS 0x88
#define SX126X_CMD_SET_CAD 0xC5
#define SX126X_CMD_NOP 0x00

// Registers
#define SX126X_REG_LORA_SYNC_WORD_MSB 0x0740
#define SX126X_REG_OCP_CONFIGURATION 0x08E7
#define SX126X_REG_IQ_POLARITY_SETUP 0x0736

// Sync words
#define SX126X_SYNC_WORD_PUBLIC 0x3444

typedef struct {
  pin_t cs;
  spi_dev_t spi;
  uint8_t buffer[256];
  uint8_t cmd;
  uint8_t state;
  uint8_t tx_buffer[256];
  uint8_t tx_len;
  uint8_t rx_buffer[256];
  uint8_t rx_len;
  uint16_t irq_status;
  uint8_t registers[0x1000];
} chip_data_t;

static void chip_spi_done(void *user_data, uint8_t *buffer, uint32_t count);
static void chip_cs_change(void *user_data, pin_t pin, uint32_t value);

void chip_init(void) {
  chip_data_t *chip = malloc(sizeof(chip_data_t));
  memset(chip, 0, sizeof(chip_data_t));

  chip->cs = pin_init("CS", INPUT);
  
  const pin_watch_config_t cs_watch = {
    .user_data = chip,
    .edge = BOTH,
    .pin_change = chip_cs_change
  };
  pin_watch(chip->cs, &cs_watch);

  const spi_config_t spi_config = {
    .sck = pin_init("SCK", INPUT),
    .mosi = pin_init("MOSI", INPUT),
    .miso = pin_init("MISO", OUTPUT),
    .mode = 0,
    .done = chip_spi_done,
    .user_data = chip,
  };
  chip->spi = spi_init(&spi_config);

  // Initialize sync word register
  chip->registers[SX126X_REG_LORA_SYNC_WORD_MSB & 0xFFF] = 0x34;
  chip->registers[(SX126X_REG_LORA_SYNC_WORD_MSB + 1) & 0xFFF] = 0x44;

  printf("[sx1262] Initialized\n");
}

static void chip_cs_change(void *user_data, pin_t pin, uint32_t value) {
  chip_data_t *chip = (chip_data_t *)user_data;
  
  if (value == LOW) {
    spi_start(chip->spi, chip->buffer, sizeof(chip->buffer));
  } else {
    spi_stop(chip->spi);
  }
}

static void chip_spi_done(void *user_data, uint8_t *buffer, uint32_t count) {
  chip_data_t *chip = (chip_data_t *)user_data;
  
  if (count == 0 || pin_read(chip->cs) == HIGH) {
    return;
  }

  uint8_t cmd = buffer[0];
  
  switch (cmd) {
    case SX126X_CMD_GET_STATUS:
      buffer[1] = 0x22; // Idle mode
      break;

    case SX126X_CMD_READ_REGISTER:
      if (count >= 4) {
        uint16_t reg = (buffer[1] << 8) | buffer[2];
        uint8_t len = count - 4;
        for (int i = 0; i < len; i++) {
          buffer[4 + i] = chip->registers[(reg + i) & 0xFFF];
        }
      }
      break;

    case SX126X_CMD_WRITE_REGISTER:
      if (count >= 4) {
        uint16_t reg = (buffer[1] << 8) | buffer[2];
        uint8_t len = count - 3;
        for (int i = 0; i < len; i++) {
          chip->registers[(reg + i) & 0xFFF] = buffer[3 + i];
        }
      }
      buffer[1] = 0x22;
      break;

    case SX126X_CMD_WRITE_BUFFER:
      if (count >= 2) {
        chip->tx_len = count - 2;
        memcpy(chip->tx_buffer, &buffer[2], chip->tx_len);
        printf("[sx1262] TX buffer loaded (%d bytes): ", chip->tx_len);
        for (int i = 0; i < chip->tx_len; i++) {
          printf("%02X ", chip->tx_buffer[i]);
        }
        printf("\n");
      }
      buffer[1] = 0x22;
      break;

    case SX126X_CMD_READ_BUFFER:
      if (count >= 3) {
        uint8_t offset = buffer[1];
        uint8_t len = count - 3;
        for (int i = 0; i < len && (offset + i) < chip->rx_len; i++) {
          buffer[3 + i] = chip->rx_buffer[offset + i];
        }
      }
      break;

    case SX126X_CMD_SET_TX:
      printf("[sx1262] *** TRANSMITTING %d bytes ***\n", chip->tx_len);
      chip->irq_status |= 0x0001; // TX_DONE
      buffer[1] = 0x62; // TX mode
      break;

    case SX126X_CMD_SET_RX:
      chip->state = 0x50; // RX mode
      buffer[1] = 0x52;
      break;

    case SX126X_CMD_GET_IRQ_STATUS:
      buffer[1] = 0x22;
      buffer[2] = (chip->irq_status >> 8) & 0xFF;
      buffer[3] = chip->irq_status & 0xFF;
      break;

    case SX126X_CMD_CLEAR_IRQ_STATUS:
      chip->irq_status = 0;
      buffer[1] = 0x22;
      break;

    case SX126X_CMD_GET_RX_BUFFER_STATUS:
      buffer[1] = 0x22;
      buffer[2] = chip->rx_len;
      buffer[3] = 0; // offset
      break;

    default:
      buffer[1] = 0x22; // Status OK
      break;
  }

  if (pin_read(chip->cs) == LOW) {
    spi_start(chip->spi, chip->buffer, sizeof(chip->buffer));
  }
}
