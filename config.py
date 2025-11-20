"""
Configuration settings for the Camera REST API.
"""

import os


class Config:
    """Application configuration."""

    # Flask settings
    HOST = '0.0.0.0'  # Listen on all interfaces (USB and WiFi)
    PORT = 5000
    DEBUG = os.getenv('FLASK_DEBUG', 'False').lower() == 'true'

    # Camera defaults
    DEFAULT_EXPOSURE = 100  # milliseconds
    DEFAULT_FORMAT = 'tif'

    # Validation ranges
    EXPOSURE_MIN = 10       # milliseconds
    EXPOSURE_MAX = 10000    # milliseconds (10 seconds)
    QUALITY_MIN = 1
    QUALITY_MAX = 100

    # Supported image formats
    SUPPORTED_FORMATS = ['tif']

    # Paths
    SETTINGS_FILE = 'config/camera_settings.json'

    # Logging
    LOG_LEVEL = os.getenv('LOG_LEVEL', 'INFO')
    LOG_FORMAT = '%(asctime)s - %(name)s - %(levelname)s - %(message)s'

    # API metadata
    API_VERSION = '1.0.0'

    # Camera placeholder settings
    PLACEHOLDER_RESOLUTION = (1920, 1080)
    PLACEHOLDER_SENSOR_TYPE = 'CMOS'
    PLACEHOLDER_INTERFACE = 'USB 2.0'
