/* Buzz Off is a dedicated AI Passport app: boot directly into the pixel frog.
 * All device access stays on the existing BSP; no pin or Flash layout changes. */
#include "bsp_audio.h"
#include "bsp_battery.h"
#include "bsp_button.h"
#include "bsp_display.h"
#include "bsp_i2c.h"
#include "bsp_pins.h"
#include "buzzoff_app.h"
#include "esp_log.h"

static const char *TAG = "buzzoff";

/* The button component invokes this outside LVGL context. Atomics keep this
 * callback non-blocking; the app timer consumes the events in LVGL context. */
static void on_key(bsp_btn_t button, bsp_btn_ev_t event, void *user)
{
    (void)user;
    buzzoff_app_post_key(button, event);
}

void app_main(void)
{
    bsp_i2c_init();
    if (bsp_display_init() != ESP_OK || !bsp_lvgl_init()) {
        ESP_LOGE(TAG, "Display init failed (MOSI=%d SCLK=%d CS=%d DC=%d BL=%d)",
                 BSP_LCD_MOSI, BSP_LCD_SCLK, BSP_LCD_CS, BSP_LCD_DC, BSP_LCD_BL);
        return;
    }
    bsp_display_backlight(100);
    while (!bsp_lvgl_lock(1000)) ESP_LOGW(TAG, "Waiting for LVGL to show boot");
    buzzoff_app_show_boot();
    bsp_lvgl_unlock();

    bool audio_ready = bsp_audio_init() == ESP_OK;
    buzzoff_app_start_audio(audio_ready);
    bool buttons_ready = bsp_button_init(on_key, NULL) == ESP_OK;
    bsp_battery_init();
    while (!bsp_lvgl_lock(1000)) ESP_LOGW(TAG, "Waiting for LVGL to enter app");
    buzzoff_app_start(buttons_ready);
    bsp_lvgl_unlock();
    ESP_LOGI(TAG, "Buzz Off ready: audio=%d buttons=%d", audio_ready, buttons_ready);
}
