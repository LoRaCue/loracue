#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIDTH 250
#define HEIGHT 122
#define BYTES_PER_ROW ((WIDTH + 7) / 8)  // Round up: 32 bytes
#define BUFFER_SIZE (BYTES_PER_ROW * HEIGHT)

// SSD1681 Commands
#define CMD_DRIVER_OUTPUT_CONTROL 0x01
#define CMD_DATA_ENTRY_MODE 0x11
#define CMD_SW_RESET 0x12
#define CMD_TEMP_SENSOR_CONTROL 0x18
#define CMD_MASTER_ACTIVATION 0x20
#define CMD_DISPLAY_UPDATE_CONTROL_1 0x21
#define CMD_DISPLAY_UPDATE_CONTROL_2 0x22
#define CMD_WRITE_RAM_BW 0x24
#define CMD_WRITE_RAM_RED 0x26
#define CMD_SET_RAM_X_ADDRESS_START_END 0x44
#define CMD_SET_RAM_Y_ADDRESS_START_END 0x45
#define CMD_SET_RAM_X_ADDRESS_COUNTER 0x4E
#define CMD_SET_RAM_Y_ADDRESS_COUNTER 0x4F
#define CMD_BORDER_WAVEFORM_CONTROL 0x3C

typedef struct {
    pin_t pin_cs;
    pin_t pin_clk;
    pin_t pin_din;
    pin_t pin_dc;
    pin_t pin_rst;
    pin_t pin_busy;

    uint32_t spi_buffer;
    uint32_t spi_bits;

    uint8_t current_cmd;
    uint8_t data_entry_mode;
    uint8_t ram_x_start;
    uint8_t ram_x_end;
    uint16_t ram_y_start;
    uint16_t ram_y_end;
    uint8_t ram_x_counter;
    uint16_t ram_y_counter;
    uint16_t write_index;

    uint8_t buffer_bw[BUFFER_SIZE];
    uint8_t buffer_red[BUFFER_SIZE];

    buffer_t framebuffer;
    uint32_t fb_width;
    uint32_t fb_height;

    timer_t busy_timer;
} chip_state_t;

static void chip_set_busy(chip_state_t *chip, bool busy)
{
    pin_write(chip->pin_busy, busy ? HIGH : LOW);
}

static void update_framebuffer(chip_state_t *chip)
{
    // Debug: Print first few bytes of buffer RIGHT NOW
    printf("SSD1681: Framebuffer update - buffer first 16 bytes: ");
    for (int i = 0; i < 16 && i < BUFFER_SIZE; i++) {
        printf("%02X ", chip->buffer_bw[i]);
    }
    printf("\n");
    
    // Write framebuffer in chunks to avoid stack overflow
    uint8_t line_data[WIDTH * 4];  // One line at a time
    
    // SSD1681: Row-major layout with stride, MSB first
    // Each row is BYTES_PER_ROW bytes (32 bytes for 250 pixels)
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            // Calculate byte position with stride
            int byte_idx = y * BYTES_PER_ROW + (x / 8);
            int bit_idx = 7 - (x % 8);  // MSB first
            
            uint8_t pixel = (chip->buffer_bw[byte_idx] >> bit_idx) & 1;
            
            int line_idx = x * 4;
            // E-Paper: 1 = black, 0 = white
            uint8_t color = pixel ? 0x00 : 0xFF;
            line_data[line_idx + 0] = color; // R
            line_data[line_idx + 1] = color; // G
            line_data[line_idx + 2] = color; // B
            line_data[line_idx + 3] = 0xFF;  // A
        }
        buffer_write(chip->framebuffer, y * WIDTH * 4, line_data, sizeof(line_data));
    }
}

static void on_busy_timer(void *user_data)
{
    chip_state_t *chip = (chip_state_t *)user_data;
    chip_set_busy(chip, false); // Release busy
    printf("SSD1681: Busy released\n");
}

static void process_command_data(chip_state_t *chip, uint8_t data)
{
    static uint8_t y_counter_byte = 0;
    
    switch (chip->current_cmd) {
        case CMD_DATA_ENTRY_MODE:
            chip->data_entry_mode = data;
            break;
        case CMD_SET_RAM_X_ADDRESS_START_END:
            // Simplified: just storing, real logic handles 2 bytes
            break;
        case CMD_SET_RAM_Y_ADDRESS_START_END:
            // Simplified
            break;
        case CMD_SET_RAM_X_ADDRESS_COUNTER:
            chip->ram_x_counter = data;
            printf("SSD1681: X counter = %d\n", data);
            break;
        case CMD_SET_RAM_Y_ADDRESS_COUNTER:
            // Y counter is 2 bytes, LSB first
            if (y_counter_byte == 0) {
                chip->ram_y_counter = data;
                y_counter_byte = 1;
            } else {
                chip->ram_y_counter |= (data << 8);
                y_counter_byte = 0;
                // Calculate write position from X/Y
                chip->write_index = chip->ram_y_counter * BYTES_PER_ROW + chip->ram_x_counter;
                printf("SSD1681: Y counter = %d, write_index = %d\n", chip->ram_y_counter, chip->write_index);
            }
            break;
        case CMD_WRITE_RAM_BW:
            // Write to buffer at current position
            if (chip->write_index < BUFFER_SIZE) {
                chip->buffer_bw[chip->write_index] = data;
                if (chip->write_index < 5) {
                    printf("SSD1681: buf[%d] = 0x%02X\n", chip->write_index, data);
                }
                chip->write_index++;
            } else {
                printf("SSD1681: Buffer overflow at index %d\n", chip->write_index);
            }
            break;
        case CMD_WRITE_RAM_RED:
            break;
    }
}

static void on_spi_byte(chip_state_t *chip, uint8_t byte)
{
    if (pin_read(chip->pin_dc) == LOW) {
        // Command
        if (chip->current_cmd == CMD_WRITE_RAM_BW) {
            printf("SSD1681: CMD 0x24 ended, wrote %d bytes - updating display\n", chip->write_index);
            update_framebuffer(chip);
        }
        chip->current_cmd = byte;
        printf("SSD1681: CMD 0x%02X\n", byte);

        switch (byte) {
            case CMD_SW_RESET:
                printf("SSD1681: SW_RESET - clearing buffer\n");
                memset(chip->buffer_bw, 0xFF, BUFFER_SIZE);
                chip->write_index = 0;
                chip_set_busy(chip, true);
                timer_start(chip->busy_timer, 1000, false); // 1ms busy (fast for simulation)
                break;
            case CMD_MASTER_ACTIVATION:
                printf("SSD1681: MASTER_ACTIVATION - refreshing now\n");
                update_framebuffer(chip);
                chip_set_busy(chip, true);
                timer_start(chip->busy_timer, 10000, false); // 10ms busy (fast for simulation)
                break;
        }
    } else {
        // Data
        process_command_data(chip, byte);
    }
}

static void on_clk_change(void *user_data, pin_t pin, uint32_t value)
{
    chip_state_t *chip = (chip_state_t *)user_data;
    if (pin_read(chip->pin_cs) == HIGH) {
        return; // Chip not selected
    }

    if (value == HIGH) { // Rising edge
        uint8_t bit      = pin_read(chip->pin_din);
        chip->spi_buffer = (chip->spi_buffer << 1) | bit;
        chip->spi_bits++;
        if (chip->spi_bits == 8) {
            on_spi_byte(chip, (uint8_t)chip->spi_buffer);
            chip->spi_bits   = 0;
            chip->spi_buffer = 0;
        }
    }
}

static void on_cs_change(void *user_data, pin_t pin, uint32_t value)
{
    chip_state_t *chip = (chip_state_t *)user_data;
    if (value == LOW) {
        chip->spi_bits   = 0;
        chip->spi_buffer = 0;
    }
}

static void on_rst_change(void *user_data, pin_t pin, uint32_t value)
{
    chip_state_t *chip = (chip_state_t *)user_data;
    if (value == LOW) {
        // Reset active
        chip_set_busy(chip, false); // Usually busy is low during reset? Or high?
                                    // Datasheet: Busy pin is high during reset? No, usually low.
                                    // Let's assume reset clears busy.
    } else {
        // Reset released
        chip_set_busy(chip, true);
        timer_start(chip->busy_timer, 10000, false); // 10ms busy after reset
    }
}

void chip_init()
{
    chip_state_t *chip = malloc(sizeof(chip_state_t));

    chip->pin_cs   = pin_init("CS", INPUT_PULLUP);
    chip->pin_clk  = pin_init("CLK", INPUT);
    chip->pin_din  = pin_init("DIN", INPUT);
    chip->pin_dc   = pin_init("DC", INPUT);
    chip->pin_rst  = pin_init("RST", INPUT);
    chip->pin_busy = pin_init("BUSY", OUTPUT_LOW);  // Start not busy

    // Initialize framebuffer
    chip->fb_width = WIDTH;
    chip->fb_height = HEIGHT;
    chip->framebuffer = framebuffer_init(&chip->fb_width, &chip->fb_height);
    
    // Clear buffers
    memset(chip->buffer_bw, 0xFF, BUFFER_SIZE);  // White
    memset(chip->buffer_red, 0x00, BUFFER_SIZE);
    
    // Initial framebuffer update (white screen)
    update_framebuffer(chip);

    chip->busy_timer = timer_init(&(timer_config_t){
        .callback  = on_busy_timer,
        .user_data = chip,
    });

    const pin_watch_config_t watch_clk = {
        .edge       = RISING,
        .pin_change = on_clk_change,
        .user_data  = chip,
    };
    pin_watch(chip->pin_clk, &watch_clk);

    const pin_watch_config_t watch_cs = {
        .edge       = BOTH,
        .pin_change = on_cs_change,
        .user_data  = chip,
    };
    pin_watch(chip->pin_cs, &watch_cs);

    const pin_watch_config_t watch_rst = {
        .edge       = BOTH,
        .pin_change = on_rst_change,
        .user_data  = chip,
    };
    pin_watch(chip->pin_rst, &watch_rst);

    chip_set_busy(chip, false); // Not busy initially
    printf("SSD1681: Chip initialized\n");
}
