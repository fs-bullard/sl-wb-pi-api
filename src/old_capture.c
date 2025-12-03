/**
 * capture.c
 *
 * Thread-safe wrapper for libxdtusb camera operations.
 * Implements blocking capture with single frame buffer for Pi Zero 2W.
 */

#include "capture.h"
#include "../libraries/xdtusb.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>

/* Global device handle (single frame only) */
static xdtusb_device_t* g_device = NULL;
static int g_initialized = 0;

/* Frame data storage (pre-allocated buffer) */
static uint32_t g_frame_width = 0;
static uint32_t g_frame_height = 0;
static uint32_t g_pixel_size = 2; /* uint16_t pixels */
static uint8_t* g_frame_data = NULL;
static size_t g_frame_capacity = 0; /* Allocated buffer size */
static size_t g_frame_size = 0;    /* Actual frame size */

/* Synchronization primitives for blocking capture */
static pthread_mutex_t g_capture_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_capture_cond = PTHREAD_COND_INITIALIZER;
static int g_frame_ready = 0;

/**
 * Frame callback - called by libxdtusb when frame is received.
 * Simplified to match working example - minimal error checking, no allocation.
 */
static void frame_callback(xdtusb_device_t* pdev, xdtusb_framebuf_t* pfb, void* puserargs)
{
    (void)pdev;      /* Unused - we use global g_device */
    (void)puserargs; /* Unused - no user data needed */

    /* Get frame dimensions */
    xdtusb_frame_dimensions_t* pframeDims;
    XDTUSB_FramebufGetDimensions(pfb, &pframeDims);

    /* Get frame data pointer */
    xdtusb_pixel_t* pframeData;
    XDTUSB_FramebufGetData(pfb, &pframeData);

    pthread_mutex_lock(&g_capture_mutex);

    /* Store frame metadata */
    g_frame_width = pframeDims->width;
    g_frame_height = pframeDims->height;
    g_frame_size = g_frame_width * g_frame_height * sizeof(xdtusb_pixel_t);

    /* Verify buffer is large enough (should be pre-allocated) */
    if (g_frame_data != NULL && g_frame_size <= g_frame_capacity) {
        /* Copy frame data to pre-allocated buffer */
        memcpy(g_frame_data, pframeData, g_frame_size);

        /* Signal that frame is ready */
        g_frame_ready = 1;
        pthread_cond_signal(&g_capture_cond);
    } else {
        fprintf(stderr, "Frame buffer not allocated or too small (%zu > %zu)\n",
                g_frame_size, g_frame_capacity);
    }

    pthread_mutex_unlock(&g_capture_mutex);

    /* Commit frame buffer back to device */
    XDTUSB_FramebufCommit(pfb);
}

int init_capture_device(void)
{
    xdtusb_error_t err;

    if (g_initialized) {
        fprintf(stderr, "Device already initialized\n");
        return 0;
    }

    /* Initialize libxdtusb */
    err = XDTUSB_Init();
    if (err != XDTUSB_ERROR_SUCCESS) {
        fprintf(stderr, "XDTUSB_Init failed: %s\n", XDTUSB_UtilErrorString(err));
        return -3;
    }

    /* Poll for devices */
    uint8_t numDevices;
    xdtusb_device_t** devlist;
    err = XDTUSB_PollDevices(&numDevices, &devlist);
    if (err != XDTUSB_ERROR_SUCCESS) {
        fprintf(stderr, "XDTUSB_PollDevices failed: %s\n", XDTUSB_UtilErrorString(err));
        XDTUSB_Exit();
        return -3;
    }

    if (numDevices == 0) {
        fprintf(stderr, "No XDT devices found\n");
        XDTUSB_Exit();
        return -1;
    }

    /* Use first device found */
    g_device = devlist[0];

    /* Open device with 1 frame buffer (memory efficient) */
    err = XDTUSB_DeviceOpen(g_device, 1);
    if (err != XDTUSB_ERROR_SUCCESS) {
        fprintf(stderr, "XDTUSB_DeviceOpen failed: %s\n", XDTUSB_UtilErrorString(err));
        XDTUSB_Exit();
        return -2;
    }

    /* Get device info */
    xdtusb_device_info_t devinfo;
    err = XDTUSB_DeviceGetInfo(g_device, &devinfo);
    if (err == XDTUSB_ERROR_SUCCESS) {
        fprintf(stdout, "Device opened: %s (Serial: %s)\n",
                devinfo.platform_hwstr, devinfo.serialNumber);
        fprintf(stdout, "USB: Bus %d, Port %d, Address %d, Speed: %s\n",
                devinfo.usb_bus, devinfo.usb_prt, devinfo.usb_adr,
                XDTUSB_UtilUsbSpeedString(devinfo.usb_spd));
    }

    /* Get maximum sensor dimensions for buffer pre-allocation */
    uint32_t max_width, max_height;
    err = XDTUSB_DeviceGetMaxDimensions(g_device, &max_width, &max_height);
    if (err != XDTUSB_ERROR_SUCCESS) {
        fprintf(stderr, "XDTUSB_DeviceGetMaxDimensions failed: %s\n", XDTUSB_UtilErrorString(err));
        XDTUSB_DeviceClose(g_device);
        XDTUSB_Exit();
        return -6;
    }

    /* Pre-allocate frame buffer based on actual sensor dimensions */
    g_frame_capacity = max_width * max_height * sizeof(xdtusb_pixel_t);
    g_frame_data = (uint8_t*)malloc(g_frame_capacity);
    if (g_frame_data == NULL) {
        fprintf(stderr, "Error: Failed to allocate %zu bytes for frame buffer\n", g_frame_capacity);
        XDTUSB_DeviceClose(g_device);
        XDTUSB_Exit();
        return -7;
    }

    fprintf(stdout, "Pre-allocated frame buffer: %zu bytes (%ux%u max)\n",
            g_frame_capacity, max_width, max_height);

    g_initialized = 1;
    fprintf(stdout, "Capture device initialized successfully\n");
    return 0;
}

int capture_frame(uint32_t exposure_ms)
{
    xdtusb_error_t err;
    struct timespec timeout;
    int ret;

    if (!g_initialized || g_device == NULL) {
        fprintf(stderr, "Device not initialized\n");
        return -1;
    }

    /* Validate exposure time */
    if (exposure_ms < 10 || exposure_ms > 10000) {
        fprintf(stderr, "Exposure time must be between 10 and 10000 ms\n");
        return -2;
    }

    pthread_mutex_lock(&g_capture_mutex);
    g_frame_ready = 0;
    pthread_mutex_unlock(&g_capture_mutex);

    /* Configure for single frame sequence capture */
    /* Using sequence mode with 1 frame, triggered by software */
    uint32_t exposure_us = exposure_ms * 1000;

    err = XDTUSB_DeviceSetSequenceModeParameters(g_device,
        /*numFrames*/ 1,
        /*expTimeUs*/ exposure_us,
        /*numSkp*/ 0,
        /*skpTimeUs*/ 0);

    if (err != XDTUSB_ERROR_SUCCESS) {
        fprintf(stderr, "XDTUSB_DeviceSetSequenceModeParameters failed: %s\n",
                XDTUSB_UtilErrorString(err));
        return -2;
    }

    /* Set acquisition mode to software-triggered sequence */
    err = XDTUSB_DeviceSetAcquisitionMode(g_device, XDT_ACQ_MODE_SEQ);
    if (err != XDTUSB_ERROR_SUCCESS) {
        fprintf(stderr, "XDTUSB_DeviceSetAcquisitionMode failed: %s\n",
                XDTUSB_UtilErrorString(err));
        return -2;
    }

    /* Start streaming with frame callback */
    err = XDTUSB_DeviceStartStreaming(g_device, frame_callback, NULL);
    if (err != XDTUSB_ERROR_SUCCESS) {
        fprintf(stderr, "XDTUSB_DeviceStartStreaming failed: %s\n",
                XDTUSB_UtilErrorString(err));
        return -4;
    }

    /* Issue software trigger to start capture */
    err = XDTUSB_DeviceIssueSwTrigger(g_device);
    if (err != XDTUSB_ERROR_SUCCESS) {
        fprintf(stderr, "XDTUSB_DeviceIssueSwTrigger failed: %s\n",
                XDTUSB_UtilErrorString(err));
        XDTUSB_DeviceStopStreaming(g_device);
        return -5;
    }

    /* Wait for frame with timeout (exposure_time + 10 seconds) */
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += (exposure_ms / 1000) + 10;
    timeout.tv_nsec += (exposure_ms % 1000) * 1000000;
    if (timeout.tv_nsec >= 1000000000) {
        timeout.tv_sec += 1;
        timeout.tv_nsec -= 1000000000;
    }

    pthread_mutex_lock(&g_capture_mutex);
    while (!g_frame_ready) {
        ret = pthread_cond_timedwait(&g_capture_cond, &g_capture_mutex, &timeout);
        if (ret == ETIMEDOUT) {
            pthread_mutex_unlock(&g_capture_mutex);
            fprintf(stderr, "Capture timeout after %u ms\n", exposure_ms + 10000);
            XDTUSB_DeviceStopStreaming(g_device);
            return -3;
        }
    }
    pthread_mutex_unlock(&g_capture_mutex);

    /* Stop streaming */
    err = XDTUSB_DeviceStopStreaming(g_device);
    if (err != XDTUSB_ERROR_SUCCESS) {
        fprintf(stderr, "Warning: XDTUSB_DeviceStopStreaming failed: %s\n",
                XDTUSB_UtilErrorString(err));
    }

    fprintf(stdout, "Frame captured: %ux%u, %zu bytes, exposure: %u ms\n",
            g_frame_width, g_frame_height, g_frame_size, exposure_ms);

    return 0;
}

void get_frame_data(uint32_t* width, uint32_t* height, uint32_t* pixel_size,
                    uint8_t** data, size_t* size)
{
    pthread_mutex_lock(&g_capture_mutex);

    if (width) *width = g_frame_width;
    if (height) *height = g_frame_height;
    if (pixel_size) *pixel_size = g_pixel_size;
    if (data) *data = g_frame_data;
    if (size) *size = g_frame_size;

    pthread_mutex_unlock(&g_capture_mutex);
}

void cleanup_capture_device(void)
{
    if (!g_initialized) {
        return;
    }

    /* Stop streaming if still active */
    if (g_device) {
        XDTUSB_DeviceStopStreaming(g_device);
        XDTUSB_DeviceClose(g_device);
        g_device = NULL;
    }

    /* Free frame buffer */
    if (g_frame_data) {
        free(g_frame_data);
        g_frame_data = NULL;
    }

    /* Exit libxdtusb */
    XDTUSB_Exit();

    g_initialized = 0;
    fprintf(stdout, "Capture device cleaned up\n");
}
