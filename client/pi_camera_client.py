"""
Python client library for Camera REST API.

This module provides a simple synchronous client for interacting with the
Camera REST API running on Pi Zero 2W. Designed for easy integration with
PySide6 desktop applications.
"""

import requests
from typing import Dict, Optional, Tuple


class PiCameraClient:
    """
    Client for Camera REST API.

    Provides simple synchronous methods for all API endpoints. For use in
    PySide6 apps, call these methods from QThread workers to avoid blocking
    the UI thread.
    """

    def __init__(
        self,
        host: str = '192.168.7.2',
        port: int = 5000,
        timeout: int = 30
    ):
        """
        Initialize camera client.

        Args:
            host: Pi Zero IP address or hostname
                  - USB connection: '192.168.7.2'
                  - WiFi: Pi's WiFi IP or 'raspberrypi.local'
            port: API port (default: 5000)
            timeout: Request timeout in seconds (default: 30)
        """
        self.base_url = f'http://{host}:{port}'
        self.timeout = timeout

    def capture_image(
        self,
        exposure_time: Optional[int] = None,
        format: str = 'tif',
        quality: int = 85
    ) -> Tuple[bytes, Dict]:
        """
        Capture an image from the camera.

        Args:
            exposure_time: Exposure time in milliseconds (10-10000)
                          None uses camera's current setting
            format: Image format ('jpeg' or 'png')
            quality: JPEG quality (1-100, ignored for PNG)

        Returns:
            Tuple of (image_bytes, metadata_dict)
            metadata_dict contains:
                - timestamp: ISO timestamp string
                - exposure: Exposure time in ms
                - resolution: String like "1920x1080"
                - size_bytes: Image size in bytes

        Raises:
            requests.exceptions.RequestException: On network/HTTP errors
            Exception: On API errors (with error message from server)
        """
        payload = {
            'format': format,
            'quality': quality
        }

        if exposure_time is not None:
            payload['exposure_time'] = exposure_time

        response = requests.post(
            f'{self.base_url}/capture',
            json=payload,
            timeout=self.timeout
        )

        if response.status_code == 200:
            # Extract metadata from headers
            metadata = {
                'timestamp': response.headers.get('X-Camera-Timestamp'),
                'exposure': int(response.headers.get('X-Camera-Exposure', 0)),
                'resolution': response.headers.get('X-Camera-Resolution'),
                'size_bytes': int(response.headers.get('X-Image-Size-Bytes', 0))
            }

            return response.content, metadata

        else:
            # Error response (JSON)
            try:
                error = response.json()
                error_msg = error.get('message', 'Unknown error')
                error_code = error.get('error_code', 'UNKNOWN')
                raise Exception(f"Capture failed [{error_code}]: {error_msg}")
            except ValueError:
                # Response is not JSON
                raise Exception(f"Capture failed: HTTP {response.status_code}")

    def get_status(self) -> Dict:
        """
        Get camera and API status.

        Returns:
            Dictionary with:
                - camera_connected: bool
                - camera_model: str
                - api_version: str
                - uptime_seconds: int

        Raises:
            requests.exceptions.RequestException: On network/HTTP errors
        """
        response = requests.get(
            f'{self.base_url}/status',
            timeout=self.timeout
        )
        response.raise_for_status()
        return response.json()

    def get_settings(self) -> Dict:
        """
        Get current camera settings.

        Returns:
            Dictionary of current settings (exposure_time, gain, etc.)

        Raises:
            requests.exceptions.RequestException: On network/HTTP errors
        """
        response = requests.get(
            f'{self.base_url}/settings',
            timeout=self.timeout
        )
        response.raise_for_status()
        return response.json()

    def update_settings(self, **kwargs) -> Dict:
        """
        Update camera settings.

        Args:
            **kwargs: Settings to update
                - exposure_time: int (10-10000 ms)
                - gain: float
                - brightness: int
                - contrast: int

        Returns:
            Dictionary with 'status' and 'settings' keys

        Raises:
            requests.exceptions.RequestException: On network/HTTP errors
            Exception: On invalid settings
        """
        response = requests.post(
            f'{self.base_url}/settings',
            json=kwargs,
            timeout=self.timeout
        )

        if response.status_code == 200:
            return response.json()
        else:
            try:
                error = response.json()
                error_msg = error.get('message', 'Unknown error')
                raise Exception(f"Update settings failed: {error_msg}")
            except ValueError:
                raise Exception(f"Update settings failed: HTTP {response.status_code}")

    def get_info(self) -> Dict:
        """
        Get camera capabilities and specifications.

        Returns:
            Dictionary with 'capabilities' and 'specifications' keys

        Raises:
            requests.exceptions.RequestException: On network/HTTP errors
        """
        response = requests.get(
            f'{self.base_url}/info',
            timeout=self.timeout
        )
        response.raise_for_status()
        return response.json()

    def test_connection(self) -> bool:
        """
        Test connection to the API.

        Returns:
            True if connection successful, False otherwise
        """
        try:
            response = requests.get(
                f'{self.base_url}/',
                timeout=5
            )
            return response.status_code == 200
        except:
            return False


# Example usage
if __name__ == '__main__':
    """
    Example usage of PiCameraClient.
    """
    import sys

    # Initialize client (USB connection)
    print("Connecting to Pi Zero via USB...")
    client = PiCameraClient(host='192.168.7.2')

    # Test connection
    if not client.test_connection():
        print("ERROR: Cannot connect to camera API")
        print("Make sure:")
        print("  1. Pi Zero is powered on")
        print("  2. USB connection is established")
        print("  3. API service is running on Pi")
        sys.exit(1)

    print("Connected successfully!\n")

    # Get status
    print("Getting status...")
    status = client.get_status()
    print(f"  Camera connected: {status['camera_connected']}")
    print(f"  API version: {status['api_version']}")
    print(f"  Uptime: {status['uptime_seconds']}s\n")

    # Get camera info
    print("Getting camera info...")
    info = client.get_info()
    print(f"  Sensor: {info['specifications']['sensor_type']}")
    print(f"  Max resolution: {info['capabilities']['max_resolution']}")
    print(f"  Exposure range: {info['capabilities']['exposure_range']} ms\n")

    # Get current settings
    print("Getting current settings...")
    settings = client.get_settings()
    print(f"  Exposure: {settings['exposure_time']} ms")
    print(f"  Gain: {settings.get('gain', 'N/A')}\n")

    # Capture image
    print("Capturing image...")
    try:
        image_bytes, metadata = client.capture_image(
            exposure_time=150,
            format='jpeg',
            quality=90
        )

        print(f"  Image captured!")
        print(f"  Timestamp: {metadata['timestamp']}")
        print(f"  Resolution: {metadata['resolution']}")
        print(f"  Size: {metadata['size_bytes']} bytes")

        # Save image
        filename = 'test_capture.jpg'
        with open(filename, 'wb') as f:
            f.write(image_bytes)
        print(f"  Saved to {filename}\n")

    except Exception as e:
        print(f"  ERROR: {e}\n")

    # Update settings
    print("Updating settings...")
    try:
        result = client.update_settings(exposure_time=200)
        print(f"  Settings updated!")
        print(f"  New exposure: {result['settings']['exposure_time']} ms\n")
    except Exception as e:
        print(f"  ERROR: {e}\n")

    print("Example complete!")
