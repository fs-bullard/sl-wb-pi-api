#!/usr/bin/env python3

import gzip
import time
import numpy as np

from src.capture_wrapper import Device

# Global C library handle
libcapture = None

# Constants
EXPOSURE_MIN = 10
EXPOSURE_MAX = 10000
DEFAULT_EXPOSURE = 100

# Global initialised flag
initialised = False

device = Device()

exposure_ms = 100

# Capture frame
print(f"Capturing frame with exposure: {exposure_ms} ms")

time.sleep(1)
data = device.capture_frame(exposure_ms)
print(f"capture_frame returned")

# Copy frame data to Python bytes and compress
frame_bytes = bytes(data)
compressed_bytes = gzip.compress(frame_bytes, compresslevel=1)


img_from_bytes = np.frombuffer(frame_bytes, dtype=np.uint16).reshape((1536, 1031))
img_from_comp_bytes = np.frombuffer(compressed_bytes, dtype=np.uint16).reshape((1536, 1031))

print(f'uncompressed: {np.min(img_from_bytes), np.max(img_from_bytes)}')
print(f'compressed: {np.min(img_from_comp_bytes), np.max(img_from_comp_bytes)}')


print(f"Frame captured: {len(data)} bytes")

