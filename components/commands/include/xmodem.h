#pragma once

#include "esp_err.h"
#include <stddef.h>

#define XMODEM_SOH 0x01
#define XMODEM_STX 0x02
#define XMODEM_EOT 0x04
#define XMODEM_ACK 0x06
#define XMODEM_NAK 0x15
#define XMODEM_CAN 0x18
#define XMODEM_CRC 0x43

#define XMODEM_BLOCK_SIZE 128
#define XMODEM_1K_BLOCK_SIZE 1024

esp_err_t xmodem_receive(size_t expected_size);
