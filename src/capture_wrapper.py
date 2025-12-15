from pathlib import Path
import logging

import ctypes

import numpy as np

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

def _get_path():
    # Find libcapture.so
    lib_path = Path(__file__).parent.parent / "libcapture.so"
    if not lib_path.exists():
        raise FileNotFoundError(
            f"libcapture.so not found. Please run 'make' to build it.")

    return lib_path

# Init libcapture library
_lib_path = _get_path()
logger.info(f"Loading C library from: {_lib_path}")

_libcapture = ctypes.CDLL(_lib_path)

# Define _libcapture function argtypes
_libcapture.open_device.argtypes = [
    ctypes.POINTER(ctypes.c_void_p)
]

_libcapture.close_device.argtypes = [
    ctypes.c_void_p
]

_libcapture.get_frame_dims.argtypes = [
    ctypes.c_void_p,
    ctypes.POINTER(ctypes.c_uint16),
    ctypes.POINTER(ctypes.c_uint16)
]

_libcapture.capture_frame.argtypes = [
    ctypes.c_void_p,
    ctypes.c_uint16,                    # Exposure time (ms)
    ctypes.POINTER(ctypes.c_uint16),    # Pointer to buffer
    ctypes.c_uint32                     # Length
]

class DeviceError(Exception):
    """Represents an error in the capture device"""

class Device:
    def __init__(self):
        # Device pointer
        self._handle = ctypes.c_void_p()

        err = _libcapture.open_device(ctypes.byref(self._handle))
        self._check_error(err)

        c_w = ctypes.c_uint16()
        c_h = ctypes.c_uint16()

        err = _libcapture.get_frame_dims(self._handle, ctypes.byref(c_w), 
                                         ctypes.byref(c_h))
        self._check_error(err)

        self.width = c_w.value
        self.height = c_h.value

        print(f"width: {self.width}, height: {self.height}")

    def capture_frame(self, exposure_ms: int):
        """
        Docstring for capture_frame
        
        :param self: Description
        """
        logger.debug(f"Capture_frame called: {exposure_ms}ms")
        # Check that self.width and self.height are valid
        
        # Define buffer
        print(type(self.width))
        print(type(self.height))
        frame_buffer = np.empty(
            shape=(self.width * self.height, 1)).astype(np.uint16)
        
        logger.debug('Buffer defined')
        
        # Get pointer to first element in buffer
        frame_buffer_ptr = frame_buffer.ctypes.data_as(
            ctypes.POINTER(ctypes.c_uint16))
        
        logger.debug('Buffer pointer found')
        
        # Call capture_frame function
        err = _libcapture.capture_frame(
            self._handle, 
            exposure_ms,
            frame_buffer_ptr,
            frame_buffer.size
        )
        self._check_error(err)

        logger.debug('Frame captured')

        return frame_buffer
    
    def close_device(self):
        _libcapture.close_device(self._handle)

    def _check_error(self, err: int):
        if err != 0:
            logger.error("Placeholder")
            raise DeviceError(err)
        