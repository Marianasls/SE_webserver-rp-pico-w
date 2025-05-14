#ifndef LED_MATRIX_H
#define LED_MATRIX_H

#include "hardware/pio.h"

#define NUM_PIXELS 25

static inline void put_pixel(uint32_t pixel_grb);

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b);

void set_leds(uint8_t r, uint8_t g, uint8_t b);

void clear_buffer();

void turn_on_leds();

void set_led_intensity(int intensity);

#endif
