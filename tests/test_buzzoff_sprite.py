import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

from PIL import Image

from tools.buzzoff_sprite import cutout_checkerboard, encode_i4, convert, convert_scene


class BuzzoffSpriteTests(unittest.TestCase):
    def test_cutout_removes_only_exterior_checkerboard(self):
        image = Image.new("RGB", (8, 8))
        for y in range(8):
            for x in range(8):
                image.putpixel((x, y), (240, 240, 240) if (x + y) % 2 else (210, 210, 210))
        for y in range(2, 6):
            for x in range(2, 6):
                image.putpixel((x, y), (10, 48, 30))
        image.putpixel((3, 3), (255, 249, 208))

        result = cutout_checkerboard(image)

        self.assertEqual(result.getpixel((0, 0))[3], 0)
        self.assertEqual(result.getpixel((7, 7))[3], 0)
        self.assertEqual(result.getpixel((2, 2))[3], 255)
        self.assertEqual(result.getpixel((3, 3)), (255, 249, 208, 255))

    def test_i4_has_transparent_palette_entry_and_packed_pixels(self):
        image = Image.new("RGBA", (2, 1))
        image.putpixel((0, 0), (255, 255, 255, 0))
        image.putpixel((1, 0), (80, 160, 40, 255))

        encoded = encode_i4(image)

        self.assertEqual(len(encoded), 65)
        self.assertEqual(encoded[:4], bytes((0, 0, 0, 0)))
        self.assertEqual(encoded[64] >> 4, 0)
        self.assertNotEqual(encoded[64] & 0xF, 0)
        self.assertNotIn(bytes((255, 255, 255, 255)),
                         [encoded[i:i + 4] for i in range(4, 64, 4)])

    def test_scene_conversion_keeps_opaque_image_and_lvgl_dimensions(self):
        with TemporaryDirectory() as directory:
            source = Path(directory) / "scene.png"
            preview = Path(directory) / "preview.png"
            output = Path(directory) / "scene.c"
            Image.new("RGB", (4, 2), (10, 35, 24)).save(source)

            size = convert_scene(source, preview, output, width=4, height=2)

            self.assertEqual(size, 68)
            self.assertEqual(Image.open(preview).getpixel((0, 0)), (10, 35, 24))
            self.assertIn(".header.w = 4", output.read_text())
            self.assertIn(".header.stride = 2", output.read_text())

    def test_open_mouth_variant_gets_distinct_lvgl_symbol(self):
        with TemporaryDirectory() as directory:
            source = Path(directory) / "frog.png"
            preview = Path(directory) / "preview.png"
            output = Path(directory) / "frog_open.c"
            Image.new("RGB", (2, 2), (10, 60, 25)).save(source)

            convert(source, preview, output, width=2, height=2,
                    name="frog_open")

            self.assertIn("buzzoff_frog_open_sprite", output.read_text())


if __name__ == "__main__":
    unittest.main()
