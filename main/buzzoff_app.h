#pragma once

#include <stdbool.h>
#include "bsp_button.h"

/* Show the splash as soon as display/LVGL are ready, before slower peripherals.
 * Both entrypoints require the LVGL lock. */
void buzzoff_app_show_boot(void);

/* Start background music immediately after the codec initializes, before
 * battery/UI setup can delay the loading sequence. No LVGL access here. */
void buzzoff_app_start_audio(bool audio_ready);

/* Complete app initialization while the splash continues animating. The app
 * owns its screens, LVGL timer, and audio task for the powered lifetime. */
void buzzoff_app_start(bool buttons_ready);

/* Non-blocking button-task entrypoint; LVGL work runs later in its timer. */
void buzzoff_app_post_key(bsp_btn_t button, bsp_btn_ev_t event);
