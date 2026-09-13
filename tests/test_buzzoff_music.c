#include <assert.h>
#include <stdint.h>
#include "buzzoff_music.h"

static void test_chiptune_stream_and_note_boundary(void)
{
    buzzoff_music_t music = {0};
    int16_t first[256];
    int16_t second[256];
    buzzoff_music_fill(&music, first, 256);
    buzzoff_music_fill(&music, second, 256);
    assert(first[0] == 0);
    assert(first[200] != 0);
    assert(music.note == 0);
    assert(music.note_sample == 512);

    int16_t rest[256];
    for (int i = 512; i < 9600; i += 256) {
        buzzoff_music_fill(&music, rest, 256);
    }
    assert(music.note == 1);
    assert(music.note_sample == 128);
    assert(rest[128] == 0);
}

static void test_music_loops_with_bounded_level(void)
{
    buzzoff_music_t music = {0};
    int16_t samples[256];
    for (int i = 0; i < 16 * 9600; i += 256) {
        buzzoff_music_fill(&music, samples, 256);
        for (int j = 0; j < 256; ++j) {
            assert(samples[j] >= -2048 && samples[j] <= 2048);
        }
    }
    assert(music.note == 0);
    assert(music.note_sample == 0);
}

int main(void)
{
    test_chiptune_stream_and_note_boundary();
    test_music_loops_with_bounded_level();
    return 0;
}
