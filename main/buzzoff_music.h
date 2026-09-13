#pragma once

#include <stddef.h>
#include <stdint.h>

/* Self-contained, original 8-bit-style attract melody at 48 kHz mono PCM. */
typedef struct {
    uint32_t phase;
    uint16_t note_sample;
    uint8_t note;
} buzzoff_music_t;

void buzzoff_music_fill(buzzoff_music_t *music, int16_t *samples, size_t count);
