#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>

// ED047TC1 4.7" E-Paper Display (540x960, 16 grayscale)
#define WIDTH  540
#define HEIGHT 960

typedef struct {
    pin_t d0, d1, d2, d3, d4, d5, d6, d7;
    pin_t ckv, sth, leh, stv, ckh;
    buffer_t framebuffer;
    uint32_t width;
    uint32_t height;
    uint32_t x, y;
} chip_state_t;

static void chip_pin_change(void *user_data, pin_t pin, uint32_t value);

void chip_init(void) {
    chip_state_t *chip = malloc(sizeof(chip_state_t));
    if (!chip) return;
    
    chip->width = WIDTH;
    chip->height = HEIGHT;
    chip->framebuffer = framebuffer_init(&chip->width, &chip->height);
    chip->x = 0;
    chip->y = 0;
    
    const pin_watch_config_t watch_config = {
        .edge = RISING,
        .pin_change = chip_pin_change,
        .user_data = chip,
    };
    
    chip->d0 = pin_init("D0", INPUT);
    chip->d1 = pin_init("D1", INPUT);
    chip->d2 = pin_init("D2", INPUT);
    chip->d3 = pin_init("D3", INPUT);
    chip->d4 = pin_init("D4", INPUT);
    chip->d5 = pin_init("D5", INPUT);
    chip->d6 = pin_init("D6", INPUT);
    chip->d7 = pin_init("D7", INPUT);
    
    chip->ckv = pin_init("CKV", INPUT);
    pin_watch(chip->ckv, &watch_config);
    
    chip->sth = pin_init("STH", INPUT);
    chip->leh = pin_init("LEH", INPUT);
    
    chip->stv = pin_init("STV", INPUT);
    pin_watch(chip->stv, &watch_config);
    
    chip->ckh = pin_init("CKH", INPUT);
    pin_watch(chip->ckh, &watch_config);
    
    printf("ED047TC1: E-Paper display initialized (%dx%d)\n", WIDTH, HEIGHT);
}

static void chip_pin_change(void *user_data, pin_t pin, uint32_t value) {
    chip_state_t *chip = (chip_state_t*)user_data;
    
    if (pin == chip->ckh) {
        uint8_t data = (pin_read(chip->d0) << 0) |
                      (pin_read(chip->d1) << 1) |
                      (pin_read(chip->d2) << 2) |
                      (pin_read(chip->d3) << 3) |
                      (pin_read(chip->d4) << 4) |
                      (pin_read(chip->d5) << 5) |
                      (pin_read(chip->d6) << 6) |
                      (pin_read(chip->d7) << 7);
        
        uint8_t gray = (data & 0x0F) * 17;
        uint32_t color = 0xFF000000 | (gray << 16) | (gray << 8) | gray;
        
        if (chip->x < chip->width && chip->y < chip->height) {
            uint32_t pix_index = chip->y * chip->width + chip->x;
            buffer_write(chip->framebuffer, pix_index * sizeof(color), (uint8_t*)&color, sizeof(color));
            chip->x++;
        }
    } else if (pin == chip->ckv) {
        chip->x = 0;
        chip->y++;
        if (chip->y >= chip->height) {
            chip->y = 0;
        }
    } else if (pin == chip->stv) {
        chip->y = 0;
        chip->x = 0;
    }
}
