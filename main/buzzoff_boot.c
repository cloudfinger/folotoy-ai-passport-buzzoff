#include "buzzoff_boot.h"

void buzzoff_boot_init(buzzoff_boot_t *boot)
{
    boot->tick = 0;
}

void buzzoff_boot_advance(buzzoff_boot_t *boot, bool app_ready)
{
    if (boot->tick < 32U || (app_ready && boot->tick < 40U)) ++boot->tick;
}

uint16_t buzzoff_boot_reveal_y(const buzzoff_boot_t *boot)
{
    return boot->tick < 32U ? (uint16_t)boot->tick * 10U : 320U;
}

uint8_t buzzoff_boot_name_chars(const buzzoff_boot_t *boot)
{
    if (boot->tick < 15U) return 0;
    if (boot->tick >= 25U) return 11;
    return (uint8_t)(boot->tick - 14U);
}

bool buzzoff_boot_finished(const buzzoff_boot_t *boot)
{
    return boot->tick >= 40U;
}

bool buzzoff_boot_show_ready(const buzzoff_boot_t *boot, bool app_ready)
{
    return app_ready && buzzoff_boot_reveal_y(boot) == 320U;
}
