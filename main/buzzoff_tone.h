#pragma once

#include <stddef.h>
#include <stdint.h>

#define BUZZOFF_SAMPLE_RATE 48000U

typedef struct {
    uint32_t phase;
} buzzoff_osc_t;

void buzzoff_tone_fill(buzzoff_osc_t *osc, uint32_t frequency_hz,
                       int16_t *samples, size_t count);
