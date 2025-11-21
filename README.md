# Camera REST API for Pi Zero 2W

REST API server for controlling a custom USB camera sensor on Raspberry Pi Zero 2W, designed for integration with SwiftImager desktop application.

## Features

- **REST API** with 5 endpoints for camera control
- **Dual connectivity**: USB gadget networking (192.168.7.2) or WiFi
- **Binary image responses** for efficient transfer
- **Persistent camera settings** saved across reboots
- **Placeholder camera implementation** ready for SDK integration
- **Systemd service** for automatic startup
- **Python client library** for desktop app integration

## Hardware Requirements

- Raspberry Pi Zero 2W
- Pi OS Lite (Bookworm or Bullseye)
- Python 3.9+ (tested with Python 3.13)
- Custom USB camera sensor (placeholder implementation included)
- USB cable for data connection (not just power!)

## Quick Start

### On Pi Zero 2W

```bash
# Clone repository
cd ~
git clone <your-repo-url> camera-api
cd camera-api

# Install service (creates venv, installs dependencies, enables systemd service)
chmod +x setup_scripts/install_service.sh
./setup_scripts/install_service.sh

# Optional: Configure USB gadget mode for USB networking
chmod +x setup_scripts/setup_usb_gadget.sh
sudo ./setup_scripts/setup_usb_gadget.sh
# Reboot after USB gadget setup
```

### On Laptop/Desktop

```bash
# Clone repository
git clone <your-repo-url> camera-api
cd camera-api

# Install client library
pip install requests pillow

# Test connection (USB)
python client/pi_camera_client.py
```

## API Documentation

### Base URL

- **USB**: `http://192.168.7.2:5000`
- **WiFi**: `http://<pi-ip>:5000` or `http://raspberrypi.local:5000`

### Endpoints

#### POST /capture

Capture an image from the camera.

**Request:**
```json
{
  "exposure_time": 100,    // Optional: 10-10000 ms
  "format": "jpeg",        // Optional: jpeg|png
  "quality": 90            // Optional: 1-100 (JPEG only)
}
```

**Response:**
- Binary image data
- Headers:
  - `Content-Type`: `image/jpeg` or `image/png`
  - `X-Camera-Timestamp`: ISO timestamp
  - `X-Camera-Exposure`: Exposure time in ms
  - `X-Camera-Resolution`: `WIDTHxHEIGHT`
  - `X-Image-Size-Bytes`: Image size in bytes

**Example:**
```python
from client.pi_camera_client import PiCameraClient

client = PiCameraClient(host='192.168.7.2')
image_bytes, metadata = client.capture_image(exposure_time=150, quality=95)

# Save image
with open('capture.jpg', 'wb') as f:
    f.write(image_bytes)
```

#### GET /status

Check camera and API status.

**Response:**
```json
{
  "camera_connected": true,
  "camera_model": "Custom USB Camera",
  "api_version": "1.0.0",
  "uptime_seconds": 3600
}
```

#### GET /settings

Get current camera settings.

**Response:**
```json
{
  "exposure_time": 100,
  "gain": 1.0,
  "brightness": 50,
  "contrast": 50
}
```

#### POST /settings

Update camera settings.

**Request:**
```json
{
  "exposure_time": 150,  // Optional: 10-10000 ms
  "gain": 2.0,          // Optional
  "brightness": 60,      // Optional
  "contrast": 55         // Optional
}
```

**Response:**
```json
{
  "status": "success",
  "settings": {
    "exposure_time": 150,
    "gain": 2.0,
    "brightness": 60,
    "contrast": 55
  }
}
```

#### GET /info

Get camera capabilities and specifications.

**Response:**
```json
{
  "capabilities": {
    "max_resolution": [1536, 1030],
    "supported_formats": ["jpeg", "png"],
    "exposure_range": [10, 10000],
    "gain_range": [1.0, 16.0]
  },
  "specifications": {
    "sensor_type": "CMOS",
    "interface": "USB 2.0"
  }
}
```

## Client Library Usage

### Basic Example

```python
from client.pi_camera_client import PiCameraClient

# Initialize client (USB connection)
client = PiCameraClient(host='192.168.7.2')

# Or WiFi connection
# client = PiCameraClient(host='192.168.1.100')  # Pi's WiFi IP
# client = PiCameraClient(host='raspberrypi.local')  # Using hostname

# Test connection
if not client.test_connection():
    print("Cannot connect to camera API")
    exit(1)

# Capture image
image_bytes, metadata = client.capture_image(
    exposure_time=100,
    format='jpeg',
    quality=90
)
print(f"Captured {metadata['size_bytes']} bytes")

# Get status
status = client.get_status()
print(f"Camera connected: {status['camera_connected']}")

# Update settings
client.update_settings(exposure_time=200, gain=1.5)

# Get current settings
settings = client.get_settings()
print(f"Exposure: {settings['exposure_time']} ms")
```

### PySide6 Integration

For PySide6 desktop applications, run camera operations in QThread to avoid blocking the UI:

```python
from PySide6.QtCore import QThread, Signal
from client.pi_camera_client import PiCameraClient

class CaptureThread(QThread):
    image_captured = Signal(bytes, dict)
    error_occurred = Signal(str)

    def __init__(self, client, exposure_time):
        super().__init__()
        self.client = client
        self.exposure_time = exposure_time

    def run(self):
        try:
            image_bytes, metadata = self.client.capture_image(
                exposure_time=self.exposure_time
            )
            self.image_captured.emit(image_bytes, metadata)
        except Exception as e:
            self.error_occurred.emit(str(e))

# In your main window
client = PiCameraClient(host='192.168.7.2')
thread = CaptureThread(client, exposure_time=150)
thread.image_captured.connect(self.on_image_captured)
thread.error_occurred.connect(self.on_error)
thread.start()
```

## USB Gadget Networking Setup

To enable USB networking on Pi Zero 2W:

### 1. Run Setup Script (on Pi)

```bash
chmod +x setup_scripts/setup_usb_gadget.sh
sudo ./setup_scripts/setup_usb_gadget.sh
sudo reboot
```

### 2. Connect to Laptop

- Use **data-capable USB cable** (not power-only)
- Connect to USB data port on Pi Zero (inner port, not power port)
- Wait ~30 seconds after Pi boots

### 3. Configure Laptop

**Windows:**
1. Control Panel → Network and Internet → Network Connections
2. Find new network adapter (RNDIS/Ethernet Gadget)
3. Properties → TCP/IPv4 → Use the following IP:
   - IP: `192.168.7.1`
   - Subnet: `255.255.255.0`
4. Test: `ping 192.168.7.2`

**Linux:**
```bash
# Should auto-configure, if not:
sudo ip addr add 192.168.7.1/24 dev usb0
ping 192.168.7.2
```

**macOS:**
```bash
# Find interface (usually en5 or similar)
ifconfig
sudo ifconfig en5 192.168.7.1 netmask 255.255.255.0
ping 192.168.7.2
```

## Development Workflow

### Update Code on Pi

```bash
# On Pi Zero 2W
cd ~/camera-api
git pull
sudo systemctl restart camera-api
```

### View Logs

```bash
# Follow live logs
sudo journalctl -u camera-api -f

# Last 50 lines
sudo journalctl -u camera-api -n 50

# Since boot
sudo journalctl -u camera-api -b
```

### Service Management

```bash
# Start service
sudo systemctl start camera-api

# Stop service
sudo systemctl stop camera-api

# Restart service
sudo systemctl restart camera-api

# Check status
sudo systemctl status camera-api

# Disable auto-start
sudo systemctl disable camera-api

# Enable auto-start
sudo systemctl enable camera-api
```

## Project Structure

```
sl-wb-pi-api/
├── camera_api.py              # Main Flask application
├── config.py                  # Configuration settings
├── requirements.txt           # Python dependencies
├── camera/
│   ├── __init__.py
│   └── controller.py          # Camera abstraction layer (placeholder)
├── client/
│   ├── __init__.py
│   └── pi_camera_client.py    # Python client library
├── config/
│   └── camera_settings.json   # Persistent settings (auto-created)
├── systemd/
│   └── camera-api.service     # Systemd service file
├── setup_scripts/
│   ├── setup_usb_gadget.sh    # USB gadget configuration
│   └── install_service.sh     # Service installation
└── README.md                  # This file
```

## Configuration

### Environment Variables

Set environment variables to customize API behavior:

```bash
# Enable debug mode (verbose logging)
export FLASK_DEBUG=true

# Set log level
export LOG_LEVEL=DEBUG  # DEBUG, INFO, WARNING, ERROR

# Run API manually with custom settings
cd ~/camera-api
source venv/bin/activate
python camera_api.py
```

### Camera Settings Persistence

Camera settings are saved to `config/camera_settings.json` and persist across:
- API restarts
- Pi reboots
- Service updates

Default settings:
```json
{
  "exposure_time": 100,
  "gain": 1.0,
  "brightness": 50,
  "contrast": 50
}
```

## Integrating Real Camera SDK

The current implementation uses a placeholder camera. To integrate your actual USB camera:

### 1. Edit `camera/controller.py`

Replace the `capture()` method in the `CameraController` class:

```python
def capture(self, exposure_time: int | None = None,
            format: str = Config.DEFAULT_FORMAT,
            quality: int = Config.DEFAULT_QUALITY) -> tuple[bytes, dict]:
    """Capture image from real camera."""

    # Your camera SDK code here
    import your_camera_sdk

    # Set exposure
    exp_time = exposure_time if exposure_time else self.settings['exposure_time']
    your_camera_sdk.set_exposure(exp_time)

    # Capture image
    image_data = your_camera_sdk.capture()

    # Convert to bytes
    buffer = BytesIO()
    if format.lower() == 'png':
        image_data.save(buffer, format='PNG')
    else:
        image_data.save(buffer, format='JPEG', quality=quality)

    image_bytes = buffer.getvalue()

    # Return image and metadata
    metadata = {
        'timestamp': datetime.now().isoformat(),
        'exposure_time': exp_time,
        'resolution': f"{image_data.width}x{image_data.height}",
        'size_bytes': len(image_bytes),
        'format': format,
        'quality': quality if format.lower() == 'jpeg' else None
    }

    return image_bytes, metadata
```

### 2. Add Camera SDK to Requirements

Edit `requirements.txt`:
```
flask
flask-cors
pillow
requests
your-camera-sdk==1.2.3  # Add your camera SDK
```

### 3. Update and Restart

```bash
cd ~/camera-api
source venv/bin/activate
pip install -r requirements.txt
sudo systemctl restart camera-api
```

## Troubleshooting

### API Won't Start

```bash
# Check logs
sudo journalctl -u camera-api -n 50

# Common issues:
# - Missing dependencies: pip install -r requirements.txt
# - Port already in use: sudo lsof -i :5000
# - Permission issues: Check file ownership
```

### USB Connection Not Working

```bash
# On Pi - Check interface
ip addr show usb0

# Should show: inet 192.168.7.2/24

# On laptop - Test network
ping 192.168.7.2

# If no response:
# 1. Check cable (must be data cable, not power-only)
# 2. Check laptop network settings (should have 192.168.7.1)
# 3. Reboot Pi
# 4. Try different USB port on laptop
```

### Camera Not Detected

```bash
# Check USB devices
lsusb

# Check dmesg for camera messages
dmesg | grep -i camera

# Test with v4l2 (if using V4L2 camera)
v4l2-ctl --list-devices
```

### Connection Timeout

```python
# Increase timeout in client
client = PiCameraClient(host='192.168.7.2', timeout=60)  # 60 seconds
```

### Settings Not Persisting

```bash
# Check config directory permissions
ls -la ~/camera-api/config/

# Should be owned by 'pi' user
# If not:
sudo chown -R pi:pi ~/camera-api/config/
```

## Performance Notes

### Pi Zero 2W Limitations

- **RAM**: 512MB - Keep images reasonable size
- **CPU**: Quad-core ARM Cortex-A53 @ 1GHz - Expect ~1-3 seconds per capture
- **Network**: USB 2.0 (~20-40 MB/s realistic), WiFi 2.4GHz (~5-15 MB/s)

### Optimization Tips

- Use JPEG instead of PNG for faster transfer
- Lower JPEG quality for faster encoding (quality=70-80 is good balance)
- Consider image downscaling for previews
- Use WiFi for wireless convenience, USB for maximum speed

## Contributing

This is a template project designed for easy customization. Feel free to:
- Add authentication (API keys, JWT)
- Implement streaming endpoints
- Add batch capture support
- Extend camera settings
- Add image processing endpoints

## License

MIT License - Free to use and modify for your SwiftImager project.

## Support

For issues or questions:
1. Check logs: `sudo journalctl -u camera-api -f`
2. Test with example client: `python client/pi_camera_client.py`
3. Verify network connectivity: `ping 192.168.7.2`
4. Check camera connection: `lsusb`
