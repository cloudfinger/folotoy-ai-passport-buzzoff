#include <assert.h>
#include <stdint.h>
#include "buzzoff_bullfrog_pcm.h"

int main(void)
{
    assert(buzzoff_bullfrog_pcm_count == 48000U);
    int peak = 0;
    for (size_t i = 0; i < buzzoff_bullfrog_pcm_count; ++i) {
        int value = buzzoff_bullfrog_pcm[i];
        if (value < 0) value = -value;
        if (value > peak) peak = value;
    }
    assert(peak > 15000);
    assert(peak < INT16_MAX);
    assert(buzzoff_bullfrog_pcm[0] == 0);
    assert(buzzoff_bullfrog_pcm[buzzoff_bullfrog_pcm_count - 1] == 0);
    return 0;
}
