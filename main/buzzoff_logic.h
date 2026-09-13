#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BUZZOFF_PRESET_COUNT 3

typedef enum {
    BUZZOFF_KEY_UP,
    BUZZOFF_KEY_DOWN,
    BUZZOFF_KEY_OK,
} buzzoff_key_t;

typedef struct {
    bool running;
    uint8_t preset;
    uint8_t tick;
    uint32_t eaten;
    uint32_t running_eaten;
} buzzoff_state_t;

typedef struct {
    bool mosquito_visible;
    uint8_t mosquito_x;
    uint8_t mosquito_y;
    uint8_t tongue_len;
    bool chewing;
} buzzoff_frame_t;

void buzzoff_state_init(buzzoff_state_t *state);
void buzzoff_state_key(buzzoff_state_t *state, buzzoff_key_t key);
void buzzoff_state_apply_keys(buzzoff_state_t *state, const buzzoff_key_t *keys,
                              size_t count, uint32_t ok_clicks);
void buzzoff_state_advance(buzzoff_state_t *state);
bool buzzoff_croak_due(const buzzoff_state_t *state);
uint32_t buzzoff_frequency_hz(const buzzoff_state_t *state);
buzzoff_frame_t buzzoff_frame(const buzzoff_state_t *state);
