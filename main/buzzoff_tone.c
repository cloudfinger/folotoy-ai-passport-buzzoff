#include "buzzoff_tone.h"

/* One quarter of a 64-step sine at a low PCM amplitude. The codec volume is
 * reduced separately in the audio worker. No trigonometry runs per sample. */
static const int16_t sine_quarter[17] = {
    0, 401, 799, 1189, 1567, 1931, 2276, 2598, 2896,
    3166, 3406, 3612, 3784, 3920, 4017, 4076, 4096,
};

static int16_t sine_sample(uint8_t index)
{
    uint8_t offset = index & 15U;
    switch (index >> 4U) {
    case 0: return sine_quarter[offset];
    case 1: return sine_quarter[16U - offset];
    case 2: return (int16_t)-sine_quarter[offset];
    default: return (int16_t)-sine_quarter[16U - offset];
    }
}

void buzzoff_tone_fill(buzzoff_osc_t *osc, uint32_t frequency_hz,
                       int16_t *samples, size_t count)
{
    if (frequency_hz == 0U || frequency_hz > 20000U) {
        for (size_t i = 0; i < count; ++i) samples[i] = 0;
        return;
    }

    uint32_t step = (uint32_t)(((uint64_t)frequency_hz << 32U) / BUZZOFF_SAMPLE_RATE);
    for (size_t i = 0; i < count; ++i) {
        samples[i] = sine_sample((uint8_t)(osc->phase >> 26U));
        osc->phase += step;
    }
}
