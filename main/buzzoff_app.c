#include "buzzoff_app.h"
#include "buzzoff_boot.h"
#include "buzzoff_bullfrog_pcm.h"
#include "buzzoff_croak.h"
#include "buzzoff_frog_sprite.h"
#include "buzzoff_frog_open_sprite.h"
#include "buzzoff_logic.h"
#include "buzzoff_scene_sprite.h"
#include "buzzoff_music.h"
#include "buzzoff_tone.h"
#include "bsp_audio.h"
#include "bsp_battery.h"
#include "bsp_display.h"
#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <stdatomic.h>
#include <stdint.h>

#define AUDIO_CHUNK_SAMPLES 256
#define ANIMATION_PERIOD_MS 100
#define FREQUENCY_QUEUE_LENGTH 16

/* Four restrained tones replace the upstream sky/grass/robot visual system. */
#define INK        0x071B16
#define FOREST     0x123629
#define MOSS       0x88B46B
#define LIME       0xB8DC83
#define CREAM      0xF5EACB
#define RUST       0xC45949
#define MUTED      0x6E9278

typedef struct {
    bool running;
    uint32_t frequency_hz;
} audio_command_t;

/* UI state lives on the LVGL/button-lock side; the worker only receives command
 * snapshots through a one-slot queue. The failure bit is the one cross-task flag. */
static buzzoff_state_t s_state;
static buzzoff_boot_t s_boot;
static QueueHandle_t s_audio_queue;
static QueueHandle_t s_frequency_queue;
static atomic_bool s_audio_failed;
static atomic_bool s_audio_stream_ready;
static atomic_uintptr_t s_frequency_queue_handle = ATOMIC_VAR_INIT(0);
static atomic_uint s_ok_clicks = ATOMIC_VAR_INIT(0);
static atomic_uint s_croak_triggers = ATOMIC_VAR_INIT(0);
static bool s_audio_ready;
static bool s_buttons_ready;
static bool s_audio_error_shown;
static bool s_boot_visible;
static bool s_app_ready;
static bool s_frog_open;

static lv_obj_t *s_screen;
static lv_obj_t *s_boot_screen;
static lv_obj_t *s_boot_mask;
static lv_obj_t *s_boot_scan;
static lv_obj_t *s_boot_signature;
static lv_obj_t *s_boot_status;
static lv_obj_t *s_frog;
static lv_obj_t *s_mosquito;
static lv_obj_t *s_tongue;
static lv_obj_t *s_left_wing;
static lv_obj_t *s_right_wing;
static lv_obj_t *s_status;
static lv_obj_t *s_frequency;
static lv_obj_t *s_score;
static lv_obj_t *s_action;

static lv_obj_t *label(lv_obj_t *parent, int x, int y, const char *text,
                       const lv_font_t *font, uint32_t color)
{
    lv_obj_t *obj = lv_label_create(parent);
    lv_label_set_text(obj, text);
    lv_obj_set_style_text_font(obj, font, 0);
    lv_obj_set_style_text_color(obj, lv_color_hex(color), 0);
    lv_obj_set_pos(obj, x, y);
    return obj;
}

/* Pixel rectangles avoid large bitmap allocations on the no-PSRAM ESP32-C3. */
static lv_obj_t *pixel(lv_obj_t *parent, int x, int y, int width, int height,
                       uint32_t color)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, width, height);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
    return obj;
}

static lv_obj_t *transparent_group(lv_obj_t *parent, int x, int y,
                                    int width, int height)
{
    lv_obj_t *group = pixel(parent, x, y, width, height, INK);
    lv_obj_set_style_bg_opa(group, LV_OPA_TRANSP, 0);
    return group;
}

/* The indexed image lives in Flash. Animation only moves its LVGL image object. */
static void build_frog(lv_obj_t *field)
{
    s_frog = lv_image_create(field);
    lv_image_set_src(s_frog, &buzzoff_frog_sprite);
    lv_obj_set_pos(s_frog, 5, 4);
    s_frog_open = false;
}

static void render_frog(bool open)
{
    if (open == s_frog_open) return;
    lv_image_set_src(s_frog, open ? &buzzoff_frog_open_sprite : &buzzoff_frog_sprite);
    s_frog_open = open;
}

static void build_mosquito(lv_obj_t *field)
{
    s_mosquito = transparent_group(field, 206, 37, 19, 19);
    lv_obj_add_flag(s_mosquito, LV_OBJ_FLAG_HIDDEN);
    s_left_wing = pixel(s_mosquito, 0, 1, 7, 5, CREAM);
    s_right_wing = pixel(s_mosquito, 11, 1, 7, 5, CREAM);
    pixel(s_mosquito, 6, 5, 7, 9, INK);
    pixel(s_mosquito, 8, 7, 3, 5, RUST);
    pixel(s_mosquito, 9, 14, 2, 4, INK);
    pixel(s_mosquito, 2, 12, 4, 2, MOSS);
    pixel(s_mosquito, 13, 12, 4, 2, MOSS);
    pixel(s_mosquito, 4, 0, 2, 3, RUST);
    pixel(s_mosquito, 14, 0, 2, 3, RUST);
}

static void render_labels(void)
{
    if (!s_audio_ready || atomic_load(&s_audio_failed)) {
        lv_label_set_text(s_status, "AUDIO ERR");
    } else if (!s_buttons_ready) {
        lv_label_set_text(s_status, "KEY ERR");
    } else {
        lv_label_set_text(s_status, s_state.running ? "RUNNING" : "PRESS OK");
    }
    lv_label_set_text_fmt(s_frequency, "%lu",
                          (unsigned long)(buzzoff_frequency_hz(&s_state) / 1000U));
    lv_label_set_text_fmt(s_score, "EATEN %03lu", (unsigned long)s_state.eaten);
    lv_label_set_text(s_action, s_state.running ? "OK STOP" : "OK START");
}

/* The top-down dark mask exposes ten rows per tick. The green scan edge and
 * typewritten signature give the loading page its handheld-console feel. */
static void render_boot(bool ready)
{
    uint16_t reveal_y = buzzoff_boot_reveal_y(&s_boot);
    if (reveal_y < 320U) {
        lv_obj_remove_flag(s_boot_mask, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_y(s_boot_mask, reveal_y);
        lv_obj_set_height(s_boot_mask, 320 - reveal_y);
        lv_obj_remove_flag(s_boot_scan, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_y(s_boot_scan, reveal_y > 313U ? 313 : reveal_y);
    } else {
        lv_obj_add_flag(s_boot_mask, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_boot_scan, LV_OBJ_FLAG_HIDDEN);
    }
    char signature[12] = "CLOUDFINGER";
    signature[buzzoff_boot_name_chars(&s_boot)] = '\0';
    lv_label_set_text(s_boot_signature, signature);
    lv_obj_align(s_boot_signature, LV_ALIGN_TOP_MID, 0, 138);
    lv_label_set_text(s_boot_status,
                      buzzoff_boot_show_ready(&s_boot, ready) ? "READY!" : "LOADING...");
}

/* The button task posts ordered frequency keys without waiting for LVGL.
 * OK clicks use an atomic counter so the stop control remains available even
 * if the frequency-key FIFO fills. */
static void consume_input(void)
{
    buzzoff_key_t keys[FREQUENCY_QUEUE_LENGTH];
    size_t count = 0;
    while (s_frequency_queue && count < FREQUENCY_QUEUE_LENGTH &&
           xQueueReceive(s_frequency_queue, &keys[count], 0) == pdTRUE) {
        ++count;
    }
    unsigned ok_clicks = atomic_exchange(&s_ok_clicks, 0);
    if (count == 0 && ok_clicks == 0U) return;

    bool can_toggle = s_audio_ready && s_buttons_ready &&
                      !atomic_load(&s_audio_failed);
    buzzoff_state_apply_keys(&s_state, keys, count,
                             can_toggle ? ok_clicks : 0U);
    if (s_audio_queue && !atomic_load(&s_audio_failed)) {
        audio_command_t command = {
            .running = s_state.running,
            .frequency_hz = buzzoff_frequency_hz(&s_state),
        };
        xQueueOverwrite(s_audio_queue, &command);
    }
    render_labels();
}

/* Runs in LVGL context. UI changes stay here, while PCM remains in its worker. */
static void animation_tick(lv_timer_t *timer)
{
    (void)timer;
    if (s_boot_visible) {
        bool ready = s_app_ready && (!s_audio_ready ||
                     atomic_load(&s_audio_stream_ready) ||
                     atomic_load(&s_audio_failed));
        buzzoff_boot_advance(&s_boot, ready);
        render_boot(ready);
        if (buzzoff_boot_finished(&s_boot) && s_app_ready) {
            atomic_exchange(&s_ok_clicks, 0);
            buzzoff_key_t ignored;
            while (s_frequency_queue &&
                   xQueueReceive(s_frequency_queue, &ignored, 0) == pdTRUE) {}
            lv_screen_load(s_screen);
            lv_obj_delete(s_boot_screen);
            s_boot_screen = NULL;
            s_boot_visible = false;
        }
        return;
    }
    bool was_running = s_state.running;
    consume_input();
    if (atomic_load(&s_audio_failed) && !s_audio_error_shown) {
        s_state.running = false;
        s_audio_error_shown = true;
        render_labels();
    }

    buzzoff_state_advance_render_frame(&s_state, was_running);
    buzzoff_frame_t frame = buzzoff_frame(&s_state);
    if (buzzoff_croak_due(&s_state)) atomic_fetch_add(&s_croak_triggers, 1U);
    render_frog(s_state.running && (frame.chewing || frame.tongue_len > 0U));
    if (frame.mosquito_visible) {
        lv_obj_remove_flag(s_mosquito, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_pos(s_mosquito, frame.mosquito_x, frame.mosquito_y);
    } else {
        lv_obj_add_flag(s_mosquito, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_set_y(s_left_wing, (s_state.tick & 1U) ? 3 : 1);
    lv_obj_set_y(s_right_wing, (s_state.tick & 1U) ? 1 : 3);
    lv_obj_set_y(s_frog, frame.chewing ? 7 : 4);
    if (frame.tongue_len > 0) {
        lv_obj_remove_flag(s_tongue, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_width(s_tongue, frame.tongue_len);
    } else {
        lv_obj_add_flag(s_tongue, LV_OBJ_FLAG_HIDDEN);
    }
    if (frame.chewing && s_state.tick == 16U) render_labels();

}

static void audio_task(void *argument)
{
    (void)argument;
    if (bsp_audio_set_format(BUZZOFF_SAMPLE_RATE, 16, 1) != ESP_OK) {
        atomic_store(&s_audio_failed, true);
        vTaskDelete(NULL);
        return;
    }

    int16_t samples[AUDIO_CHUNK_SAMPLES];
    buzzoff_osc_t osc = {0};
    buzzoff_music_t music = {0};
    buzzoff_croak_t croak = {0};
    audio_command_t command = {.running = false, .frequency_hz = 16000U};
    uint8_t output_volume = 26;
    bsp_audio_set_volume(output_volume);
    atomic_store(&s_audio_stream_ready, true);
    for (;;) {
        audio_command_t next;
        if (xQueueReceive(s_audio_queue, &next, 0) == pdTRUE) {
            bool starting = !command.running && next.running;
            bool returning_to_music = command.running && !next.running;
            command = next;
            if (starting) osc.phase = 0;
            if (returning_to_music) music = (buzzoff_music_t){0};
            if (!command.running) buzzoff_croak_stop(&croak);
        }
        bool croak_chunk = false;
        if (command.running) {
            buzzoff_tone_fill(&osc, command.frequency_hz, samples, AUDIO_CHUNK_SAMPLES);
            if (atomic_exchange(&s_croak_triggers, 0U) != 0U)
                buzzoff_croak_start(&croak, buzzoff_bullfrog_pcm,
                                    buzzoff_bullfrog_pcm_count);
            croak_chunk = croak.active;
            buzzoff_croak_mix(&croak, samples, AUDIO_CHUNK_SAMPLES);
        } else {
            atomic_exchange(&s_croak_triggers, 0U);
            buzzoff_music_fill(&music, samples, AUDIO_CHUNK_SAMPLES);
        }
        uint8_t target_volume = buzzoff_croak_volume(command.running, croak_chunk);
        if (target_volume != output_volume) {
            bsp_audio_set_volume(target_volume);
            output_volume = target_volume;
        }
        if (bsp_audio_write(samples, sizeof(samples)) != ESP_OK) {
            bsp_audio_set_volume(0);
            atomic_store(&s_audio_failed, true);
            break;
        }
    }
    vTaskDelete(NULL);
}

void buzzoff_app_show_boot(void)
{
    if (s_boot_visible) return;
    buzzoff_boot_init(&s_boot);
    atomic_store(&s_audio_failed, false);
    atomic_store(&s_audio_stream_ready, false);
    s_boot_visible = true;
    s_boot_screen = lv_obj_create(NULL);
    lv_obj_remove_flag(s_boot_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_boot_screen, lv_color_hex(INK), 0);
    lv_obj_set_style_border_width(s_boot_screen, 0, 0);
    lv_obj_set_style_pad_all(s_boot_screen, 0, 0);

    pixel(s_boot_screen, 13, 25, 214, 266, FOREST);
    pixel(s_boot_screen, 17, 29, 206, 258, INK);
    pixel(s_boot_screen, 30, 47, 180, 3, MOSS);
    pixel(s_boot_screen, 30, 58, 126, 2, FOREST);
    pixel(s_boot_screen, 30, 257, 180, 3, MOSS);
    lv_obj_t *caption = label(s_boot_screen, 0, 0, "POWER ON", &lv_font_unscii_8, MOSS);
    lv_obj_align(caption, LV_ALIGN_TOP_MID, 0, 88);
    s_boot_signature = label(s_boot_screen, 0, 0, "", &lv_font_unscii_16, CREAM);
    lv_obj_align(s_boot_signature, LV_ALIGN_TOP_MID, 0, 138);
    lv_obj_t *title = label(s_boot_screen, 0, 0, "BUZZ OFF", &lv_font_unscii_16, RUST);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 186);
    s_boot_status = label(s_boot_screen, 0, 0, "LOADING...", &lv_font_unscii_8, MOSS);
    lv_obj_align(s_boot_status, LV_ALIGN_TOP_MID, 0, 231);

    s_boot_mask = pixel(s_boot_screen, 0, 0, 240, 320, INK);
    s_boot_scan = pixel(s_boot_screen, 0, 0, 240, 3, LIME);
    lv_screen_load(s_boot_screen);
    lv_timer_create(animation_tick, ANIMATION_PERIOD_MS, NULL);
}

void buzzoff_app_start_audio(bool audio_ready)
{
    s_audio_ready = audio_ready;
    if (!audio_ready) return;
    s_audio_queue = xQueueCreate(1, sizeof(audio_command_t));
    if (!s_audio_queue || xTaskCreate(audio_task, "buzzoff_audio", 4096,
                                       NULL, 4, NULL) != pdPASS) {
        atomic_store(&s_audio_failed, true);
    }
}

void buzzoff_app_start(bool buttons_ready)
{
    if (!s_boot_visible) buzzoff_app_show_boot();
    buzzoff_state_init(&s_state);
    atomic_store(&s_ok_clicks, 0);
    atomic_store(&s_croak_triggers, 0);
    s_frequency_queue = xQueueCreate(FREQUENCY_QUEUE_LENGTH, sizeof(buzzoff_key_t));
    atomic_store(&s_frequency_queue_handle, (uintptr_t)s_frequency_queue);
    s_buttons_ready = buttons_ready && s_frequency_queue != NULL;
    s_audio_error_shown = !s_audio_ready || atomic_load(&s_audio_failed);

    s_screen = lv_obj_create(NULL);
    lv_obj_remove_flag(s_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_screen, lv_color_hex(INK), 0);
    lv_obj_set_style_border_width(s_screen, 0, 0);
    lv_obj_set_style_pad_all(s_screen, 0, 0);

    label(s_screen, 11, 12, "BUZZ", &lv_font_unscii_16, CREAM);
    label(s_screen, 83, 12, "OFF", &lv_font_unscii_16, RUST);
    label(s_screen, 12, 34, "CLOUDFINGER / 001", &lv_font_unscii_8, MUTED);
    lv_obj_t *battery = label(s_screen, 166, 11, "", &lv_font_unscii_8, CREAM);
    int soc = bsp_battery_soc();
    if (soc >= 0) lv_label_set_text_fmt(battery, "BAT %d%%", soc);
    else lv_label_set_text(battery, "BAT --");
    pixel(s_screen, 11, 48, 185, 2, MOSS);
    pixel(s_screen, 202, 48, 4, 2, MOSS);
    pixel(s_screen, 212, 48, 4, 2, MOSS);
    pixel(s_screen, 222, 48, 4, 2, MOSS);

    lv_obj_t *field = transparent_group(s_screen, 0, 51, 240, 179);
    lv_obj_t *scene = lv_image_create(field);
    lv_image_set_src(scene, &buzzoff_scene_sprite);
    lv_obj_set_pos(scene, 0, 0);
    build_frog(field);
    s_tongue = pixel(field, 136, 50, 1, 3, RUST);
    lv_obj_add_flag(s_tongue, LV_OBJ_FLAG_HIDDEN);
    build_mosquito(field);

    pixel(s_screen, 12, 231, 216, 2, FOREST);
    pixel(s_screen, 32, 247, 20, 2, MOSS);
    pixel(s_screen, 187, 247, 20, 2, MOSS);
    s_status = label(s_screen, 57, 237, "", &lv_font_unscii_16, LIME);
    s_frequency = label(s_screen, 54, 257, "", &lv_font_montserrat_32, CREAM);
    label(s_screen, 105, 271, "kHz", &lv_font_unscii_16, CREAM);
    s_score = label(s_screen, 13, 294, "", &lv_font_unscii_8, CREAM);
    s_action = label(s_screen, 151, 294, "", &lv_font_unscii_8, LIME);
    pixel(s_screen, 12, 308, 216, 1, FOREST);
    label(s_screen, 37, 311, "UP/DN  TUNE", &lv_font_unscii_8, MUTED);
    render_labels();
    s_app_ready = true;
}

void buzzoff_app_post_key(bsp_btn_t button, bsp_btn_ev_t event)
{
    if (event != BSP_BTN_CLICK) return;
    if (button == BSP_BTN_OK) {
        atomic_fetch_add(&s_ok_clicks, 1U);
    } else if (button == BSP_BTN_UP || button == BSP_BTN_DOWN) {
        QueueHandle_t queue = (QueueHandle_t)atomic_load(&s_frequency_queue_handle);
        if (queue) {
            buzzoff_key_t key = button == BSP_BTN_UP ? BUZZOFF_KEY_UP : BUZZOFF_KEY_DOWN;
            xQueueSend(queue, &key, 0);
        }
    }
}
