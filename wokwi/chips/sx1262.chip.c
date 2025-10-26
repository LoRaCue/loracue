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

// Debug helpers
static const char* cmd_name(uint8_t cmd) {
  switch(cmd) {
    case SX126X_CMD_GET_STATUS: return "GET_STATUS";
    case SX126X_CMD_WRITE_REGISTER: return "WRITE_REGISTER";
    case SX126X_CMD_READ_REGISTER: return "READ_REGISTER";
    case SX126X_CMD_WRITE_BUFFER: return "WRITE_BUFFER";
    case SX126X_CMD_READ_BUFFER: return "READ_BUFFER";
    case SX126X_CMD_SET_STANDBY: return "SET_STANDBY";
    case SX126X_CMD_SET_RX: return "SET_RX";
    case SX126X_CMD_SET_TX: return "SET_TX";
    case SX126X_CMD_SET_RF_FREQUENCY: return "SET_RF_FREQUENCY";
    case SX126X_CMD_SET_PACKET_TYPE: return "SET_PACKET_TYPE";
    case SX126X_CMD_SET_MODULATION_PARAMS: return "SET_MODULATION_PARAMS";
    case SX126X_CMD_SET_PACKET_PARAMS: return "SET_PACKET_PARAMS";
    case SX126X_CMD_SET_TX_PARAMS: return "SET_TX_PARAMS";
    case SX126X_CMD_SET_BUFFER_BASE_ADDRESS: return "SET_BUFFER_BASE_ADDRESS";
    case SX126X_CMD_SET_PA_CONFIG: return "SET_PA_CONFIG";
    case SX126X_CMD_SET_REGULATOR_MODE: return "SET_REGULATOR_MODE";
    case SX126X_CMD_SET_DIO3_AS_TCXO_CTRL: return "SET_DIO3_AS_TCXO_CTRL";
    case SX126X_CMD_CALIBRATE_IMAGE: return "CALIBRATE_IMAGE";
    case SX126X_CMD_CALIBRATE: return "CALIBRATE";
    case SX126X_CMD_SET_DIO2_AS_RF_SWITCH_CTRL: return "SET_DIO2_AS_RF_SWITCH_CTRL";
    case SX126X_CMD_STOP_TIMER_ON_PREAMBLE: return "STOP_TIMER_ON_PREAMBLE";
    case SX126X_CMD_SET_LORA_SYMB_NUM_TIMEOUT: return "SET_LORA_SYMB_NUM_TIMEOUT";
    case SX126X_CMD_CLEAR_IRQ_STATUS: return "CLEAR_IRQ_STATUS";
    case SX126X_CMD_GET_IRQ_STATUS: return "GET_IRQ_STATUS";
    case SX126X_CMD_SET_DIO_IRQ_PARAMS: return "SET_DIO_IRQ_PARAMS";
    default: return "UNKNOWN";
  }
}

static const char* state_name(uint8_t state) {
  switch(state) {
    case 0x02: return "STANDBY_RC";
    case 0x03: return "STANDBY_XOSC";
    case 0x05: return "RX";
    case 0x06: return "TX";
    default: return "UNKNOWN";
  }
}

typedef struct {
  pin_t cs;
  pin_t busy;
  spi_dev_t spi;
  uint8_t spi_buffer[1];  // Single byte buffer for byte-by-byte SPI
  uint8_t cmd_buffer[256]; // Accumulate command bytes
  uint8_t cmd_pos;
  uint8_t cmd;
  uint8_t state;
  uint8_t tx_buffer[256];
  uint8_t tx_len;
  uint8_t rx_buffer[256];
  uint8_t rx_len;
  uint16_t irq_status;
  uint8_t registers[0x1000];
} chip_data_t;

static uint8_t get_status_byte(chip_data_t *chip) {
  // SX126x status byte: [chip_mode:3][cmd_status:3][reserved:2]
  // chip_mode: 2=STBY_RC, 3=STBY_XOSC, 4=FS, 5=RX, 6=TX
  // cmd_status: 1=success, 2=data available, 3=timeout, 4=error, 5=exec_fail, 6=tx_done
  uint8_t chip_mode = chip->state;
  uint8_t cmd_status = 0x02; // Command processed successfully
  return (chip_mode << 4) | (cmd_status << 1);
}

static void chip_spi_done(void *user_data, uint8_t *buffer, uint32_t count);
static void chip_cs_change(void *user_data, pin_t pin, uint32_t value);

void chip_init(void) {
  chip_data_t *chip = malloc(sizeof(chip_data_t));
  if (!chip) return;
  memset(chip, 0, sizeof(chip_data_t));

  chip->cs = pin_init("CS", INPUT);
  chip->busy = pin_init("BUSY", OUTPUT);
  
  // BUSY starts LOW (ready)
  pin_write(chip->busy, LOW);
  
  // Initialize to STANDBY mode
  chip->state = 0x02; // STANDBY_RC
  
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

  printf("Initialized\n");
}

static void chip_cs_change(void *user_data, pin_t pin, uint32_t value) {
  chip_data_t *chip = (chip_data_t *)user_data;
  
  if (value == LOW) {
    // Start new transaction
    chip->cmd_pos = 0;
    chip->spi_buffer[0] = 0x00; // Dummy byte for first transfer
    spi_start(chip->spi, chip->spi_buffer, 1);
  } else {
    spi_stop(chip->spi);
  }
}

static void chip_spi_done(void *user_data, uint8_t *buffer, uint32_t count) {
  chip_data_t *chip = (chip_data_t *)user_data;
  
  if (count == 0 || pin_read(chip->cs) == HIGH) {
    return;
  }

  // Store received byte
  chip->cmd_buffer[chip->cmd_pos] = buffer[0];
  chip->cmd_pos++;

  // Prepare next byte to send
  uint8_t next_byte = 0x00;

  if (chip->cmd_pos == 1) {
    // First byte is command, next byte is status
    chip->cmd = chip->cmd_buffer[0];
    
    // Only log GET_IRQ_STATUS if IRQ is non-zero (reduce spam)
    if (chip->cmd != SX126X_CMD_GET_IRQ_STATUS || chip->irq_status != 0) {
      printf("→ %s (0x%02X)\n", cmd_name(chip->cmd), chip->cmd);
    }
    
    // Execute simple commands immediately
    switch (chip->cmd) {
      case SX126X_CMD_SET_STANDBY:
        chip->state = 0x02; // STANDBY_RC
        printf("  State: %s\n", state_name(chip->state));
        break;
      case SX126X_CMD_SET_TX:
        chip->state = 0x06; // TX
        chip->irq_status |= 0x0001; // TX_DONE (instant for simulation)
        printf("  State: %s, IRQ: TX_DONE\n", state_name(chip->state));
        // Log the payload being transmitted
        if (chip->tx_len > 0) {
          printf("  ========================================\n");
          printf("  📡 TRANSMITTING LoRa PACKET\n");
          printf("  Payload: %d bytes\n", chip->tx_len);
          printf("  Data: ");
          for (int i = 0; i < chip->tx_len && i < 32; i++) {
            printf("%02X ", chip->tx_buffer[i]);
          }
          if (chip->tx_len > 32) printf("...");
          printf("\n");
          printf("  ========================================\n");
        }
        break;
      case SX126X_CMD_SET_RX:
        chip->state = 0x05; // RX
        // Auto-clear TX_DONE when entering RX (simulation convenience)
        if (chip->irq_status & 0x0001) {
          chip->irq_status &= ~0x0001;
          printf("  State: %s (auto-cleared TX_DONE)\n", state_name(chip->state));
        } else {
          printf("  State: %s\n", state_name(chip->state));
        }
        break;
      case SX126X_CMD_CLEAR_IRQ_STATUS:
        // CLEAR_IRQ_STATUS takes 2 parameter bytes (IRQ mask to clear)
        // We'll clear on the first byte received
        if (chip->cmd_pos >= 2) {
          uint16_t clear_mask = (chip->cmd_buffer[1] << 8) | (chip->cmd_pos >= 3 ? chip->cmd_buffer[2] : 0);
          chip->irq_status &= ~clear_mask;
          if (chip->cmd_pos == 3) {
            printf("  IRQ cleared: mask=0x%04X, remaining=0x%04X\n", clear_mask, chip->irq_status);
          }
        }
        break;
    }
    
    next_byte = get_status_byte(chip);
    if (chip->cmd != SX126X_CMD_GET_IRQ_STATUS || chip->irq_status != 0) {
      printf("← Status: 0x%02X (%s)\n", next_byte, state_name(chip->state));
    }
  } else if (chip->cmd == SX126X_CMD_READ_REGISTER) {
    // READ_REGISTER: [CMD][ADDR_H][ADDR_L][STATUS][DATA...]
    if (chip->cmd_pos == 3) {
      uint16_t reg = (chip->cmd_buffer[1] << 8) | chip->cmd_buffer[2];
      printf("  Addr: 0x%04X\n", reg);
    }
    if (chip->cmd_pos == 4) {
      // After status byte, send first data byte
      uint16_t reg = (chip->cmd_buffer[1] << 8) | chip->cmd_buffer[2];
      next_byte = chip->registers[reg & 0xFFF];
      printf("← Data[0]: 0x%02X\n", next_byte);
    } else if (chip->cmd_pos > 4) {
      // Continue sending register data
      uint16_t reg = (chip->cmd_buffer[1] << 8) | chip->cmd_buffer[2];
      next_byte = chip->registers[(reg + chip->cmd_pos - 4) & 0xFFF];
      printf("← Data[%d]: 0x%02X\n", chip->cmd_pos - 4, next_byte);
    } else {
      next_byte = get_status_byte(chip);
    }
  } else if (chip->cmd == SX126X_CMD_WRITE_BUFFER) {
    // WRITE_BUFFER: [CMD][OFFSET][DATA...]
    if (chip->cmd_pos >= 2) {
      uint8_t offset = chip->cmd_buffer[1];
      if (chip->cmd_pos == 2) {
        chip->tx_len = 0; // Reset for new write
        printf("  Offset: 0x%02X\n", offset);
      }
      if (chip->cmd_pos > 2) {
        // Store data bytes
        chip->tx_buffer[offset + chip->cmd_pos - 3] = chip->cmd_buffer[chip->cmd_pos - 1];
        chip->tx_len = offset + chip->cmd_pos - 2;
      }
    }
    next_byte = get_status_byte(chip);
  } else if (chip->cmd == SX126X_CMD_GET_STATUS) {
    next_byte = get_status_byte(chip);
  } else if (chip->cmd == SX126X_CMD_GET_IRQ_STATUS) {
    if (chip->cmd_pos == 2) {
      next_byte = (chip->irq_status >> 8) & 0xFF;
      if (chip->irq_status != 0) printf("← IRQ[MSB]: 0x%02X\n", next_byte);
    } else if (chip->cmd_pos == 3) {
      next_byte = chip->irq_status & 0xFF;
      if (chip->irq_status != 0) printf("← IRQ[LSB]: 0x%02X (IRQ=0x%04X)\n", next_byte, chip->irq_status);
    } else {
      next_byte = get_status_byte(chip);
    }
  } else {
    next_byte = get_status_byte(chip);
  }

  buffer[0] = next_byte;

  if (pin_read(chip->cs) == LOW) {
    spi_start(chip->spi, chip->spi_buffer, 1);
  }
}
