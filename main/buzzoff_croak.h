#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    const int16_t *pcm;
    size_t count;
    size_t position;
    bool active;
} buzzoff_croak_t;

void buzzoff_croak_start(buzzoff_croak_t *croak, const int16_t *pcm, size_t count);
void buzzoff_croak_stop(buzzoff_croak_t *croak);
void buzzoff_croak_mix(buzzoff_croak_t *croak, int16_t *samples, size_t count);
uint8_t buzzoff_croak_volume(bool running, bool croak_chunk);
