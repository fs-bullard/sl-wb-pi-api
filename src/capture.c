#include <string.h>

#include <pthread.h>

#include "capture.h"

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
        return 1;
    }
	
    // Use first device found
    *handle = device_list[0];

    // Open device
    xdtusb_error_t err = XDTUSB_DeviceOpen(*handle, 1);

    return err != XDTUSB_ERROR_SUCCESS;
}

void close_device(xdtusb_device_t* handle){
    XDTUSB_DeviceClose(handle);

    XDTUSB_Exit();
}

int read_register(xdtusb_device_t* handle, uint_16_t adr, uint16_t* val){
    xdtusb_error_t err = XDTUSB_DeviceFpgaRegisterRead(handle, adr, val);
    if (err != XDTUSB_ERROR_SUCCESS) {
        return 1;
    }

    return 0;
}

int write_register(xdtusb_device_t* handle, uint_16_t adr, uint16_t val){
    xdtusb_error_t err = XDTUSB_DeviceFpgaRegisterWrite(handle, adr, val)
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
    xdtusb_error_t err = XDTUSB_DeviceSetAcquisitionMode(handle, XDT_ACQ_MODE_SEQ);
    if (err != XDTUSB_ERROR_SUCCESS) {
        return 1;
    }

    err = XDTUSB_DeviceSetSequenceModeParameters(
        handle,
        1,
        (uint32_t)exposure_ms * 1000,
        0,
        0
    );
    if (err != XDTUSB_ERROR_SUCCESS) {
        return 1;
    }
    
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
		return 1;
	}
    
    // Issue SW trigger
    err = XDTUSB_DeviceIssueSwTrigger(handle);
    if (err != XDTUSB_ERROR_SUCCESS) {
        return 1;
    }

    // Wait for frame TODO: add timeout (max wait time)
    pthread_mutex_lock(&mutex);
    pthread_cond_wait(&cond, &mutex);
    pthread_mutex_unlock(&mutex);
    
    // Stop streaming
    err = XDTUSB_DeviceStopStreaming(handle);
    if (err != XDTUSB_ERROR_SUCCESS) {
        return 1;
    }

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
        xdtusb_pixel_t* temp_frame_data;
        err = XDTUSB_FramebufGetData(frame_buffer, &temp_frame_data);

        if (err == XDTUSB_ERROR_SUCCESS) {
            if (callback_data->length >= frame_dimensions->width * frame_dimensions->height) {
                pthread_mutex_lock(callback_data->mutex);
                memcpy(callback_data->buffer, temp_frame_data, callback_data->length * sizeof(uint16_t));
                pthread_mutex_unlock(callback_data->mutex);
            }
        }
    }

    pthread_cond_broadcast(callback_data->cond);

    XDTUSB_FramebufCommit(frame_buffer);
}
