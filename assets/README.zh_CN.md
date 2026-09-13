<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

# 资源目录（Assets）

本目录集中存放可复用的资源（字库、图片、音乐等），按资源类型分子目录管理。每个资源放在其类型对应的子目录，并记录放置路径、命名方式、集成方式与来源/许可。二进制资源（字体、图片、音频）不属于纯 markdown 文档，请勿与文档混放。涉及版权/授权的资源需注明来源与许可。

## 字库（fonts）

可复用的字库文件与生成的字库源码放在 `fonts/`。

- 命名要能反映字族、字重、字级与格式。
- 记录来源、许可、字符范围、转换命令与目标放置路径。
- 添加字库前评估 Flash 与内部 RAM 影响；ESP32-C3 无 PSRAM。
- 不提交许可不允许分发的字库。

## 图片（images）

可复用的源图与生成的显示资产放在 `images/`。

- `images/buzzoff-frog-source.png` 与 `images/buzzoff-frog-open-source.png` 是生成的青蛙闭嘴／张嘴源图，分别配有 `168x172` 透明预览图；`images/buzzoff-scene-source.png` 是生成的湿地背景，配有 `240x179` 预览图。全部取材于创作者选定的 Buzz Off 设计概念，均为项目原创 AI 生成素材，未使用第三方图片。只有重新生成存放于 Flash 的 LVGL I4（16 色索引）图片源码时才需要 Pillow：

  ```bash
  python3 tools/buzzoff_sprite.py assets/images/buzzoff-frog-source.png assets/images/buzzoff-frog-168x172.png main/buzzoff_frog_sprite.c
  python3 tools/buzzoff_sprite.py --name frog_open assets/images/buzzoff-frog-open-source.png assets/images/buzzoff-frog-open-168x172.png main/buzzoff_frog_open_sprite.c
  python3 tools/buzzoff_sprite.py --scene assets/images/buzzoff-scene-source.png assets/images/buzzoff-scene-240x179.png main/buzzoff_scene_sprite.c
  ```

- 使用描述性命名，并记录尺寸、像素格式、转换步骤与目标路径。
- 优先采用适合 240 × 320 RGB565 显示的格式，并纳入 Flash 与内部 RAM 考量。
- 许可允许时保留可编辑源文件，并记录来源与许可。
- 图片中不得包含设备二维码秘密、凭证或个人数据。

## 音乐与音效（music）

可复用的音乐与音效源码放在 `music/`。

- `music/buzzoff-bullfrog-preview.wav` 是 [USGS 美洲牛蛙录音](https://www.usgs.gov/media/videos/american-bullfrogs-lithobates-catebeianus)中约一秒的片段，格式为 48 kHz、16 位单声道；USGS 标记原素材为公有领域。取原片 2.2–3.2 秒，首尾分别做 15 毫秒／50 毫秒淡入淡出，并放大 3 倍；没有合成青蛙声。`tools/buzzoff_wav_to_c.py` 将其转为存放于 Flash 的 `main/buzzoff_bullfrog_pcm.c`；音频任务直接播放录音，同时降低当前底音。重做时先下载 USGS 原影片，用 FFmpeg 参数 `-af 'atrim=start=2.2:end=3.2,asetpts=PTS-STARTPTS,afade=t=in:st=0:d=0.015,afade=t=out:st=0.95:d=0.05,volume=3.0' -ac 1 -ar 48000 -c:a pcm_s16le` 生成试听 WAV，再执行 `python3 tools/buzzoff_wav_to_c.py assets/music/buzzoff-bullfrog-preview.wav main/buzzoff_bullfrog_pcm.c`。

- 记录来源、许可、采样率、位深、声道、转换命令与目标路径。
- 与当前 codec 格式一致（Buzz Off 使用 48 kHz、16 位单声道）。
- 嵌入音频前评估 Flash 与内部 RAM 成本；长录音应流式或分块。
- 无再分发许可不提交媒体文件。
