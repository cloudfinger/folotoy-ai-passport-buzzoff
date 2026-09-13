#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "buzzoff_croak.h"

static void test_idle_leaves_tone_unchanged(void)
{
    buzzoff_croak_t croak = {0};
    int16_t samples[] = {4000, -4000, 0};
    buzzoff_croak_mix(&croak, samples, 3);
    assert(samples[0] == 4000);
    assert(samples[1] == -4000);
    assert(samples[2] == 0);
}

static void test_recorded_pcm_replaces_synth_and_ducks_tone(void)
{
    const int16_t recording[] = {0, 1000, -1000, 500};
    buzzoff_croak_t croak = {0};
    int16_t samples[] = {4000, 4000, 4000, 4000, 4000};
    buzzoff_croak_start(&croak, recording, 4);
    buzzoff_croak_mix(&croak, samples, 5);
    const int16_t expected[] = {1000, 2000, 0, 1500, 4000};
    assert(memcmp(samples, expected, sizeof(expected)) == 0);
}

static void test_chunks_match_one_shot_and_saturate(void)
{
    const int16_t recording[] = {32000, -32000, 100, -100, 300, -300};
    buzzoff_croak_t whole = {0};
    buzzoff_croak_t chunks = {0};
    int16_t a[] = {16000, -16000, 4000, 4000, 4000, 4000};
    int16_t b[] = {16000, -16000, 4000, 4000, 4000, 4000};
    buzzoff_croak_start(&whole, recording, 6);
    buzzoff_croak_start(&chunks, recording, 6);
    buzzoff_croak_mix(&whole, a, 6);
    buzzoff_croak_mix(&chunks, b, 2);
    buzzoff_croak_mix(&chunks, b + 2, 4);
    assert(memcmp(a, b, sizeof(a)) == 0);
    assert(a[0] == INT16_MAX);
    assert(a[1] == INT16_MIN);
}

static void test_stop_discards_recording_and_restart_begins_at_start(void)
{
    const int16_t recording[] = {300, 600, 900};
    buzzoff_croak_t croak = {0};
    int16_t samples[] = {4000, 4000};
    buzzoff_croak_start(&croak, recording, 3);
    buzzoff_croak_mix(&croak, samples, 2);
    buzzoff_croak_stop(&croak);
    samples[0] = 4000;
    samples[1] = 4000;
    buzzoff_croak_mix(&croak, samples, 2);
    assert(samples[0] == 4000 && samples[1] == 4000);
    buzzoff_croak_start(&croak, recording, 3);
    buzzoff_croak_mix(&croak, samples, 2);
    assert(samples[0] == 1300 && samples[1] == 1600);
}

static void test_empty_recording_does_not_duck_tone(void)
{
    buzzoff_croak_t croak = {0};
    int16_t samples[] = {4000};
    buzzoff_croak_start(&croak, NULL, 0);
    buzzoff_croak_mix(&croak, samples, 1);
    assert(samples[0] == 4000);
}

static void test_codec_volume_tracks_recording_chunks(void)
{
    const int16_t recording[] = {100, 200};
    buzzoff_croak_t croak = {0};
    int16_t sample = 4000;
    assert(buzzoff_croak_volume(false, false) == 26);
    assert(buzzoff_croak_volume(true, false) == 35);

    buzzoff_croak_start(&croak, recording, 2);
    bool playing_this_chunk = croak.active;
    buzzoff_croak_mix(&croak, &sample, 1);
    assert(buzzoff_croak_volume(true, playing_this_chunk) == 55);

    playing_this_chunk = croak.active;
    buzzoff_croak_mix(&croak, &sample, 1);
    assert(buzzoff_croak_volume(true, playing_this_chunk) == 55);
    assert(!croak.active);
    assert(buzzoff_croak_volume(true, croak.active) == 35);
    assert(buzzoff_croak_volume(false, true) == 26);
}

int main(void)
{
    test_idle_leaves_tone_unchanged();
    test_recorded_pcm_replaces_synth_and_ducks_tone();
    test_chunks_match_one_shot_and_saturate();
    test_stop_discards_recording_and_restart_begins_at_start();
    test_empty_recording_does_not_duck_tone();
    test_codec_volume_tracks_recording_chunks();
    return 0;
}
