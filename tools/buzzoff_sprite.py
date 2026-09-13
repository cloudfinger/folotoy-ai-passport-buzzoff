"""Convert the selected frog artwork into a Flash-resident LVGL I4 sprite."""

import argparse
from pathlib import Path

from PIL import Image, ImageDraw


def cutout_checkerboard(image):
    """Remove the generated grey checkerboard connected to the canvas edge."""
    rgb = image.convert("RGB")
    background = bytes(
        255 if min(color) > 170 and max(color) - min(color) < 24 else 0
        for color in rgb.getdata()
    )
    exterior = Image.frombytes("L", rgb.size, background)
    for x in range(rgb.width):
        for y in (0, rgb.height - 1):
            if exterior.getpixel((x, y)) == 255:
                ImageDraw.floodfill(exterior, (x, y), 128)
    for y in range(rgb.height):
        for x in (0, rgb.width - 1):
            if exterior.getpixel((x, y)) == 255:
                ImageDraw.floodfill(exterior, (x, y), 128)
    rgba = rgb.convert("RGBA")
    rgba.putalpha(exterior.point(lambda value: 0 if value == 128 else 255))
    return rgba


def encode_i4(image):
    """Encode LVGL's 16-entry BGRA palette followed by two pixels per byte."""
    rgba = image.convert("RGBA")
    if rgba.width % 2:
        raise ValueError("I4 width must be even")
    # Invisible checker pixels must not consume scarce palette entries.
    rgb = Image.new("RGB", rgba.size, (7, 27, 22))
    rgb.paste(rgba.convert("RGB"), mask=rgba.getchannel("A"))
    quantized = rgb.quantize(colors=15, method=Image.MEDIANCUT,
                             dither=Image.NONE)
    raw_palette = quantized.getpalette()
    palette = bytearray((0, 0, 0, 0))
    for index in range(15):
        color = raw_palette[index * 3:index * 3 + 3]
        red, green, blue = color if len(color) == 3 else (0, 0, 0)
        palette.extend((blue, green, red, 255))
    encoded = bytearray(palette)
    pixels = list(quantized.getdata())
    alpha = list(rgba.getchannel("A").getdata())
    for y in range(rgba.height):
        for x in range(0, rgba.width, 2):
            offset = y * rgba.width + x
            high = pixels[offset] + 1 if alpha[offset] >= 128 else 0
            low = (pixels[offset + 1] + 1 if alpha[offset + 1] >= 128 else 0)
            encoded.append((high << 4) | low)
    return bytes(encoded)


def write_c_image(data, output, width, height, name):
    rows = ["    " + ", ".join(f"0x{value:02x}" for value in data[i:i + 16]) + ","
            for i in range(0, len(data), 16)]
    text = (
        f'#include "buzzoff_{name}_sprite.h"\n\n'
        f'static const uint8_t {name}_pixels[] = {{\n'
        + "\n".join(rows) + "\n};\n\n"
        f'const lv_image_dsc_t buzzoff_{name}_sprite = {{\n'
        '    .header.cf = LV_COLOR_FORMAT_I4,\n'
        f'    .header.w = {width},\n'
        f'    .header.h = {height},\n'
        f'    .header.stride = {width // 2},\n'
        f'    .data_size = sizeof({name}_pixels),\n'
        f'    .data = {name}_pixels,\n'
        '};\n'
    )
    Path(output).write_text(text)


def convert(source, preview, output, width=168, height=172, name="frog"):
    if width % 2:
        raise ValueError("I4 width must be even")
    artwork = cutout_checkerboard(Image.open(source))
    sprite = artwork.resize((width, height), Image.NEAREST)
    sprite.save(preview)
    data = encode_i4(sprite)
    write_c_image(data, output, width, height, name)
    return len(data)


def convert_scene(source, preview, output, width=240, height=179):
    if width % 2:
        raise ValueError("I4 width must be even")
    scene = Image.open(source).convert("RGB").resize((width, height), Image.NEAREST)
    scene.save(preview)
    data = encode_i4(scene)
    write_c_image(data, output, width, height, "scene")
    return len(data)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--scene", action="store_true")
    parser.add_argument("--name", default="frog", choices=("frog", "frog_open"))
    parser.add_argument("source", type=Path)
    parser.add_argument("preview", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    if args.scene:
        size = convert_scene(args.source, args.preview, args.output)
    else:
        size = convert(args.source, args.preview, args.output, name=args.name)
    print(f"Encoded {size} bytes")


if __name__ == "__main__":
    main()
