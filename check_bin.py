#!/usr/bin/env python3
import os, struct, sys
p = sys.argv[1] if len(sys.argv) > 1 else "GPTputer_ADV_app.bin"
with open(p, "rb") as f:
    h = f.read(24)
if len(h) < 24:
    raise SystemExit("ERROR: binary is too small")
if h[0] != 0xE9:
    raise SystemExit(f"ERROR: not an ESP application image (magic=0x{h[0]:02X}, expected 0xE9)")
segments = h[1]
flash_mode = h[2]
size_freq = h[3]
chip_id = struct.unpack_from('<H', h, 12)[0]
size = os.path.getsize(p)
print(f"OK: ESP image magic 0xE9")
print(f"segments: {segments}")
print(f"flash mode byte: {flash_mode}")
print(f"flash size/freq byte: 0x{size_freq:02X}")
print(f"chip id: {chip_id} (ESP32-S3 is expected to be 9)")
print(f"file size: {size} bytes ({size/1024/1024:.2f} MiB)")
if chip_id != 9:
    raise SystemExit("ERROR: image was not built for ESP32-S3")
if size > 0x600000:
    raise SystemExit("ERROR: app is larger than the 6 MiB build partition")
print("BIN CHECK PASSED")
