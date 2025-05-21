#include "led_matrix.h"
#include <math.h>
#include "pico/stdlib.h"
// Buffer para armazenar quais LEDs estão ligados matriz 5x5
bool led_buffer[NUM_PIXELS] = {
    0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0
};

float intensity = 1; // default de 10%

// Matriz de LEDs
static inline void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

void set_leds(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t color = urgb_u32(r*intensity, g*intensity, b*intensity);
    for (int i = 0; i < NUM_PIXELS; i++) {
        if (led_buffer[i]) {
            put_pixel(color);
        }
        else {
            put_pixel(0);
        }
    }
}

void set_led_intensity(int value) {
    intensity = value / 100.0f;
}

void turn_on_leds() {
    for(int i = 0; i < 25; i++) {
        led_buffer[i] = 1;
    }
}
void clear_buffer() {
    for(int i = 0; i < NUM_PIXELS; i++) {
        led_buffer[i] = 0;
    }
}

void hsv_to_rgb(float h, float s, float v, uint8_t *r, uint8_t *g, uint8_t *b) {
    float c = v * s;
    float x = c * (1 - fabsf(fmodf(h / 60.0f, 2) - 1));
    float m = v - c;

    float r_, g_, b_;

    if (h < 60) {
        r_ = c; g_ = x; b_ = 0;
    } else if (h < 120) {
        r_ = x; g_ = c; b_ = 0;
    } else if (h < 180) {
        r_ = 0; g_ = c; b_ = x;
    } else if (h < 240) {
        r_ = 0; g_ = x; b_ = c;
    } else if (h < 300) {
        r_ = x; g_ = 0; b_ = c;
    } else {
        r_ = c; g_ = 0; b_ = x;
    }

    *r = (uint8_t)((r_ + m) * 255 * intensity);
    *g = (uint8_t)((g_ + m) * 255 * intensity);
    *b = (uint8_t)((b_ + m) * 255 * intensity);
}

void rainbow_cycle(int delay_ms, int *mode) {
    const float sat = 1.0f;
    const float val = 1.0f;
    float hue = 0;
    turn_on_leds();  // Liga todos os LEDs

    while (*mode) {
        for (int i = 0; i < NUM_PIXELS; i++) {
            float led_hue = fmodf(hue + (360.0f / NUM_PIXELS) * i, 360.0f);
            uint8_t r, g, b;
            hsv_to_rgb(led_hue, sat, val, &r, &g, &b);
            if (led_buffer[i]) {
                put_pixel(urgb_u32(r, g, b));
            } else {
                put_pixel(0);
            }
        }

        hue += 3; // velocidade do arco-íris
        if (hue >= 360.0f) hue -= 360.0f;

        sleep_ms(delay_ms);
    }
}
