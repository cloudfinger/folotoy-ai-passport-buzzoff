#!/usr/bin/env python3
"""Convert a 48 kHz mono, signed 16-bit WAV to Flash-resident C samples."""

import argparse
import struct
import wave
from pathlib import Path


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    with wave.open(str(args.input), "rb") as source:
        if (source.getnchannels(), source.getsampwidth(), source.getframerate()) != (1, 2, 48000):
            parser.error("expected 48 kHz, 16-bit mono PCM WAV")
        count = source.getnframes()
        pcm = source.readframes(count)
    samples = [sample for (sample,) in struct.iter_unpack("<h", pcm)]
    if not samples:
        parser.error("WAV must contain samples")

    lines = [
        '/* Generated from assets/music/buzzoff-bullfrog-preview.wav. */',
        '/* Original USGS American bullfrog recording; public domain. */',
        '#include "buzzoff_bullfrog_pcm.h"',
        f'const int16_t buzzoff_bullfrog_pcm[{len(samples)}] = {{',
    ]
    for offset in range(0, len(samples), 12):
        lines.append("    " + ", ".join(str(value) for value in samples[offset:offset + 12]) + ",")
    lines.extend(["};", f"const size_t buzzoff_bullfrog_pcm_count = {len(samples)}U;", ""])
    args.output.write_text("\n".join(lines), encoding="utf-8")


if __name__ == "__main__":
    main()
