/**
 * capture.h
 *
 * Thread-safe C wrapper for libxdtusb camera operations.
 * Provides simple blocking capture interface for Flask integration.
 */

#ifndef CAPTURE_H
#define CAPTURE_H

#include <stdint.h>
#include <stddef.h>

/**
 * Initialize the XDT USB capture device.
 * Opens device with 1 frame buffer for memory efficiency.
 *
 * Returns:
 *   0 on success
 *   -1 if no device found
 *   -2 if device open failed
 *   -3 if initialization failed
 */
int init_capture_device(void);

/**
 * Capture a single frame with specified exposure time.
 * Blocks until frame is received or timeout occurs.
 *
 * Args:
 *   exposure_ms: Exposure time in milliseconds (10-10000)
 *
 * Returns:
 *   0 on success
 *   -1 if device not initialized
 *   -2 if capture configuration failed
 *   -3 if timeout (exposure_time + 10 seconds)
 *   -4 if streaming start failed
 *   -5 if trigger failed
 *
 * Thread-safe: Uses mutex and condition variable for synchronization
 */
int capture_frame(uint32_t exposure_ms);

/**
 * Get captured frame data and metadata.
 * Must be called after successful capture_frame().
 *
 * Args:
 *   width: Pointer to store frame width
 *   height: Pointer to store frame height
 *   pixel_size: Pointer to store bytes per pixel (always 2 for uint16_t)
 *   data: Pointer to store frame data pointer (do not free!)
 *   size: Pointer to store total data size in bytes
 *
 * Note: Data pointer is valid until next capture_frame() call
 */
void get_frame_data(uint32_t* width, uint32_t* height, uint32_t* pixel_size,
                    uint8_t** data, size_t* size);

/**
 * Cleanup and release capture device resources.
 * Should be called on application exit.
 */
void cleanup_capture_device(void);

#endif /* CAPTURE_H */
