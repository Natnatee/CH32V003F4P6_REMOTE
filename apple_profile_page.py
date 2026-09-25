"""Build the 64-byte Apple profile page for the existing flash_storage v4 format.

Run: python apple_profile_page.py
This only writes a local binary file; it does not access the programmer or MCU.
Before programming the page, back up the current 64 bytes at 0x08003DC0.
The Apple IR commands here are the existing, unverified values from src/main.c.
"""

from pathlib import Path
import struct

PAGE_ADDRESS = 0x08003DC0
PAGE_SIZE = 64
PROFILE_MAGIC = 0xA55A0003
META_V4_FLAG = 0x80
OUTPUT = Path(__file__).with_name("apple_profile_full_page.bin")

# Indices correspond to buttons 1, 2, 3, 5, 6, 7, 9, 10, 11, 13, 14, 15.
# Stored codes for buttons 1 and 6 bypass the firmware's extra wake-up frame.
COMMANDS = (0x5E, 0x0B, 0, 0x08, 0x5D, 0x07, 0x02, 0x0D, 0, 0, 0, 0)


def build_page():
    codes = [0x590087EE | (command << 16) if command else 0
             for command in COMMANDS]
    # 12 codes + magic + 8-byte name + v4 protocol masks (both zero = NEC).
    page = struct.pack("<12II8sI", *codes, PROFILE_MAGIC, b"APPLE\0\0\0", META_V4_FLAG)
    assert len(page) == PAGE_SIZE
    return page


if __name__ == "__main__":
    with OUTPUT.open("xb") as file:
        file.write(build_page())
    print(f"Created {OUTPUT.name}: {PAGE_SIZE} bytes for profile 08 at 0x{PAGE_ADDRESS:08X}")
