/**
 * capture.h
 *
 * Wrapper for libxdtusb camera operations.
 */

#ifndef CAPTURE_H
#define CAPTURE_H

#include <stdint.h>
#include "libraries/xdtusb.h"

int open_device(xdtusb_device_t** handle);

void close_device(xdtusb_device_t* handle);

int get_frame_dims(xdtusb_device_t* handle, uint16_t* width, uint16_t* height);

int read_register(xdtusb_device_t* handle, uint16_t* adr, uint16_t* val);

int write_register(xdtusb_device_t* handle, uint16_t* adr, uint16_t val);

int capture_frame(
    xdtusb_device_t* handle,
    uint16_t exposure_ms,
    uint16_t* buffer,
    uint32_t length
);

#endif /* CAPTURE_H */
