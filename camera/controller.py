"""
Camera controller with placeholder implementation for custom USB camera sensor.

This module provides an abstraction layer for camera operations. The current
implementation uses placeholder/dummy functionality that can be easily replaced
with actual camera SDK integration.
"""

import json
import logging
from pathlib import Path
from datetime import datetime
from io import BytesIO
from PIL import Image, ImageDraw, ImageFont

from config import Config


logger = logging.getLogger(__name__)


class CameraController:
    """
    Controller for camera operations with persistent settings.

    This is a placeholder implementation that generates dummy images.
    Replace the capture() method with actual camera SDK calls when ready.
    """

    def __init__(self, settings_file: str = Config.SETTINGS_FILE):
        """
        Initialize camera controller.

        Args:
            settings_file: Path to JSON file for persistent settings
        """
        self.settings_file = Path(settings_file)
        self.settings = self._load_settings()
        self._connected = True  # Placeholder: always connected
        logger.info("Camera controller initialized")

    def _load_settings(self) -> dict:
        """
        Load camera settings from JSON file.

        Returns:
            Dictionary of camera settings
        """
        # Create config directory if it doesn't exist
        self.settings_file.parent.mkdir(parents=True, exist_ok=True)

        if self.settings_file.exists():
            try:
                with open(self.settings_file, 'r') as f:
                    settings = json.load(f)
                logger.info(f"Loaded settings from {self.settings_file}")
                return settings
            except Exception as e:
                logger.error(f"Error loading settings: {e}")
                return self._default_settings()
        else:
            # Create default settings file
            settings = self._default_settings()
            self._save_settings(settings)
            logger.info(f"Created default settings at {self.settings_file}")
            return settings

    def _default_settings(self) -> dict:
        """
        Get default camera settings.

        Returns:
            Dictionary of default settings
        """
        return {
            "exposure_time": Config.DEFAULT_EXPOSURE
        }

    def _save_settings(self, settings: dict | None = None):
        """
        Save camera settings to JSON file atomically.

        Args:
            settings: Settings dictionary to save (uses self.settings if None)
        """
        if settings is None:
            settings = self.settings

        # Write to temporary file first, then rename
        temp_file = self.settings_file.with_suffix('.tmp')
        try:
            with open(temp_file, 'w') as f:
                json.dump(settings, f, indent=2)
            temp_file.replace(self.settings_file)
            logger.info(f"Settings saved to {self.settings_file}")
        except Exception as e:
            logger.error(f"Error saving settings: {e}")
            if temp_file.exists():
                temp_file.unlink()
            raise

    def capture(
        self,
        exposure_time: int | None = None,
        format: str = Config.DEFAULT_FORMAT,
    ) -> tuple[bytes, dict]:
        """
        Capture an image from the camera.

        PLACEHOLDER IMPLEMENTATION: Generates a dummy image with test pattern.
        Replace this method with actual camera SDK calls.

        Args:
            exposure_time: Exposure time in milliseconds (None uses current setting)
            format: Image format ('tif')

        Returns:
            Tuple of (image_bytes, metadata_dict)

        Raises:
            Exception: If camera is not connected or capture fails
        """
        if not self._connected:
            raise Exception("Camera not connected")

        # Use provided exposure or fall back to saved setting
        exp_time = exposure_time if exposure_time is not None else self.settings['exposure_time']

        logger.info(f"Capturing image: exposure={exp_time}ms, format={format}")

        # PLACEHOLDER: Generate a test pattern image
        # TODO: Replace with actual camera SDK capture call
        width, height = Config.PLACEHOLDER_RESOLUTION
        image = Image.new('L', (width, height), color=30)

        # Draw test pattern
        draw = ImageDraw.Draw(image)

        # Draw gradient bars
        for i in range(0, width, 100):
            draw.rectangle([i, 0, i + 100, height], fill=i % 256)

        # Draw grid
        for x in range(0, width, 100):
            draw.line([(x, 0), (x, height)], fill=255, width=1)
        for y in range(0, height, 100):
            draw.line([(0, y), (width, y)], fill=255, width=1)

        # Add text overlay with capture info
        timestamp = datetime.now().isoformat()
        text_lines = [
            f"PLACEHOLDER IMAGE",
            f"Time: {timestamp}",
            f"Exposure: {exp_time}ms",
            f"Resolution: {width}x{height}"
        ]

        font = ImageFont.load_default()

        y_offset = 50
        for line in text_lines:
            draw.text((50, y_offset), line, fill=(255, 255, 255), font=font)
            y_offset += 40

        # Convert to bytes
        buffer = BytesIO()
        if format.lower() == 'tif':
            image.save(buffer, format='tif')
        else:
            logger.error(f"Unsupported format '{format}'")
            return

        image_bytes = buffer.getvalue()

        # Prepare metadata
        metadata = {
            'timestamp': timestamp,
            'exposure_time': exp_time,
            'resolution': f"{width}x{height}",
            'size_bytes': len(image_bytes),
            'format': format
        }

        logger.info(f"Image captured: {len(image_bytes)} bytes")
        return image_bytes, metadata

    def get_settings(self) -> dict:
        """
        Get current camera settings.

        Returns:
            Dictionary of current settings
        """
        return self.settings.copy()

    def update_settings(self, **kwargs) -> dict:
        """
        Update camera settings and persist to file.

        Args:
            **kwargs: Settings to update (exposure_time, gain, brightness, contrast)

        Returns:
            Dictionary of updated settings

        Raises:
            ValueError: If invalid settings are provided
        """
        # Validate exposure time if provided
        if 'exposure_time' in kwargs:
            exp = kwargs['exposure_time']
            if not (Config.EXPOSURE_MIN <= exp <= Config.EXPOSURE_MAX):
                raise ValueError(
                    f"Exposure time must be between {Config.EXPOSURE_MIN} "
                    f"and {Config.EXPOSURE_MAX} ms"
                )

        # Update settings
        for key, value in kwargs.items():
            if key in self.settings:
                self.settings[key] = value
                logger.info(f"Updated setting: {key} = {value}")
            else:
                logger.warning(f"Unknown setting ignored: {key}")

        # Persist to file
        self._save_settings()

        return self.settings.copy()

    def get_capabilities(self) -> dict:
        """
        Get camera capabilities and specifications.

        Returns:
            Dictionary of camera capabilities
        """
        return {
            'capabilities': {
                'max_resolution': list(Config.PLACEHOLDER_RESOLUTION),
                'supported_formats': Config.SUPPORTED_FORMATS.copy(),
                'exposure_range': [Config.EXPOSURE_MIN, Config.EXPOSURE_MAX],
            },
            'specifications': {
                'sensor_type': Config.PLACEHOLDER_SENSOR_TYPE,
                'interface': Config.PLACEHOLDER_INTERFACE
            }
        }

    def is_connected(self) -> bool:
        """
        Check if camera is connected.

        Returns:
            True if connected, False otherwise
        """
        # PLACEHOLDER: Always returns True
        # TODO: Implement actual camera connection check
        return self._connected

    def disconnect(self):
        """
        Disconnect from camera and cleanup resources.
        """
        self._connected = False
        logger.info("Camera disconnected")

    def __enter__(self):
        """Context manager entry."""
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit."""
        self.disconnect()
