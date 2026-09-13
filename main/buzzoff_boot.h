#pragma once

#include <stdbool.h>
#include <stdint.h>

/* One advance per 100 ms: a 3.2 s top-down reveal, then an 0.8 s hold. */
typedef struct {
    uint8_t tick;
} buzzoff_boot_t;

void buzzoff_boot_init(buzzoff_boot_t *boot);
/* The reveal may complete before peripherals; hold the final loading frame
 * until audio and app state are ready, then show it for another 0.8 s. */
void buzzoff_boot_advance(buzzoff_boot_t *boot, bool app_ready);
uint16_t buzzoff_boot_reveal_y(const buzzoff_boot_t *boot);
uint8_t buzzoff_boot_name_chars(const buzzoff_boot_t *boot);
bool buzzoff_boot_finished(const buzzoff_boot_t *boot);
bool buzzoff_boot_show_ready(const buzzoff_boot_t *boot, bool app_ready);
