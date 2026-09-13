idf_component_register(
    SRCS "main.c"
         "buzzoff_app.c"
         "buzzoff_frog_sprite.c"
         "buzzoff_frog_open_sprite.c"
         "buzzoff_scene_sprite.c"
         "buzzoff_boot.c"
         "buzzoff_croak.c"
         "buzzoff_bullfrog_pcm.c"
         "buzzoff_logic.c"
         "buzzoff_music.c"
         "buzzoff_tone.c"
    INCLUDE_DIRS "."
    REQUIRES bsp esp_timer
)
