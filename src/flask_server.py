#!/usr/bin/env python3
"""
Flask HTTP Server for Western Blot Camera Capture API

Provides HTTP interface for laptop to request camera captures via C library.
Optimized for Pi Zero 2W with minimal resource usage.
"""

import sys
import logging
import signal
from ctypes import (
    CDLL, 
    c_int, 
    c_uint32, 
    c_size_t, 
    POINTER, 
    c_uint8, 
    c_voidp, 
    byref
)
from pathlib import Path

from flask import Flask, request, jsonify, Response

from gpio.led_pwm import LED
from gpio.button_pwm import ButtonLED

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
device_initialized = False

# Global device
device = c_voidp()

# GPIO devices
try:
    led = LED(14)
    button_led = ButtonLED(15, 16)
except Exception as e:
    logger.error(f"Error connecting to GPIO devices: {e}")
    led = None
    button_led = None

# Constants
EXPOSURE_MIN = 10
EXPOSURE_MAX = 10000
DEFAULT_EXPOSURE = 100


def load_c_library():
    """Load the capture C library and define function signatures."""
    global libcapture

    # Find libcapture.so
    lib_path = Path(__file__).parent / "libcapture.so"
    if not lib_path.exists():
        # Try parent directory
        lib_path = Path(__file__).parent.parent / "libcapture.so"

    if not lib_path.exists():
        raise FileNotFoundError(f"libcapture.so not found. Please run 'make' to build it.")

    logger.info(f"Loading C library from: {lib_path}")
    libcapture = CDLL(str(lib_path))

    # Define function signatures
    # Note: xdtusb_device_t* is an opaque pointer, so we use c_void_p

    # int init_device(xdtusb_device_t** ppdev)
    libcapture.init_device.argtypes = [
        POINTER(c_voidp)  # Pointer to device pointer
    ]
    libcapture.init_device.restype = c_int

    # int set_capture_settings(xdtusb_device_t* pdev, uint32_t exposure_us)
    libcapture.set_capture_settings.argtypes = [
        c_voidp,  # Device pointer
        c_uint32
    ]
    libcapture.set_capture_settings.restype = c_int

    # int capture_frame(xdtusb_device_t* pdev)
    libcapture.capture_frame.argtypes = [
        c_voidp  # Device pointer
    ]
    libcapture.capture_frame.restype = c_int

    # int cleanup_capture_device(xdtusb_device_t* pdev)
    libcapture.cleanup_capture_device.argtypes = [
        c_voidp  # Device pointer
    ]
    libcapture.cleanup_capture_device.restype = c_int

    # int get_frame_data(uint8_t** data, uint32_t* size,
    #                    uint32_t* width, uint32_t* height)
    libcapture.get_frame_data.argtypes = [
        POINTER(POINTER(c_uint8)),  # data
        POINTER(c_uint32),  # size
        POINTER(c_uint32),  # width
        POINTER(c_uint32)  # height
    ]
    libcapture.get_frame_data.restype = c_int

    # void clear_frame_data()
    libcapture.clear_frame_data.argtypes = []
    libcapture.clear_frame_data.restype = None

    logger.info("C library loaded successfully")


def initialize_device():
    """Initialize the camera device."""
    global device_initialized
    global device

    if device_initialized:
        logger.info("Device already initialized")
        return True

    logger.info("Initializing capture device...")
    err = libcapture.init_device(byref(device))

    if err == 0:
        device_initialized = True
        logger.info("Device initialized successfully")
        return True
    else:
        logger.error(f"Failed to initialise device")
        return False
    
def set_capture_settings(exposure_ms: int):
    global device_initialized
    global device

    if not device_initialized:
        logger.info("Device not initialized")
        return False

    # Convert milliseconds to microseconds
    exposure_us = exposure_ms * 1000
    err = libcapture.set_capture_settings(device, c_uint32(exposure_us))

    if err == 0:
        logger.info(f"Set exposure time to {exposure_ms}ms ({exposure_us}us)")
        return True
    else:
        logger.error(f"Failed to set exposure time to {exposure_ms}ms")
        return False

def capture_frame():
    global device_initialized
    global device

    if not device_initialized:
        logger.info("Device not initialized")
        return False
    
    err = libcapture.capture_frame(device)
    if err == 0:
        logger.info('Successfully captured frame')
        return True
    else:
        logger.error('Failed to capture frame')
        return False

def cleanup_capture_device():
    global device_initialized
    global device

    if not device_initialized:
        logger.info("Device not initialized")
        return False
    
    err = libcapture.cleanup_capture_device(device)
    if err == 0:
        logger.info('Successfully cleaned up capture device')
        return True
    else:
        logger.error('Failed to cleanup device')
        return False
    
def get_frame_data(datap, size, width, height):
    global device_initialized
    global device

    if not device_initialized:
        logger.info("Device not initialized")
        return False
    
    err = libcapture.get_frame_data(byref(datap), byref(size), byref(width), byref(height))
    
    if err == 0:
        logger.info('Successfully retrieved frame data')
        return True
    else:
        logger.error('Failed to retrieve frame data')
        raise Exception("Failed to retrieve farme data")
    
@app.route('/init', methods=['POST'])
def init():
    ret = initialize_device()

    global device_initialized

    if ret and device_initialized:
        return jsonify({
            'status': 'ready',
            'device': 'connected'
        }), 200
    else:
        return jsonify({
            'status': 'error',
            'message': 'Failed to initialize device'
        }), 500


@app.route('/health', methods=['GET'])
def health():
    """
    Check device status and readiness.

    Response:
        200: Device ready
        503: Device not ready
    """
    if device_initialized:
        return jsonify({
            'status': 'ready',
            'device': 'connected'
        }), 200
    else:
        return jsonify({
            'status': 'not_ready',
            'device': 'not_initialized'
        }), 503

@app.route('/led_on', methods=['POST'])
def led_on():
    """
    Turn LED on
    """
    if not device_initialized:
        return jsonify({
            'status': 'error',
            'message': 'Device not initialized'
        }), 503

    global led
    if led is not None:
        led.set_on(True)

    return jsonify({
        'status': 'success',
        'message': 'LED turned on'
    }), 200

@app.route('/led_off', methods=['POST'])
def led_off():
    """
    Turn LED off
    """
    if not device_initialized:
        return jsonify({
            'status': 'error',
            'message': 'Device not initialized'
        }), 503

    global led
    if led is not None:
        led.set_on(False)

    return jsonify({
        'status': 'success',
        'message': 'LED turned off'
    }), 200

@app.route('/capture', methods=['POST'])
def capture():
    """
    Capture an image from the camera.

    Request JSON:
        {
            "exposure_ms": 1000  # Required: 10-10000 ms
        }

    Response:
        Binary image data (raw uint16 pixels)
        Headers:
            - X-Frame-Width: Frame width in pixels
            - X-Frame-Height: Frame height in pixels
            - X-Pixel-Size: Bytes per pixel (2)
            - X-Exposure-Ms: Exposure time in ms
            - Content-Type: application/octet-stream
    """
    if not device_initialized:
        return jsonify({
            'status': 'error',
            'message': 'Device not initialized'
        }), 503

    # Parse request
    data = request.get_json()
    if not data or 'exposure_ms' not in data:
        return jsonify({
            'status': 'error',
            'message': 'Missing exposure_ms parameter'
        }), 400

    exposure_ms = data['exposure_ms']

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
    
    # Set capture settings
    try:
        ret = set_capture_settings(exposure_ms)
    except Exception as e:
        logger.error(f"Exception in set_capture_settings: {e}", exc_info=True)
        return jsonify({
            'status': 'error',
            'message': f'C library error: {str(e)}'
        }), 500
    
    if not ret:
        logger.error(f"Set capture settings failed")
        return jsonify({
            'status': 'error',
            'message': 'Set Capture Settings Failed'
        }), 500


    # Capture frame
    logger.info(f"Capturing frame with exposure: {exposure_ms} ms")

    try:
        ret = capture_frame()
        logger.info(f"capture_frame returned: {ret}")
    except Exception as e:
        logger.error(f"Exception in capture_frame: {e}", exc_info=True)
        return jsonify({
            'status': 'error',
            'message': f'C library error: {str(e)}'
        }), 500

    if not ret:
        logger.error(f"Capture failed")
        return jsonify({
            'status': 'error',
            'message': 'Capture Failed'
        }), 500

    # Get frame data
    width = c_uint32()
    height = c_uint32()
    data_ptr = POINTER(c_uint8)()
    data_size = c_size_t()

    try:
        rtn = get_frame_data(
            data_ptr,
            data_size,
            width,
            height
        )
        logger.info(f"get_frame_data returned: {width.value}x{height.value}, {data_size.value} bytes")
    except Exception as e:
        logger.error(f"Exception in get_frame_data: {e}", exc_info=True)
        return jsonify({
            'status': 'error',
            'message': f'Failed to get frame data: {str(e)}'
        }), 500

    # Copy frame data to Python bytes
    try:
        frame_bytes = bytes(data_ptr[:data_size.value])
    except Exception as e:
        logger.error(f"Exception copying frame data: {e}", exc_info=True)
        return jsonify({
            'status': 'error',
            'message': f'Failed to copy frame data: {str(e)}'
        }), 500

    logger.info(f"Frame captured: {width.value}x{height.value}, {data_size.value} bytes")

    # Create response with binary data
    response = Response(frame_bytes, mimetype='application/octet-stream')
    response.headers['X-Frame-Width'] = str(width.value)
    response.headers['X-Frame-Height'] = str(height.value)
    response.headers['X-Exposure-Ms'] = str(exposure_ms)

    return response


@app.route('/shutdown', methods=['POST'])
def shutdown():
    """
    Graceful device shutdown.

    Response:
        200: Shutdown successful
    """
    global device_initialized

    logger.info("Shutting down device...")

    # if device_initialized:
    #     libcapture.cleanup_capture_device()
    #     device_initialized = False

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
    global device_initialized

    # Register signal handlers
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    logger.info("=" * 60)
    logger.info("Western Blot Capture API Server")
    logger.info("=" * 60)

    try:
        # Load C library
        load_c_library()

        # Initialize device
        # if not initialize_device():
        #     logger.error("Failed to initialize device. Exiting.")
        #     sys.exit(1)

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
        # if device_initialized:
        #     libcapture.cleanup_capture_device()
        sys.exit(1)


if __name__ == '__main__':
    main()
