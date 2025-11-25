# Western Blot Camera Capture API

C-based camera control API for Raspberry Pi Zero 2W with Flask HTTP interface. Designed for efficient image capture from XDT USB cameras using libxdtusb SDK.

## Architecture

```
Laptop/Desktop → HTTP → Flask Server → ctypes → libcapture.so → libxdtusb.so → Camera
                          (Python)               (Our C code)    (Vendor SDK)
```

**Key Features:**
- **C library wrapper** for direct libxdtusb integration
- **Thread-safe blocking capture** with pthread synchronization
- **Single frame buffer** for memory efficiency
- **Binary response streaming** for minimal network overhead
- **Minimal dependencies** optimized for Pi Zero 2W

## Project Structure

```
sl-wb-pi-api/
├── src/
│   ├── capture.c              # C shared library for camera control
│   ├── capture.h              # Header file
│   └── flask_server.py        # Flask HTTP server
├── scripts/
│   ├── install.sh             # One-time installation
│   └── deploy.sh              # Quick rebuild and restart
├── config/
│   └── capture.service        # Systemd service file
├── library_examples/
│   ├── xdtusb.h               # XDT USB SDK header
│   └── libxdtusb.so           # XDT USB SDK library (place here)
├── Makefile                   # Build configuration
└── requirements.txt           # Python dependencies (Flask only)
```

## Hardware Requirements

- Raspberry Pi Zero 2W
- Raspberry Pi OS Lite (Bookworm recommended)
- XDT USB camera sensor
- libxdtusb SDK files in `library_examples/`

## Installation

### On Raspberry Pi Zero 2W

```bash
# 1. Clone repository
cd /home/freddie
git clone <your-repo-url> sl-wb-pi-api
cd sl-wb-pi-api

# 2. Ensure libxdtusb.so is in library_examples/
ls library_examples/libxdtusb.so

# 3. Run installation script
chmod +x scripts/install.sh
./scripts/install.sh
```

The installation script will:
1. Install system dependencies (build-essential, libcurl, libusb)
2. Install libxdtusb to `/usr/local/lib/`
3. Build `libcapture.so`
4. Create Python virtual environment
5. Install Flask
6. Install and start systemd service

### Verify Installation

```bash
# Check service status
sudo systemctl status capture

# View logs
sudo journalctl -u capture -f

# Test health endpoint
curl http://localhost:5000/health
```

## API Documentation

### Base URL
- **Local**: `http://localhost:5000`
- **Network**: `http://<pi-ip>:5000`

### Endpoints

#### `GET /health`

Check device status and readiness.

**Response 200 (Ready):**
```json
{
  "status": "ready",
  "device": "connected"
}
```

**Response 503 (Not Ready):**
```json
{
  "status": "not_ready",
  "device": "not_initialized"
}
```

#### `POST /capture`

Capture a single frame from the camera.

**Request:**
```json
{
  "exposure_ms": 1000
}
```

**Parameters:**
- `exposure_ms` (required): Exposure time in milliseconds (10-10000)

**Response 200:**
- **Content-Type**: `application/octet-stream`
- **Body**: Raw binary frame data (uint16 pixels, little-endian)
- **Headers**:
  - `X-Frame-Width`: Frame width in pixels
  - `X-Frame-Height`: Frame height in pixels
  - `X-Pixel-Size`: Bytes per pixel (always 2)
  - `X-Exposure-Ms`: Exposure time used

**Response 400 (Validation Error):**
```json
{
  "status": "error",
  "message": "exposure_ms must be between 10 and 10000"
}
```

**Response 500 (Capture Failed):**
```json
{
  "status": "error",
  "message": "Capture timeout"
}
```

**Example (curl):**
```bash
# Capture with 100ms exposure, save to file
curl -X POST http://localhost:5000/capture \
  -H 'Content-Type: application/json' \
  -d '{"exposure_ms": 100}' \
  --output capture.raw

# View metadata from headers
curl -i -X POST http://localhost:5000/capture \
  -H 'Content-Type: application/json' \
  -d '{"exposure_ms": 500}' \
  --output capture.raw
```

**Example (Python):**
```python
import requests
import numpy as np

# Capture image
response = requests.post(
    'http://192.168.1.100:5000/capture',
    json={'exposure_ms': 1000}
)

if response.status_code == 200:
    # Get metadata from headers
    width = int(response.headers['X-Frame-Width'])
    height = int(response.headers['X-Frame-Height'])

    # Parse raw uint16 data
    raw_data = response.content
    image = np.frombuffer(raw_data, dtype=np.uint16)
    image = image.reshape((height, width))

    print(f"Captured: {width}x{height}, exposure: {response.headers['X-Exposure-Ms']}ms")
else:
    print(f"Error: {response.json()}")
```

#### `POST /shutdown`

Gracefully shutdown camera device.

**Response 200:**
```json
{
  "status": "success",
  "message": "Device shutdown complete"
}
```

## Development Workflow

### Build and Deploy

```bash
# Quick rebuild and restart
cd /home/freddie/sl-wb-pi-api
./scripts/deploy.sh
```

### Manual Build

```bash
# Build C library
make clean
make

# Restart service
sudo systemctl restart capture
```

### View Logs

```bash
# Follow live logs
sudo journalctl -u capture -f

# Last 50 lines
sudo journalctl -u capture -n 50

# Since boot
sudo journalctl -u capture -b
```

### Service Management

```bash
# Start
sudo systemctl start capture

# Stop
sudo systemctl stop capture

# Restart
sudo systemctl restart capture

# Status
sudo systemctl status capture

# Disable auto-start
sudo systemctl disable capture

# Enable auto-start
sudo systemctl enable capture
```

## Memory & Performance

### Optimizations for Pi Zero 2W

1. **Single frame buffer**: Only 1 buffer allocated (~3MB for 1536×1030 sensor)
2. **No Python image processing**: Raw binary transfer, no PIL/numpy on Pi
3. **Static allocation**: Frame buffer reused across captures
4. **Single-threaded Flask**: Minimal context switching overhead
5. **Direct memory access**: ctypes provides zero-copy access to C buffer

### Expected Performance

- **Memory usage**: ~30-50MB (Flask + C library + frame buffer)
- **Capture latency**: Exposure time + ~100-500ms overhead
- **Network transfer**: ~3MB raw data @ USB 2.0 speeds (~20-40MB/s)

## Troubleshooting

### Service Won't Start

```bash
# Check logs
sudo journalctl -u capture -n 50

# Common issues:
# - libxdtusb not installed: Run install.sh again
# - libcapture.so not built: make clean && make
# - Camera not connected: Check USB connection
```

### Capture Timeout

**Symptoms**: Capture returns error "Capture timeout"

**Solutions**:
1. Check camera is connected: `lsusb | grep XDT`
2. Verify camera responds: Check dmesg for USB errors
3. Increase exposure time (camera may need longer exposure)
4. Restart service: `sudo systemctl restart capture`

### "Device not initialized" Error

**Symptoms**: `/capture` returns "Device not initialized"

**Solutions**:
1. Check service logs: `sudo journalctl -u capture -f`
2. Look for device detection errors on startup
3. Verify camera is connected before service starts
4. Restart service after connecting camera

### USB Permission / ACCESS Error

**Symptoms**: Service logs show "XDTUSB_DETECT: Could not create device information (ACCESS)"

**Cause**: User doesn't have permission to access USB device

**Solution**:
```bash
# Run the permission fix script
sudo ./scripts/fix_usb_permissions.sh

# Reconnect camera or reboot
sudo reboot

# Restart service
sudo systemctl restart capture
```

**Manual Fix**:
```bash
# Find your device
lsusb | grep -i xdt

# Create udev rule (replace VENDOR_ID and PRODUCT_ID)
sudo nano /etc/udev/rules.d/99-xdtusb.rules

# Add this line:
SUBSYSTEM=="usb", ATTRS{idVendor}=="VENDOR_ID", ATTRS{idProduct}=="PRODUCT_ID", MODE="0666", GROUP="plugdev"

# Reload rules
sudo udevadm control --reload-rules
sudo udevadm trigger

# Reconnect camera
```

### Library Loading Error

**Symptoms**: "libcapture.so not found" or "cannot open shared object file"

**Solutions**:
```bash
# Rebuild library
cd /home/freddie/sl-wb-pi-api
make clean
make

# Verify library exists
ls -lh libcapture.so

# Check library dependencies
ldd libcapture.so

# If libxdtusb missing:
sudo ldconfig
```

## Testing

### Health Check

```bash
curl http://localhost:5000/health
```

Expected output:
```json
{"status": "ready", "device": "connected"}
```

### Capture Test

```bash
# Capture with 100ms exposure
curl -X POST http://localhost:5000/capture \
  -H 'Content-Type: application/json' \
  -d '{"exposure_ms": 100}' \
  --output test_100ms.raw

# Verify file size (should be width × height × 2 bytes)
ls -lh test_100ms.raw
```

### Load Test

```bash
# Multiple captures
for i in {1..10}; do
  echo "Capture $i..."
  curl -X POST http://localhost:5000/capture \
    -H 'Content-Type: application/json' \
    -d '{"exposure_ms": 100}' \
    --output test_$i.raw
done
```

## Technical Details

### C Library Implementation

The `libcapture.so` library provides thread-safe camera operations:

**Functions:**
- `init_capture_device()`: Initialize XDT device with 1 frame buffer
- `capture_frame(exposure_ms)`: Blocking capture with timeout
- `get_frame_data(...)`: Retrieve captured frame metadata and data pointer
- `cleanup_capture_device()`: Release resources

**Synchronization:**
- Uses pthread mutex and condition variable
- Frame callback signals when capture complete
- Timeout = exposure_time + 10 seconds

**Memory Management:**
- Single static frame buffer
- Reused across captures (no malloc/free per frame)
- Data valid until next capture

### Flask Server Implementation

**ctypes Integration:**
- Loads `libcapture.so` at startup
- Defines C function signatures
- Provides zero-copy access to frame buffer

**Resource Management:**
- Single-threaded (threaded=False)
- Streams response directly from C buffer
- No temporary Python objects for image data

## Contributing

For issues or improvements:
1. Test changes locally
2. Build with `make clean && make`
3. Verify with deployment script
4. Check service logs for errors

## License

MIT License - Free to use and modify.

## Support

For issues:
1. Check service logs: `sudo journalctl -u capture -f`
2. Verify camera connection: `lsusb`
3. Test with health endpoint
4. Review error messages in logs
