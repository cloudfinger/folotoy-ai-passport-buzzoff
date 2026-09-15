#!/usr/bin/env python3
"""Host tests for the protected firmware-layout parser."""

from __future__ import annotations

import hashlib
import importlib.util
import struct
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "verify_firmware", ROOT / "tools" / "verify_firmware.py"
)
assert SPEC and SPEC.loader
VERIFY = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = VERIFY
SPEC.loader.exec_module(VERIFY)


def sample_table() -> bytes:
    entries = (
        (1, 2, 0x9000, 0x6000, "nvs"),
        (1, 1, 0xF000, 0x1000, "phy_init"),
        (0, 0, 0x10000, 0x300000, "factory"),
        (1, 2, 0x356000, 0x4000, "cardid"),
    )
    raw = bytearray(b"\xff" * VERIFY.PARTITION_TABLE_SIZE)
    for index, (kind, subtype, offset, size, label) in enumerate(entries):
        VERIFY.ENTRY.pack_into(
            raw,
            index * VERIFY.ENTRY.size,
            0x50AA,
            kind,
            subtype,
            offset,
            size,
            label.encode().ljust(16, b"\0"),
            0,
        )
    marker = len(entries) * VERIFY.ENTRY.size
    struct.pack_into("<H", raw, marker, 0xEBEB)
    raw[marker + 16 : marker + 32] = hashlib.md5(raw[:marker]).digest()
    return bytes(raw)


class PartitionParserTest(unittest.TestCase):
    def test_parses_protected_layout_and_md5(self) -> None:
        partitions, found_md5 = VERIFY.parse_partition_table(sample_table())
        self.assertTrue(found_md5)
        self.assertEqual(partitions[-1].label, "cardid")
        self.assertEqual(partitions[-1].offset, VERIFY.CARDID_OFFSET)

    def test_rejects_bad_md5(self) -> None:
        raw = bytearray(sample_table())
        raw[28] ^= 1
        with self.assertRaisesRegex(ValueError, "MD5"):
            VERIFY.parse_partition_table(bytes(raw))


class ProtectedLayoutTest(unittest.TestCase):
    def test_layout_verification_accepts_current_partition_table(self) -> None:
        merged = bytearray(b"\xff" * (0x10000 + 1))
        merged[
            VERIFY.PARTITION_TABLE_OFFSET :
            VERIFY.PARTITION_TABLE_OFFSET + VERIFY.PARTITION_TABLE_SIZE
        ] = sample_table()
        merged[0x10000] = 0xE9

        with tempfile.TemporaryDirectory() as directory:
            build_dir = Path(directory)
            (build_dir / "FoloToy-AI-Passport.bin").write_bytes(b"\xe9")
            VERIFY.verify_protected_layout(bytes(merged), build_dir)

    def test_verifies_buzzoff_named_merged_firmware(self) -> None:
        merged = bytearray(b"\xff" * (0x10000 + 1))
        merged[0] = 0xE9
        merged[
            VERIFY.PARTITION_TABLE_OFFSET :
            VERIFY.PARTITION_TABLE_OFFSET + VERIFY.PARTITION_TABLE_SIZE
        ] = sample_table()
        merged[0x10000] = 0xE9

        with tempfile.TemporaryDirectory() as directory:
            build_dir = Path(directory)
            (build_dir / "bootloader").mkdir()
            (build_dir / "partition_table").mkdir()
            (build_dir / "bootloader/bootloader.bin").write_bytes(b"\xe9")
            (build_dir / "partition_table/partition-table.bin").write_bytes(sample_table())
            (build_dir / "FoloToy-AI-Passport.bin").write_bytes(b"\xe9")
            (build_dir / "folotoy-ai-passport-buzzoff.bin").write_bytes(merged)
            (build_dir / "flash_args").write_text("--flash_size 8MB", encoding="utf-8")

            with mock.patch.object(sys, "argv", ["verify_firmware.py", str(build_dir)]):
                self.assertEqual(VERIFY.main(), 0)


if __name__ == "__main__":
    unittest.main()
