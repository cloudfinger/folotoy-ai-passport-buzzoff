#include "buzzoff_logic.h"

static const uint32_t frequencies_hz[BUZZOFF_PRESET_COUNT] = {
    12000U, 16000U, 20000U,
};

void buzzoff_state_init(buzzoff_state_t *state)
{
    *state = (buzzoff_state_t){.running = false, .preset = 1, .tick = 0};
}

void buzzoff_state_key(buzzoff_state_t *state, buzzoff_key_t key)
{
    if (key == BUZZOFF_KEY_UP && state->preset > 0) {
        --state->preset;
    } else if (key == BUZZOFF_KEY_DOWN && state->preset + 1 < BUZZOFF_PRESET_COUNT) {
        ++state->preset;
    } else if (key == BUZZOFF_KEY_OK) {
        state->running = !state->running;
        state->tick = 0;
        if (state->running) state->running_eaten = 0;
    }
}

void buzzoff_state_apply_keys(buzzoff_state_t *state, const buzzoff_key_t *keys,
                              size_t count, uint32_t ok_clicks)
{
    for (size_t i = 0; i < count; ++i) buzzoff_state_key(state, keys[i]);
    if (ok_clicks & 1U) buzzoff_state_key(state, BUZZOFF_KEY_OK);
}

void buzzoff_state_advance(buzzoff_state_t *state)
{
    if (!state->running) return;
    state->tick = (uint8_t)((state->tick + 1U) % 24U);
    if (state->tick == 16U) {
        ++state->eaten;
        ++state->running_eaten;
    }
}

bool buzzoff_croak_due(const buzzoff_state_t *state)
{
    return state->running && state->tick == 16U && state->running_eaten != 0U &&
           (state->running_eaten & 1U) == 0U;
}

uint32_t buzzoff_frequency_hz(const buzzoff_state_t *state)
{
    return frequencies_hz[state->preset < BUZZOFF_PRESET_COUNT ? state->preset : 1];
}

buzzoff_frame_t buzzoff_frame(const buzzoff_state_t *state)
{
    buzzoff_frame_t frame = {
        .mosquito_visible = state->running,
        .mosquito_x = 206,
        .mosquito_y = 37,
        .tongue_len = 0,
        .chewing = false,
    };
    if (!state->running) return frame;
    uint8_t tick = state->tick;
    if (tick < 12) {
        frame.mosquito_x = (uint8_t)(206 - 3 * tick);
        frame.mosquito_y = (uint8_t)(37 + 2 * (tick % 3));
    } else if (tick < 16) {
        frame.mosquito_x = 174;
        frame.mosquito_y = 42;
        frame.tongue_len = (uint8_t)(32 + 8 * (tick - 12));
    } else {
        frame.mosquito_visible = false;
        frame.chewing = tick < 21;
        frame.tongue_len = tick == 16 ? 24 : 0;
    }
    return frame;
}
