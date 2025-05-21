#ifndef LED_MATRIX_H
#define LED_MATRIX_H

#include "hardware/pio.h"

#define NUM_PIXELS 25
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    char *name;
} color_t;

static inline void put_pixel(uint32_t pixel_grb);

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b);

void set_leds(uint8_t r, uint8_t g, uint8_t b);

void clear_buffer();

void turn_on_leds();

void set_led_intensity(int value);

void rainbow_cycle(int delay_ms, int *mode);

void hsv_to_rgb(float h, float s, float v, uint8_t *r, uint8_t *g, uint8_t *b);

#endif
