#!/usr/bin/env python3
"""
Flask HTTP Server for Western Blot Camera Capture API

Provides HTTP interface for laptop to request camera captures via C library.
Optimized for Pi Zero 2W with minimal resource usage.
"""

import sys
import logging
import signal
import gzip

from flask import Flask, request, jsonify, Response

from gpio.led_pwm import LED
from gpio.button_pwm import ButtonLED

from src.capture_wrapper import Device, DeviceError

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

# Flask app (single-threaded for Pi Zero resource constraints)
app = Flask(__name__)

# Global C library handle
libcapture = None

# Global device
device = Device()

# GPIO devices
try:
    led = LED(22)
    # button_led = ButtonLED(15, 16)
except Exception as e:
    logger.error(f"Error connecting to GPIO devices: {e}")
    led = None
    button_led = None

# Constants
EXPOSURE_MIN = 10
EXPOSURE_MAX = 10000
DEFAULT_EXPOSURE = 100

@app.route('/capture', methods=['POST'])
def capture():
    """
    Capture an image from the camera.

    Request JSON:
        {
            "exposure_ms": 1000  # Required: 10-10000 ms
            "led": 0.01 # Required: 0-1.0 
        }

    Response:
        Binary image data (raw uint16 pixels)
        Headers:
            - X-Frame-Width: Frame width in pixels
            - X-Frame-Height: Frame height in pixels
            - X-Pixel-Size: Bytes per pixel (2)
            - X-Exposure-Ms: Exposure time in ms
            - X-LED: led value
            - Content-Type: application/octet-stream
    """

    # Parse request
    data = request.get_json()
    if not data or 'exposure_ms' not in data:
        return jsonify({
            'status': 'error',
            'message': 'Missing exposure_ms parameter'
        }), 400

    exposure_ms = data['exposure_ms']
    led_val = data['led']

    # Validate exposure time
    if not isinstance(exposure_ms, (int, float)):
        return jsonify({
            'status': 'error',
            'message': 'exposure_ms must be a number'
        }), 400

    exposure_ms = int(exposure_ms)

    if not (EXPOSURE_MIN <= exposure_ms <= EXPOSURE_MAX):
        return jsonify({
            'status': 'error',
            'message': f'exposure_ms must be between {EXPOSURE_MIN} and {EXPOSURE_MAX}'
        }), 400
    
    # # Apply LED setting
    # global led
    # if led_val is None:
    #     pass
    # elif isinstance(led_val, (float)) and 0 <= led_val <= 1:
    #     led.set_on(True, led_val)
    # else:
    #     return jsonify({
    #         'status': 'error',
    #         'message': f'led_val must be between {0} and {1}'
    #     }), 400
    
    # Capture frame
    logger.info(f"Capturing frame with exposure: {exposure_ms} ms")

    try:
        data = Device.capture_frame(exposure_ms)
        logger.info(f"capture_frame returned")
    except Exception as e:
        logger.error(f"Exception in capture_frame: {e}", exc_info=True)
        return jsonify({
            'status': 'error',
            'message': f'C library error: {str(e)}'
        }), 500
    # finally:
    #     led.set_on(False)

    # Copy frame data to Python bytes and compress
    try:
        frame_bytes = bytes(data)
        compressed_bytes = gzip.compress(frame_bytes, compresslevel=1)
    except Exception as e:
        logger.error(f"Exception copying frame data: {e}", exc_info=True)
        return jsonify({
            'status': 'error',
            'message': f'Failed to copy frame data: {str(e)}'
        }), 500

    logger.info(f"Frame captured: {len(data)} bytes")

    # Create response with binary data
    response = Response(compressed_bytes, mimetype='application/octet-stream')
    response.headers['X-Frame-Width'] = str(device.width)
    response.headers['X-Frame-Height'] = str(device.height)
    response.headers['X-Exposure-Ms'] = str(exposure_ms)
    response.headers['X-LED'] = str(led_val)
    response.headers['Content-Encoding'] = 'gzip'

    return response


@app.route('/shutdown', methods=['POST'])
def shutdown():
    """
    Graceful device shutdown.

    Response:
        200: Shutdown successful
    """

    logger.info("Shutting down device...")

    device.close_device()

    return jsonify({
        'status': 'success',
        'message': 'Device shutdown complete'
    }), 200


def signal_handler(sig, frame):
    """Handle shutdown signals gracefully."""
    logger.info(f"Received signal {sig}, shutting down...")

    # if device_initialized:
    #     libcapture.cleanup_capture_device()

    sys.exit(0)


def main():
    """Main entry point."""

    # Register signal handlers
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    logger.info("=" * 60)
    logger.info("Western Blot Capture API Server")
    logger.info("=" * 60)

    try:

        # Start Flask server (single-threaded for Pi Zero)
        logger.info("Starting Flask server on 0.0.0.0:5000")
        logger.info("Endpoints:")
        logger.info("  GET  /health   - Check device status")
        logger.info("  POST /capture  - Capture image")
        logger.info("  POST /shutdown - Shutdown device")
        logger.info("=" * 60)

        app.run(
            host='0.0.0.0',
            port=5000,
            debug=False,
            threaded=False  # Single-threaded for Pi Zero resource constraints
        )

    except Exception as e:
        logger.error(f"Fatal error: {e}", exc_info=True)
        sys.exit(1)


if __name__ == '__main__':
    main()
