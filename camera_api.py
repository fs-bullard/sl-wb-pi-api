"""
Camera REST API for Pi Zero 2W.

This Flask application provides REST endpoints for controlling a custom USB camera
sensor via WiFi or USB gadget networking.
"""

import logging
from datetime import datetime
from io import BytesIO

from flask import Flask, request, jsonify, send_file
from flask_cors import CORS

from config import Config
from camera.controller import CameraController


# Configure logging
logging.basicConfig(
    level=Config.LOG_LEVEL,
    format=Config.LOG_FORMAT
)
logger = logging.getLogger(__name__)

# Initialize Flask app
app = Flask(__name__)
CORS(app)  # Enable CORS for desktop app

# Initialize camera controller
camera = CameraController(settings_file=Config.SETTINGS_FILE)

# Track API start time for uptime calculation
API_START_TIME = datetime.now()


@app.route('/capture', methods=['POST'])
def capture():
    """
    Capture an image from the camera.

    Request JSON:
        {
            "exposure_time": 100,    # Optional: 10-10000 ms
            "format": "tif",        # Optional: tif
        }

    Response:
        Binary image data with metadata headers
        - Content-Type: image/jpeg or image/png
        - X-Camera-Timestamp: ISO timestamp
        - X-Camera-Exposure: Exposure time in ms
        - X-Camera-Resolution: WIDTHxHEIGHT
        - X-Image-Size-Bytes: Image size in bytes

    Error Response:
        JSON with status and error message
    """
    try:
        # Parse request JSON
        data = request.get_json() or {}

        # Extract parameters with defaults
        exposure_time = data.get('exposure_time')
        image_format = data.get('format', Config.DEFAULT_FORMAT).lower()

        # Validate exposure time if provided
        if exposure_time is not None:
            if not isinstance(exposure_time, (int, float)):
                return jsonify({
                    'status': 'error',
                    'message': 'Exposure time must be a number',
                    'error_code': 'INVALID_EXPOSURE'
                }), 400

            if not (Config.EXPOSURE_MIN <= exposure_time <= Config.EXPOSURE_MAX):
                return jsonify({
                    'status': 'error',
                    'message': f'Exposure time must be between {Config.EXPOSURE_MIN} and {Config.EXPOSURE_MAX} ms',
                    'error_code': 'EXPOSURE_OUT_OF_RANGE'
                }), 400

        # Validate format
        if image_format not in Config.SUPPORTED_FORMATS:
            return jsonify({
                'status': 'error',
                'message': f'Format must be one of: {", ".join(Config.SUPPORTED_FORMATS)}',
                'error_code': 'INVALID_FORMAT'
            }), 400


        # Capture image
        image_bytes, metadata = camera.capture(
            exposure_time=exposure_time,
            format=image_format
        )

        # Prepare response with image data
        mime_type = f'image/{image_format}'
        response = send_file(
            BytesIO(image_bytes),
            mimetype=mime_type,
            as_attachment=False
        )

        # Add metadata headers
        response.headers['X-Camera-Timestamp'] = metadata['timestamp']
        response.headers['X-Camera-Exposure'] = str(metadata['exposure_time'])
        response.headers['X-Camera-Resolution'] = metadata['resolution']
        response.headers['X-Image-Size-Bytes'] = str(metadata['size_bytes'])

        logger.info(f"Image captured and sent: {metadata['size_bytes']} bytes")
        return response

    except Exception as e:
        logger.error(f"Capture error: {e}")
        return jsonify({
            'status': 'error',
            'message': str(e),
            'error_code': 'CAPTURE_FAILED'
        }), 500


@app.route('/status', methods=['GET'])
def status():
    """
    Get API and camera status.

    Response JSON:
        {
            "camera_connected": true,
            "camera_model": "SL-1510",
            "api_version": "1.0.0",
            "uptime_seconds": 3600
        }
    """
    try:
        uptime = (datetime.now() - API_START_TIME).total_seconds()

        return jsonify({
            'camera_connected': camera.is_connected(),
            'camera_model': 'SL-1510',
            'api_version': Config.API_VERSION,
            'uptime_seconds': int(uptime)
        })

    except Exception as e:
        logger.error(f"Status error: {e}")
        return jsonify({
            'status': 'error',
            'message': str(e)
        }), 500


@app.route('/settings', methods=['GET', 'POST'])
def settings():
    """
    Get or update camera settings.

    GET Response JSON:
        {
            "exposure_time": 100
        }

    POST Request JSON:
        {
            "exposure_time": 150  # Optional
        }

    POST Response JSON:
        {
            "status": "success",
            "settings": { ... }
        }
    """
    try:
        if request.method == 'GET':
            # Return current settings
            current_settings = camera.get_settings()
            return jsonify(current_settings)

        else:  # POST
            # Update settings
            data = request.get_json() or {}

            if not data:
                return jsonify({
                    'status': 'error',
                    'message': 'No settings provided',
                    'error_code': 'NO_SETTINGS'
                }), 400

            # Update settings
            updated_settings = camera.update_settings(**data)

            return jsonify({
                'status': 'success',
                'settings': updated_settings
            })

    except ValueError as e:
        logger.warning(f"Settings validation error: {e}")
        return jsonify({
            'status': 'error',
            'message': str(e),
            'error_code': 'INVALID_SETTINGS'
        }), 400

    except Exception as e:
        logger.error(f"Settings error: {e}")
        return jsonify({
            'status': 'error',
            'message': str(e)
        }), 500


@app.route('/info', methods=['GET'])
def info():
    """
    Get camera capabilities and specifications.

    Response JSON:
        {
            "capabilities": {
                "max_resolution": [1920, 1080],
                "supported_formats": ["tif"],
                "exposure_range": [10, 10000]
            },
            "specifications": {
                "sensor_type": "CMOS",
                "interface": "USB 2.0"
            }
        }
    """
    try:
        capabilities = camera.get_capabilities()
        return jsonify(capabilities)

    except Exception as e:
        logger.error(f"Info error: {e}")
        return jsonify({
            'status': 'error',
            'message': str(e)
        }), 500


@app.route('/', methods=['GET'])
def root():
    """
    Root endpoint - API information.
    """
    return jsonify({
        'name': 'Camera REST API',
        'version': Config.API_VERSION,
        'endpoints': {
            'POST /capture': 'Capture image from camera',
            'GET /status': 'Get camera and API status',
            'GET /settings': 'Get current camera settings',
            'POST /settings': 'Update camera settings',
            'GET /info': 'Get camera capabilities and specifications'
        }
    })


@app.errorhandler(404)
def not_found(error):
    """Handle 404 errors."""
    return jsonify({
        'status': 'error',
        'message': 'Endpoint not found',
        'error_code': 'NOT_FOUND'
    }), 404


@app.errorhandler(500)
def internal_error(error):
    """Handle 500 errors."""
    logger.error(f"Internal error: {error}")
    return jsonify({
        'status': 'error',
        'message': 'Internal server error',
        'error_code': 'INTERNAL_ERROR'
    }), 500


def main():
    """Main entry point."""
    logger.info("=" * 60)
    logger.info("Camera REST API Starting")
    logger.info(f"Version: {Config.API_VERSION}")
    logger.info(f"Host: {Config.HOST}:{Config.PORT}")
    logger.info(f"Debug: {Config.DEBUG}")
    logger.info("=" * 60)

    # Start Flask app
    app.run(
        host=Config.HOST,
        port=Config.PORT,
        debug=Config.DEBUG
    )


if __name__ == '__main__':
    main()
