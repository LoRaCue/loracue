/**
 * @file bsp_heltec_v3.c
 * @brief Board Support Package for Heltec LoRa V3 (ESP32-S3 + SX1262)
 * 
 * CONTEXT: LoRaCue enterprise presentation clicker
 * HARDWARE: Heltec LoRa V3 development board
 * PINS: SPI(8-14)=LoRa, I2C(17-18)=OLED, GPIO(45-46)=Buttons, ADC(1,37)=Battery
 * PROTOCOL: SF7/BW500kHz LoRa with AES-128 encryption
 * ARCHITECTURE: BSP abstraction layer for multi-board support
 * 
 * This file implements hardware abstraction for:
 * - GPIO initialization (buttons with pullups)
 * - SPI bus for SX1262 LoRa transceiver  
 * - I2C bus for SH1106 OLED display
 * - ADC for battery voltage monitoring
 * - Power management and sleep modes
 */
