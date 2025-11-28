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

	// Store frame data 
	// uint32_t frame_width = pframeDims->width;

	// Trace out the first 16 pixels of the frame here
	const uint32_t traceCount = 16;
	for (uint32_t i = 0; i < (traceCount+(8-1))/8; i++) {
		printf ("%04x: %04x %04x %04x %04x %04x %04x %04x %04x\n", i*16, pframeData[0], pframeData[1], pframeData[2], pframeData[3], pframeData[4], pframeData[5], pframeData[6], pframeData[7]);
		pframeData += 8;
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

	nanosleep(&(struct timespec){0, 10000000}, NULL);
	
	// Stop Streaming mode
	err = XDTUSB_DeviceStopStreaming(pdev);
	if (err != XDTUSB_ERROR_SUCCESS)
	{
		fprintf(stderr, "StopStreaming failed: %s\n", XDTUSB_UtilErrorString(err));
		return 1;
	}
	else
	{
		printf("Captured frame successfully \n");
		return 0;
	}

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

void get_frame_data()
{
	printf("Getting frame data \n");
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

	// Close device
	err = cleanup_capture_device(pdev);
	if (err != 0)
	{
		XDTUSB_Exit();
		exit(EXIT_FAILURE);
	}	

	XDTUSB_Exit();

	exit(EXIT_SUCCESS);
}



/* EOF */




