#include <assert.h>
#include <stdint.h>
#include "buzzoff_tone.h"

static void test_twelve_khz_tone(void)
{
    buzzoff_osc_t osc = {0};
    int16_t samples[8] = {0};
    buzzoff_tone_fill(&osc, 12000U, samples, 8);
    const int16_t expected[8] = {0, 4096, 0, -4096, 0, 4096, 0, -4096};
    for (int i = 0; i < 8; ++i) assert(samples[i] == expected[i]);
}

static void test_phase_continues_between_chunks(void)
{
    buzzoff_osc_t osc = {0};
    int16_t first[3] = {0};
    int16_t next = 0;
    buzzoff_tone_fill(&osc, 12000U, first, 3);
    buzzoff_tone_fill(&osc, 12000U, &next, 1);
    assert(next == -4096);
}

static void test_quarter_wave_matches_sine(void)
{
    buzzoff_osc_t osc = {0};
    int16_t samples[17] = {0};
    buzzoff_tone_fill(&osc, 750U, samples, 17);
    const int16_t expected[17] = {
        0, 401, 799, 1189, 1567, 1931, 2276, 2598, 2896,
        3166, 3406, 3612, 3784, 3920, 4017, 4076, 4096,
    };
    for (int i = 0; i < 17; ++i) assert(samples[i] == expected[i]);
}

static void test_invalid_frequency_is_silent(void)
{
    buzzoff_osc_t osc = {123U};
    int16_t samples[2] = {7, 7};
    buzzoff_tone_fill(&osc, 0, samples, 2);
    assert(samples[0] == 0 && samples[1] == 0);
    assert(osc.phase == 123U);
    buzzoff_tone_fill(&osc, 24000U, samples, 2);
    assert(samples[0] == 0 && samples[1] == 0);
    assert(osc.phase == 123U);
}

int main(void)
{
    test_twelve_khz_tone();
    test_phase_continues_between_chunks();
    test_quarter_wave_matches_sine();
    test_invalid_frequency_is_silent();
    return 0;
}
