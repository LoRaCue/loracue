#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>

// ED047TC1 4.7" E-Paper Display (960x540, 16 grayscale)
// Parallel interface with control signals

#define WIDTH  960
#define HEIGHT 540
#define GRAYSCALE 16

typedef struct {
    pin_t d0, d1, d2, d3, d4, d5, d6, d7;  // Data pins
    pin_t ckv;   // Vertical clock
    pin_t sth;   // Start horizontal
    pin_t leh;   // Latch enable horizontal
    pin_t stv;   // Start vertical
    pin_t ckh;   // Horizontal clock
    pin_t oe;    // Output enable
    pin_t mode;  // Mode select
    uint32_t pixel_count;
    bool initialized;
} chip_state_t;

void chip_init(void) {
    chip_state_t *chip = malloc(sizeof(chip_state_t));
    if (!chip) return;
    
    // Initialize pins
    chip->d0 = pin_init("D0", INPUT);
    chip->d1 = pin_init("D1", INPUT);
    chip->d2 = pin_init("D2", INPUT);
    chip->d3 = pin_init("D3", INPUT);
    chip->d4 = pin_init("D4", INPUT);
    chip->d5 = pin_init("D5", INPUT);
    chip->d6 = pin_init("D6", INPUT);
    chip->d7 = pin_init("D7", INPUT);
    
    chip->ckv = pin_init("CKV", INPUT);
    chip->sth = pin_init("STH", INPUT);
    chip->leh = pin_init("LEH", INPUT);
    chip->stv = pin_init("STV", INPUT);
    chip->ckh = pin_init("CKH", INPUT);
    chip->oe = pin_init("OE", INPUT);
    chip->mode = pin_init("MODE", INPUT);
    
    chip->pixel_count = 0;
    chip->initialized = true;
    
    printf("ED047TC1: E-Paper display initialized (%dx%d, %d grayscale)\n", 
           WIDTH, HEIGHT, GRAYSCALE);
}
