<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Assets

This directory stores reusable fonts, images, music, and sound effects, organized by asset type.

Keep each asset in the matching subdirectory and document its destination, naming, integration method, and source/license. Do not mix binary assets with Markdown documentation.

## Fonts

Store reusable font files and generated font sources in `fonts/`.

- Use descriptive names that include the family, weight, size, and format when relevant.
- Document the source, license, character range, conversion command, and expected destination.
- Check Flash and internal-RAM impact before adding a font; the ESP32-C3 has no PSRAM.
- Do not commit fonts whose license does not permit redistribution.

## Images

Store reusable source images and generated display assets in `images/`.

- `images/buzzoff-community-cover-illustration.png` is a project-original AI-generated 3:4 gameplay illustration for the FoloToy community listing, based on the existing frog and wetland art. It depicts a representative mosquito catch, not a device screenshot or measured result.

- `images/buzzoff-gallery-boot-illustration.png`, `images/buzzoff-gallery-standby-illustration.png`, `images/buzzoff-gallery-catch-illustration.png`, and `images/buzzoff-gallery-croak-illustration.png` are four project-original AI-generated 3:4 community gallery illustrations. They use the existing Buzz Off frog, wetland art, and cover as visual references, without third-party images. They depict the boot reveal, mosquito-free standby, active catch, and bullfrog-call moment respectively; they are representative art, not device screenshots. Upload them as optional gallery images alongside the cover when a new submission revision is authorized.

- `images/buzzoff-frog-source.png` and `images/buzzoff-frog-open-source.png` are the generated closed/open-mouth frog sources, with corresponding `168x172` transparent previews. `images/buzzoff-scene-source.png` is the generated wetland backdrop, with a `240x179` preview. All derive from the creator-selected Buzz Off concept and are project-original AI-generated material; no third-party images were used. Pillow is needed only to regenerate their Flash-resident LVGL I4 (16-color indexed) C sources:

  ```bash
  python3 tools/buzzoff_sprite.py assets/images/buzzoff-frog-source.png assets/images/buzzoff-frog-168x172.png main/buzzoff_frog_sprite.c
  python3 tools/buzzoff_sprite.py --name frog_open assets/images/buzzoff-frog-open-source.png assets/images/buzzoff-frog-open-168x172.png main/buzzoff_frog_open_sprite.c
  python3 tools/buzzoff_sprite.py --scene assets/images/buzzoff-scene-source.png assets/images/buzzoff-scene-240x179.png main/buzzoff_scene_sprite.c
  ```

- Use descriptive names and document dimensions, pixel format, conversion steps, and destination.
- Prefer formats suitable for the 240 × 320 RGB565 display and account for Flash and internal RAM.
- Preserve editable sources where licensing permits, and record the source and license.
- Never commit device QR secrets, credentials, or personal data in images.

## Music and sound effects

Store reusable music and sound-effect sources in `music/`.

- `music/buzzoff-bullfrog-preview.wav` is a 48 kHz, 16-bit mono, one-second excerpt from the [USGS American bullfrog recording](https://www.usgs.gov/media/videos/american-bullfrogs-lithobates-catebeianus), which USGS marks public domain. The excerpt covers 2.2–3.2 seconds, has 15 ms/50 ms edge fades and 3× gain, and contains no synthetic frog voice. `tools/buzzoff_wav_to_c.py` converts it to Flash-resident `main/buzzoff_bullfrog_pcm.c`; the audio worker plays the recording directly and ducks the active tone. To reproduce it, download the source video linked by USGS, decode with FFmpeg using `-af 'atrim=start=2.2:end=3.2,asetpts=PTS-STARTPTS,afade=t=in:st=0:d=0.015,afade=t=out:st=0.95:d=0.05,volume=3.0' -ac 1 -ar 48000 -c:a pcm_s16le`, then run `python3 tools/buzzoff_wav_to_c.py assets/music/buzzoff-bullfrog-preview.wav main/buzzoff_bullfrog_pcm.c`.

- Document the source, license, sample rate, bit depth, channels, conversion command, and destination.
- Match the active codec format (48 kHz, 16-bit mono for Buzz Off).
- Check Flash and internal-RAM cost before embedding audio; stream or chunk long recordings.
- Do not commit media without redistribution permission.
