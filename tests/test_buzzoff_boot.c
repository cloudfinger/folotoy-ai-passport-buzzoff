#include <assert.h>
#include <stdbool.h>
#include "buzzoff_boot.h"

static void test_top_down_reveal_and_signature(void)
{
    buzzoff_boot_t boot;
    buzzoff_boot_init(&boot);
    assert(buzzoff_boot_reveal_y(&boot) == 0);
    assert(buzzoff_boot_name_chars(&boot) == 0);
    assert(!buzzoff_boot_finished(&boot));

    for (int i = 0; i < 15; ++i) buzzoff_boot_advance(&boot, false);
    assert(buzzoff_boot_reveal_y(&boot) == 150);
    assert(buzzoff_boot_name_chars(&boot) == 1);

    for (int i = 0; i < 17; ++i) buzzoff_boot_advance(&boot, false);
    assert(buzzoff_boot_reveal_y(&boot) == 320);
    assert(buzzoff_boot_name_chars(&boot) == 11);
    assert(!buzzoff_boot_finished(&boot));
    assert(!buzzoff_boot_show_ready(&boot, false));
    assert(buzzoff_boot_show_ready(&boot, true));

    for (int i = 0; i < 8; ++i) buzzoff_boot_advance(&boot, false);
    assert(!buzzoff_boot_finished(&boot));
    assert(buzzoff_boot_reveal_y(&boot) == 320);

    for (int i = 0; i < 8; ++i) buzzoff_boot_advance(&boot, true);
    assert(buzzoff_boot_finished(&boot));
    for (int i = 0; i < 100; ++i) buzzoff_boot_advance(&boot, true);
    assert(buzzoff_boot_reveal_y(&boot) == 320);
    assert(buzzoff_boot_name_chars(&boot) == 11);
}

int main(void)
{
    test_top_down_reveal_and_signature();
    return 0;
}
