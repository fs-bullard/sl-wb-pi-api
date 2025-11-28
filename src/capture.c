/** \file
 *
 * capture.c.
 *
 * Wrapper for libxdtusb camera operations.
 *
 */


/*----------------------------------------------------------------------------*\
	Headers
\*----------------------------------------------------------------------------*/
#include <time.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "libraries/xdtusb.h"

/*----------------------------------------------------------------------------*\
	Constants
\*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*\
	Macros
\*----------------------------------------------------------------------------*/
#define NUM_FRAME_BUFS 1

/*----------------------------------------------------------------------------*\
	Types
\*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*\
	Local Prototypes
\*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*\
	Local Variables
\*----------------------------------------------------------------------------*/
// Structure to hold captured frame data
typedef struct {
    uint8_t* data;           // Raw image bytes
    uint32_t size;           // Size in bytes
    uint32_t width;
    uint32_t height;
    volatile bool ready;     // Flag indicating data is ready
} frame_buffer_t;

static frame_buffer_t g_frame_buffer = {NULL, 0, 0, 0, false};
/*----------------------------------------------------------------------------*\
	API Variables
\*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*\
	API Functions
\*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*\
	Local Functions
\*----------------------------------------------------------------------------*/

// Frame received callback
void frame_cb(xdtusb_device_t* pdev, xdtusb_framebuf_t* pfb, void* puserargs)
{
	// Extract image information and data
	// Get actual image dimensions
	xdtusb_frame_dimensions_t* pframeDims;
	XDTUSB_FramebufGetDimensions(pfb, &pframeDims);

	// Get frames image data
	xdtusb_pixel_t* pframeData;
	XDTUSB_FramebufGetData(pfb, &pframeData);

	// For simplicity just output some frame information
	printf("\nFrame received (h=%4u, w=%4u)\n", pframeDims->height, pframeDims->width);
	printf("-------------------------------\n");

	// Trace out the first 16 pixels of the frame here
	const uint32_t traceCount = 16;
	for (uint32_t i = 0; i < (traceCount+(8-1))/8; i++) {
		printf ("%04x: %04x %04x %04x %04x %04x %04x %04x %04x\n", i*16, pframeData[0], pframeData[1], pframeData[2], pframeData[3], pframeData[4], pframeData[5], pframeData[6], pframeData[7]);
		pframeData += 8;
	}
	
	// Calculate frame size (assuming xdtusb_pixel_t is 2 bytes - uint16_t)
    uint32_t pixel_count = pframeDims->width * pframeDims->height;
    uint32_t frame_size = pixel_count * sizeof(xdtusb_pixel_t);

    // Free old buffer if exists
    if (g_frame_buffer.data != NULL) {
        free(g_frame_buffer.data);
    }

    // Allocate new buffer and copy data
    g_frame_buffer.data = (uint8_t*)malloc(frame_size);
    if (g_frame_buffer.data != NULL) {
        memcpy(g_frame_buffer.data, pframeData, frame_size);
        g_frame_buffer.size = frame_size;
        g_frame_buffer.width = pframeDims->width;
        g_frame_buffer.height = pframeDims->height;
        g_frame_buffer.ready = true;
        printf("Frame data stored: %u bytes\n", frame_size);
    } else {
        fprintf(stderr, "Failed to allocate frame buffer\n");
    }
}


int init_device(xdtusb_device_t** ppdev)
{
	// Initialize libxdtusb
	XDTUSB_Init();
	
	// Poll devices
	uint8_t numDevices;
	xdtusb_device_t** devlist;
	XDTUSB_PollDevices(&numDevices, &devlist);

	// If XDTUSB devices were found
	if (numDevices >= 1)
	{

		// Use first device found
		*ppdev = devlist[0];

		// Open device
		xdtusb_error_t err;
		err = XDTUSB_DeviceOpen(*ppdev, 1);
		if (err == XDTUSB_ERROR_SUCCESS)
		{
			// Get some firmware information
			xdtusb_fwinfo_t afp_fwinfo;
			XDTUSB_DeviceGetAfpInfo(*ppdev, &afp_fwinfo);
			printf("AFP rev=0x%04X, build time=%02u-%02u-%04u %02u:%02u\n", afp_fwinfo.rev, afp_fwinfo.day, afp_fwinfo.month, afp_fwinfo.year, afp_fwinfo.hour, afp_fwinfo.minute);

			// Set Acquisition Mode
			XDTUSB_DeviceSetAcquisitionMode(*ppdev, XDT_ACQ_MODE_SEQ);

			return 0;
		}
		else
		{
			fprintf(stderr, "init_device failed: %s\n", XDTUSB_UtilErrorString(err));
			return 1;
		}
	}
	else
	{
		return 1;
	}

}

int set_capture_settings(xdtusb_device_t* pdev, uint32_t exposure_us)
{
	// Configure Sequence
	xdtusb_error_t err;
	err = XDTUSB_DeviceSetSequenceModeParameters(
		pdev, 
		/*numFrames*/ 1, 
		/*expTimeUs*/ exposure_us, 
		/*numDummy*/ 0, 
		/*expTimeDummy*/ 0
	);
	if (err != XDTUSB_ERROR_SUCCESS)
	{
		fprintf(stderr, "set_capture_settings failed: %s\n", XDTUSB_UtilErrorString(err));
		return 1;
	}
	else
	{
		printf("Set capture settings successfully \n");
		return 0;
	}
}

int capture_frame(xdtusb_device_t* pdev)
{
	xdtusb_error_t err;

	// Clear previous frame
	g_frame_buffer.ready = false;

	// Start Streaming mode
	err = XDTUSB_DeviceStartStreaming(pdev, frame_cb, NULL);
	if (err != XDTUSB_ERROR_SUCCESS)
	{
		fprintf(stderr, "DeviceStartStreaming failed: %s\n", XDTUSB_UtilErrorString(err));
		return 1;
	}

	// Issue SW trigger
	err = XDTUSB_DeviceIssueSwTrigger(pdev);
	if (err != XDTUSB_ERROR_SUCCESS)
	{
		fprintf(stderr, "SoftwareTrigger failed: %s\n", XDTUSB_UtilErrorString(err));
		return 1;
	}

	// Wait for frame with timeout
	int timeout_ms = 500;
	while (!g_frame_buffer.ready && timeout_ms > 0) {
		nanosleep(&(struct timespec){0, 10000000}, NULL);  // 10ms
		timeout_ms -= 10;
	}

	if (!g_frame_buffer.ready) {
		fprintf(stderr, "Timeout waiting for frame\n");
		XDTUSB_DeviceStopStreaming(pdev);
		return 1;
	}

	// Stop Streaming mode
	err = XDTUSB_DeviceStopStreaming(pdev);
	if (err != XDTUSB_ERROR_SUCCESS)
	{
		fprintf(stderr, "StopStreaming failed: %s\n", XDTUSB_UtilErrorString(err));
		return 1;
	}

	printf("Captured frame successfully\n");
	return 0;
}

int cleanup_capture_device(xdtusb_device_t* pdev)
{
	// Close device
	xdtusb_error_t err;
	err = XDTUSB_DeviceClose(pdev);
	if (err != XDTUSB_ERROR_SUCCESS)
	{
		fprintf(stderr, "DeviceClose failed: %s\n", XDTUSB_UtilErrorString(err));
		return 1;
	}
	else
	{
		printf("Cleaned up device successfully \n");
		return 0;
	}
}

int get_frame_data(uint8_t** data, uint32_t* size, uint32_t* width, uint32_t* height)
{
	printf("Getting frame data \n");

	if (!g_frame_buffer.ready || g_frame_buffer.data == NULL) {
        return 1;  // No frame available
    }

    *data = g_frame_buffer.data;
    *size = g_frame_buffer.size;
    *width = g_frame_buffer.width;
    *height = g_frame_buffer.height;
    
    return 0;
}

void clear_frame_data()
{
    if (g_frame_buffer.data != NULL) {
        free(g_frame_buffer.data);
        g_frame_buffer.data = NULL;
    }
    g_frame_buffer.ready = false;
    g_frame_buffer.size = 0;
}

int main()
{	
	int err;

	// Initialise  the device
	xdtusb_device_t* pdev;
	err = init_device(&pdev);
	if (err != 0)
	{
		XDTUSB_Exit();
		exit(EXIT_FAILURE);
	}

	// Configure capture settings 
	uint32_t exposure_us = 100000;
	err = set_capture_settings(pdev, exposure_us);
	if (err != 0)
	{
		XDTUSB_Exit();
		exit(EXIT_FAILURE);
	}

	// Capture frame
	err = capture_frame(pdev);
	if (err != 0)
	{
		XDTUSB_Exit();
		exit(EXIT_FAILURE);
	}

	// Get frame data and verify it worked
	uint8_t* data;
	uint32_t size;
	uint32_t width;
	uint32_t height;
	err = get_frame_data(&data, &size, &width, &height);
	if (err != 0)
	{
		fprintf(stderr, "Failed to get frame data\n");
		XDTUSB_Exit();
		exit(EXIT_FAILURE);
	}

	// Print frame info to verify
	printf("\n=== Frame Data Retrieved ===\n");
	printf("Size: %u bytes\n", size);
	printf("Dimensions: %u x %u\n", width, height);
	printf("First 10 pixels: ");
	uint16_t* pixels = (uint16_t*)data;
	for (int i = 0; i < 10 && i < (int)(size/2); i++) {
		printf("%04x ", pixels[i]);
	}
	printf("\n============================\n");

	// Close device
	err = cleanup_capture_device(pdev);
	if (err != 0)
	{
		XDTUSB_Exit();
		exit(EXIT_FAILURE);
	}

	// Clean up frame buffer
	clear_frame_data();

	XDTUSB_Exit();

	exit(EXIT_SUCCESS);
}



/* EOF */




