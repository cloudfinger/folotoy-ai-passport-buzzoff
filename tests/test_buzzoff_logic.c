#include <assert.h>
#include "buzzoff_logic.h"

static void test_controls(void)
{
    buzzoff_state_t state;
    buzzoff_state_init(&state);
    assert(state.eaten == 0);
    assert(!state.running);
    assert(buzzoff_frequency_hz(&state) == 16000U);

    buzzoff_state_key(&state, BUZZOFF_KEY_UP);
    assert(buzzoff_frequency_hz(&state) == 12000U);
    buzzoff_state_key(&state, BUZZOFF_KEY_UP);
    assert(buzzoff_frequency_hz(&state) == 12000U);
    buzzoff_state_key(&state, BUZZOFF_KEY_DOWN);
    buzzoff_state_key(&state, BUZZOFF_KEY_DOWN);
    buzzoff_state_key(&state, BUZZOFF_KEY_DOWN);
    assert(buzzoff_frequency_hz(&state) == 20000U);

    buzzoff_state_key(&state, BUZZOFF_KEY_OK);
    assert(state.running);
    buzzoff_state_key(&state, BUZZOFF_KEY_OK);
    assert(!state.running);
}

static void test_frog_animation(void)
{
    buzzoff_state_t state;
    buzzoff_state_init(&state);
    buzzoff_frame_t frame = buzzoff_frame(&state);
    assert(!frame.mosquito_visible);
    assert(frame.tongue_len == 0);
    assert(!frame.chewing);

    buzzoff_state_key(&state, BUZZOFF_KEY_OK);
    assert(state.tick == 0);
    frame = buzzoff_frame(&state);
    assert(frame.mosquito_visible);
    assert(frame.mosquito_x == 206);
    assert(frame.mosquito_y == 37);
    for (int i = 0; i < 12; ++i) buzzoff_state_advance(&state);
    frame = buzzoff_frame(&state);
    assert(frame.mosquito_visible);
    assert(frame.mosquito_x == 174);
    assert(frame.mosquito_y == 42);
    assert(frame.tongue_len > 0);

    for (int i = 0; i < 4; ++i) buzzoff_state_advance(&state);
    frame = buzzoff_frame(&state);
    assert(!frame.mosquito_visible);
    assert(frame.chewing);
    assert(state.eaten == 1);

    for (int i = 0; i < 8; ++i) buzzoff_state_advance(&state);
    assert(state.tick == 0);
    assert(buzzoff_frame(&state).mosquito_visible);
    assert(state.eaten == 1);
    for (int i = 0; i < 16; ++i) buzzoff_state_advance(&state);
    assert(state.eaten == 2);
}

static void test_idle_and_stop_hide_hunt_and_freeze_score(void)
{
    buzzoff_state_t state;
    buzzoff_state_init(&state);
    for (int i = 0; i < 48; ++i) buzzoff_state_advance(&state);
    assert(!state.running);
    assert(state.tick == 0);
    assert(state.eaten == 0);
    assert(!buzzoff_frame(&state).mosquito_visible);
    assert(buzzoff_frame(&state).tongue_len == 0);

    buzzoff_state_key(&state, BUZZOFF_KEY_OK);
    for (int i = 0; i < 16; ++i) buzzoff_state_advance(&state);
    assert(state.eaten == 1);
    for (int i = 0; i < 21; ++i) buzzoff_state_advance(&state);
    assert(buzzoff_frame(&state).tongue_len > 0);

    buzzoff_state_key(&state, BUZZOFF_KEY_OK);
    assert(!state.running);
    assert(state.tick == 0);
    buzzoff_frame_t frame = buzzoff_frame(&state);
    assert(!frame.mosquito_visible);
    assert(frame.tongue_len == 0);
    assert(!frame.chewing);
    for (int i = 0; i < 48; ++i) buzzoff_state_advance(&state);
    assert(state.tick == 0);
    assert(state.eaten == 1);

    buzzoff_state_key(&state, BUZZOFF_KEY_OK);
    assert(state.running);
    assert(state.tick == 0);
    frame = buzzoff_frame(&state);
    assert(frame.mosquito_visible);
    assert(frame.tongue_len == 0);
    assert(state.eaten == 1);
}

static void test_render_step_shows_start_frame_before_advancing(void)
{
    buzzoff_state_t state;
    buzzoff_state_init(&state);

    bool was_running = state.running;
    buzzoff_state_key(&state, BUZZOFF_KEY_OK);
    buzzoff_state_advance_render_frame(&state, was_running);
    assert(state.tick == 0);
    assert(buzzoff_frame(&state).mosquito_x == 206);

    was_running = state.running;
    buzzoff_state_advance_render_frame(&state, was_running);
    assert(state.tick == 1);
    assert(buzzoff_frame(&state).mosquito_x == 203);

    was_running = state.running;
    buzzoff_state_key(&state, BUZZOFF_KEY_OK);
    buzzoff_state_advance_render_frame(&state, was_running);
    assert(state.tick == 0);
    assert(!buzzoff_frame(&state).mosquito_visible);
}

static void test_batched_button_input(void)
{
    buzzoff_state_t state;
    buzzoff_state_init(&state);
    const buzzoff_key_t down[] = {BUZZOFF_KEY_DOWN, BUZZOFF_KEY_DOWN,
                                  BUZZOFF_KEY_DOWN, BUZZOFF_KEY_DOWN};
    buzzoff_state_apply_keys(&state, down, 4, 2);
    assert(buzzoff_frequency_hz(&state) == 20000U);
    assert(!state.running);
    const buzzoff_key_t up[] = {BUZZOFF_KEY_UP, BUZZOFF_KEY_UP,
                                BUZZOFF_KEY_UP, BUZZOFF_KEY_UP};
    buzzoff_state_apply_keys(&state, up, 4, 3);
    assert(buzzoff_frequency_hz(&state) == 12000U);
    assert(state.running);
}

static void test_order_is_preserved_at_boundaries(void)
{
    buzzoff_state_t state;
    buzzoff_state_init(&state);
    const buzzoff_key_t initial[] = {BUZZOFF_KEY_UP};
    buzzoff_state_apply_keys(&state, initial, 1, 0);
    assert(buzzoff_frequency_hz(&state) == 12000U);

    const buzzoff_key_t up_then_down[] = {BUZZOFF_KEY_UP, BUZZOFF_KEY_DOWN};
    buzzoff_state_apply_keys(&state, up_then_down, 2, 0);
    assert(buzzoff_frequency_hz(&state) == 16000U);

    const buzzoff_key_t to_max[] = {BUZZOFF_KEY_DOWN};
    buzzoff_state_apply_keys(&state, to_max, 1, 0);
    const buzzoff_key_t down_then_up[] = {BUZZOFF_KEY_DOWN, BUZZOFF_KEY_UP};
    buzzoff_state_apply_keys(&state, down_then_up, 2, 0);
    assert(buzzoff_frequency_hz(&state) == 16000U);
}

static void advance_to_next_catch(buzzoff_state_t *state)
{
    do {
        buzzoff_state_advance(state);
    } while (state->tick != 16U);
}

static void test_croak_every_second_running_catch(void)
{
    buzzoff_state_t state;
    buzzoff_state_init(&state);
    buzzoff_state_advance(&state);
    assert(state.tick == 0);
    assert(!buzzoff_croak_due(&state));

    buzzoff_state_key(&state, BUZZOFF_KEY_OK);
    advance_to_next_catch(&state);
    assert(!buzzoff_croak_due(&state));
    advance_to_next_catch(&state);
    assert(buzzoff_croak_due(&state));
    buzzoff_state_advance(&state);
    assert(!buzzoff_croak_due(&state));

    buzzoff_state_key(&state, BUZZOFF_KEY_OK);
    assert(state.tick == 0);
    buzzoff_state_advance(&state);
    assert(state.tick == 0);
    assert(!buzzoff_croak_due(&state));
    buzzoff_state_key(&state, BUZZOFF_KEY_OK);
    advance_to_next_catch(&state);
    assert(!buzzoff_croak_due(&state));
    advance_to_next_catch(&state);
    assert(buzzoff_croak_due(&state));
}

int main(void)
{
    test_controls();
    test_frog_animation();
    test_idle_and_stop_hide_hunt_and_freeze_score();
    test_render_step_shows_start_frame_before_advancing();
    test_batched_button_input();
    test_order_is_preserved_at_boundaries();
    test_croak_every_second_running_catch();
    return 0;
}
