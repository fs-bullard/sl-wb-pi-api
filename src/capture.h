/**
 * capture.h
 *
 * Wrapper for libxdtusb camera operations.
 */

#ifndef CAPTURE_H
#define CAPTURE_H

#include <stdint.h>
#include "libraries/xdtusb.h"

/**
 * Initialize the XDT USB capture device.
 * Initializes libxdtusb, polls for devices, opens the first device found,
 * and sets acquisition mode.
 *
 * Args:
 *   ppdev: Pointer to device pointer that will be set to opened device
 *
 * Returns:
 *   0 on success
 *   1 on failure (no device found or open failed)
 */
int init_device(xdtusb_device_t** ppdev);

/**
 * Configure capture settings for the device.
 *
 * Args:
 *   pdev: Pointer to opened device
 *   exposure_us: Exposure time in microseconds
 *
 * Returns:
 *   0 on success
 *   1 on failure
 */
int set_capture_settings(xdtusb_device_t* pdev, uint32_t exposure_us);

/**
 * Capture a single frame.
 * Starts streaming, issues software trigger, waits for frame callback,
 * and stops streaming.
 *
 * Args:
 *   pdev: Pointer to opened device
 *
 * Returns:
 *   0 on success
 *   1 on failure (timeout or streaming error)
 */
int capture_frame(xdtusb_device_t* pdev);

/**
 * Get captured frame data and metadata.
 * Must be called after successful capture_frame().
 *
 * Args:
 *   data: Pointer to store frame data pointer (caller should not free)
 *   size: Pointer to store total data size in bytes
 *   width: Pointer to store frame width
 *   height: Pointer to store frame height
 *
 * Returns:
 *   0 on success
 *   1 if no frame available
 *
 * Note: Data pointer is valid until next capture_frame() or clear_frame_data() call
 */
int get_frame_data(uint8_t** data, uint32_t* size, uint32_t* width, uint32_t* height);

/**
 * Free frame buffer and clear frame data.
 */
void clear_frame_data(void);

/**
 * Cleanup and close capture device.
 *
 * Args:
 *   pdev: Pointer to opened device
 *
 * Returns:
 *   0 on success
 *   1 on failure
 */
int cleanup_capture_device(xdtusb_device_t* pdev);

#endif /* CAPTURE_H */
