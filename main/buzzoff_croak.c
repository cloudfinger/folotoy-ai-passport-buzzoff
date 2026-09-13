#include "buzzoff_croak.h"

/* The recording is held in Flash by its caller. Only a cursor lives in RAM;
 * the ongoing tone is ducked so the real bullfrog remains audible. */
void buzzoff_croak_start(buzzoff_croak_t *croak, const int16_t *pcm, size_t count)
{
    *croak = (buzzoff_croak_t){
        .pcm = pcm,
        .count = count,
        .position = 0,
        .active = pcm != NULL && count != 0U,
    };
}

void buzzoff_croak_stop(buzzoff_croak_t *croak)
{
    *croak = (buzzoff_croak_t){0};
}

void buzzoff_croak_mix(buzzoff_croak_t *croak, int16_t *samples, size_t count)
{
    for (size_t i = 0; i < count && croak->active; ++i) {
        int32_t mixed = (int32_t)samples[i] / 4 + croak->pcm[croak->position];
        if (mixed > INT16_MAX) mixed = INT16_MAX;
        if (mixed < INT16_MIN) mixed = INT16_MIN;
        samples[i] = (int16_t)mixed;
        if (++croak->position == croak->count) croak->active = false;
    }
}

uint8_t buzzoff_croak_volume(bool running, bool croak_chunk)
{
    if (!running) return 26;
    return croak_chunk ? 55 : 35;
}
