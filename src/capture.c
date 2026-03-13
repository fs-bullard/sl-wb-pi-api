#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <pthread.h>

#include "capture.h"

#define LOG_INFO(fmt, ...)  fprintf(stderr, "[capture] INFO  " fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) fprintf(stderr, "[capture] ERROR " fmt "\n", ##__VA_ARGS__)

#define WIDTH_ADDRESS 69
#define HEIGHT_ADDRESS 68

typedef struct callback_data{
    uint16_t* buffer;
    uint32_t length;
    pthread_mutex_t* mutex;
    pthread_cond_t* cond;
} callback_data;

static void callback(
    xdtusb_device_t* handle,
    xdtusb_framebuf_t* buffer,
    callback_data* callback_data
);

int open_device(xdtusb_device_t** handle){
    // Initialize libxdtusb
	XDTUSB_Init();

	// Poll devices
	uint8_t num_devices;
	xdtusb_device_t** device_list;
	XDTUSB_PollDevices(&num_devices, &device_list);

	// If XDTUSB devices were found
	if (num_devices == 0)
    {
        LOG_ERROR("no XDTUSB devices found");
        return 1;
    }
    LOG_INFO("found %d device(s), using first", num_devices);

    // Use first device found
    *handle = device_list[0];

    // Open device
    xdtusb_error_t err = XDTUSB_DeviceOpen(*handle, 1);

    if (err != XDTUSB_ERROR_SUCCESS) {
        LOG_ERROR("XDTUSB_DeviceOpen failed (err=%d)", err);
        return 1;
    }

    LOG_INFO("device opened successfully");
    return 0;
}

void close_device(xdtusb_device_t* handle){
    XDTUSB_DeviceClose(handle);

    XDTUSB_Exit();
}

int read_register(xdtusb_device_t* handle, uint16_t adr, uint16_t* val){
    xdtusb_error_t err = XDTUSB_DeviceFpgaRegisterRead(handle, adr, val);
    if (err != XDTUSB_ERROR_SUCCESS) {
        return 1;
    }

    return 0;
}

int write_register(xdtusb_device_t* handle, uint16_t adr, uint16_t val){
    xdtusb_error_t err = XDTUSB_DeviceFpgaRegisterWrite(handle, adr, val);
    if (err != XDTUSB_ERROR_SUCCESS) {
            return 1;
        }

        return 0;
}

int get_frame_dims(xdtusb_device_t* handle, uint16_t* width, uint16_t* height){
    xdtusb_error_t err = XDTUSB_DeviceFpgaRegisterRead(handle, WIDTH_ADDRESS, width);
    if (err != XDTUSB_ERROR_SUCCESS) {
        return 1;
    }

    err = XDTUSB_DeviceFpgaRegisterRead(handle, HEIGHT_ADDRESS, height);
    if (err != XDTUSB_ERROR_SUCCESS) {
        return 1;
    }

    return 0;
}

int capture_frame(
    xdtusb_device_t* handle,
    uint16_t exposure_ms,
    uint16_t* buffer,
    uint32_t length
){
    LOG_INFO("capturing frame: exposure=%u ms, buffer_length=%u", exposure_ms, length);

    xdtusb_error_t err = XDTUSB_DeviceSetAcquisitionMode(handle, XDT_ACQ_MODE_SEQ);
    if (err != XDTUSB_ERROR_SUCCESS) {
        LOG_ERROR("XDTUSB_DeviceSetAcquisitionMode failed (err=%d)", err);
        return 1;
    }
    LOG_INFO("Set acquisition mode");

    err = XDTUSB_DeviceSetSequenceModeParameters(
        handle,
        1,
        (uint32_t)exposure_ms * 1000,
        0,
        0
    );
    if (err != XDTUSB_ERROR_SUCCESS) {
        LOG_ERROR("XDTUSB_DeviceSetSequenceModeParameters failed (err=%d)", err);
        return 1;
    }
    LOG_INFO("Set Sequence mode parameters");

    pthread_mutex_t mutex;
    pthread_cond_t cond;

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

    callback_data callback_data;
    callback_data.buffer = buffer;
    callback_data.length = length;
    callback_data.mutex = &mutex;
    callback_data.cond = &cond;

    // Start streaming
    err = XDTUSB_DeviceStartStreaming(handle, (xdtusb_frame_cb_t)callback, &callback_data);
	if (err != XDTUSB_ERROR_SUCCESS)
	{
        LOG_ERROR("XDTUSB_DeviceStartStreaming failed (err=%d)", err);
        pthread_mutex_destroy(&mutex);
        pthread_cond_destroy(&cond);
		return 1;
	}
    LOG_INFO("Started streaming");

    // Issue SW trigger
    err = XDTUSB_DeviceIssueSwTrigger(handle);
    if (err != XDTUSB_ERROR_SUCCESS) {
        LOG_ERROR("XDTUSB_DeviceIssueSwTrigger failed (err=%d)", err);
        XDTUSB_DeviceStopStreaming(handle);
        pthread_mutex_destroy(&mutex);
        pthread_cond_destroy(&cond);
        return 1;
    }
    LOG_INFO("SW trigger issued, waiting for frame");

    // Build absolute timeout: exposure time + 5s headroom
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    long timeout_ms = (long)exposure_ms + 5000;
    timeout.tv_sec  += timeout_ms / 1000;
    timeout.tv_nsec += (timeout_ms % 1000) * 1000000L;
    if (timeout.tv_nsec >= 1000000000L) {
        timeout.tv_sec++;
        timeout.tv_nsec -= 1000000000L;
    }

    pthread_mutex_lock(&mutex);
    int wait_result = pthread_cond_timedwait(&cond, &mutex, &timeout);
    pthread_mutex_unlock(&mutex);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    XDTUSB_DeviceStopStreaming(handle);

    if (wait_result == ETIMEDOUT) {
        LOG_ERROR("timed out waiting for frame (timeout=%ld ms)", timeout_ms);
        return 1;
    }

    LOG_INFO("frame captured successfully");
    return 0;
}   

void callback(
    xdtusb_device_t* handle,
    xdtusb_framebuf_t* frame_buffer,
    callback_data* callback_data
){
    xdtusb_frame_dimensions_t* frame_dimensions;
	xdtusb_error_t err = XDTUSB_FramebufGetDimensions(frame_buffer, &frame_dimensions);
    
    if (err == XDTUSB_ERROR_SUCCESS) {
        LOG_INFO("frame dims: %ux%u", frame_dimensions->width, frame_dimensions->height);
        xdtusb_pixel_t* temp_frame_data;
        err = XDTUSB_FramebufGetData(frame_buffer, &temp_frame_data);

        if (err == XDTUSB_ERROR_SUCCESS) {
            uint32_t frame_pixels = frame_dimensions->width * frame_dimensions->height;
            if (callback_data->length >= frame_pixels) {
                pthread_mutex_lock(callback_data->mutex);
                memcpy(callback_data->buffer, temp_frame_data, callback_data->length * sizeof(uint16_t));
                pthread_mutex_unlock(callback_data->mutex);
            } else {
                LOG_ERROR("buffer too small: need %u pixels, have %u", frame_pixels, callback_data->length);
            }
        } else {
            LOG_ERROR("XDTUSB_FramebufGetData failed (err=%d)", err);
        }
    } else {
        LOG_ERROR("XDTUSB_FramebufGetDimensions failed (err=%d)", err);
    }

    pthread_cond_broadcast(callback_data->cond);

    XDTUSB_FramebufCommit(frame_buffer);
}
