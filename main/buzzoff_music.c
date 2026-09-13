#include "buzzoff_music.h"
#include "buzzoff_tone.h"

#define NOTE_SAMPLES 9600U
#define NOTE_COUNT 16U

/* C/E/G/A figures form a short original two-bar loop; zero is a rest. */
static const uint16_t melody_hz[NOTE_COUNT] = {
    523, 659, 784, 659, 880, 784, 659, 0,
    587, 698, 880, 698, 784, 659, 523, 0,
};

static uint32_t phase_step(uint16_t frequency_hz)
{
    return (uint32_t)(((uint64_t)frequency_hz << 32U) / BUZZOFF_SAMPLE_RATE);
}

void buzzoff_music_fill(buzzoff_music_t *music, int16_t *samples, size_t count)
{
    uint32_t step = phase_step(melody_hz[music->note]);
    for (size_t i = 0; i < count; ++i) {
        if (music->note_sample == NOTE_SAMPLES) {
            music->note_sample = 0;
            music->note = (uint8_t)((music->note + 1U) % NOTE_COUNT);
            music->phase = 0;
            step = phase_step(melody_hz[music->note]);
        }
        uint16_t edge = music->note_sample;
        uint16_t remaining = (uint16_t)(NOTE_SAMPLES - music->note_sample);
        if (remaining < edge) edge = remaining;
        int16_t level = edge < 128U ? (int16_t)(16U * edge) : 2048;
        samples[i] = step == 0U ? 0 :
            (music->phase & 0x80000000U) ? level : (int16_t)-level;
        music->phase += step;
        ++music->note_sample;
    }
    if (music->note_sample == NOTE_SAMPLES) {
        music->note_sample = 0;
        music->note = (uint8_t)((music->note + 1U) % NOTE_COUNT);
        music->phase = 0;
    }
}
