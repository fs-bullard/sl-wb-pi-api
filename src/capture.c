/** \file
 *
 * libxdtusb example.
 *
 * This example receives a SequenceExposureMode sequence.
 * Received frames are handled in the frame_cb callback.
 *
 * There are NUM_FRAME_BUFS frame buffers (which also equals the length of the sequence),
 * which ensures that even if the Application does not commit frames immediately
 * (e.g. if the processing in frame_cb takes very long or the application would not commit the frames right away
 * in the frame_cb (it does in the example)), all frames will be received.
 *
 * \author peter.dietzsch@ib-dt.de http://www.ib-dt.de
 *
 */


/*----------------------------------------------------------------------------*\
	Headers
\*----------------------------------------------------------------------------*/
#include <time.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "xdtusb.h"

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

	// Do some processing on the data
	//
	// This can be anything done directly here (there is no
	// strict timing requirement on this callback)
	// or the frame could also be passed to another thread

	// For simplicity just output some frame information
	printf("\nFrame received (h=%4u, w=%4u)\n", pframeDims->height, pframeDims->width);
	printf("-------------------------------\n");
	// Trace out the first 16 pixels of the frame here
	const uint32_t traceCount = 16;
	for (uint32_t i = 0; i < (traceCount+(8-1))/8; i++) {
		printf ("%04x: %04x %04x %04x %04x %04x %04x %04x %04x\n", i*16, pframeData[0], pframeData[1], pframeData[2], pframeData[3], pframeData[4], pframeData[5], pframeData[6], pframeData[7]);
		pframeData += 8;
	}

	uint32_t* pNumFramesReceived = (uint32_t*) puserargs;

	// Increment number of frames received counter (Main loop looks at this)
	(*pNumFramesReceived)++;

	// When done commit processed buffer back to device so it is
	// available for another frame reception
	XDTUSB_FramebufCommit(pfb);
}



int capture_frame(uint32_t exposure_ms)
{

	// Initialize libxdtusb
	XDTUSB_Init();

	XDTUSB_SetTraceMode(XDTUSB_TRACEMODE_DEBUG);

	printf(">>> Please Connect XDT device! <<<\n");

	// We are polling the device list every second
	// and time-out after 20 seconds and no device was found
	// This polling could be moved to a dedicated thread which signals a found
	// device to another
	for (int i = 0; i < 20; ++i)
	{
		// Poll devices
		uint8_t numDevices;
		xdtusb_device_t** devlist;
		XDTUSB_PollDevices(&numDevices, &devlist);

		// Trace progress
		printf("%us: Found %u devices\r", i, numDevices);
		fflush(stdout);

		// If XDTUSB devices were found
		if (numDevices > 0)
		{

			// Use first device found
			xdtusb_device_t* pdev = devlist[0];

			// Open device
			if (XDTUSB_ERROR_SUCCESS == XDTUSB_DeviceOpen(pdev, 1))
			{

				// Get some firmware information
				xdtusb_fwinfo_t afp_fwinfo;
				XDTUSB_DeviceGetAfpInfo(pdev, &afp_fwinfo);
				printf("AFP rev=0x%04X, build time=%02u-%02u-%04u %02u:%02u\n", afp_fwinfo.rev, afp_fwinfo.day, afp_fwinfo.month, afp_fwinfo.year, afp_fwinfo.hour, afp_fwinfo.minute);

				// Set Acquisition Mode
				XDTUSB_DeviceSetAcquisitionMode(pdev, XDT_ACQ_MODE_SEQ);

				// Configure Sequence
				XDTUSB_DeviceSetSequenceModeParameters(pdev, /*numFrames*/ 1, /*expTimeUs*/ exposure_ms, /*numDummy*/ 0, /*expTimeDummy*/ 0);

				// Start Streaming mode
				XDTUSB_DeviceStartStreaming(pdev, frame_cb, NULL);

				// sleep(1);

				// Issue SW trigger
				XDTUSB_DeviceIssueSwTrigger(pdev);

				// TODO: Do we need to sleep to make sure frame is received before stopping stream

				// Stop Streaming mode
				XDTUSB_DeviceStopStreaming(pdev);

				// Close device
				XDTUSB_DeviceClose(pdev);

			}
			else
			{
				XDTUSB_Exit();
				exit(EXIT_FAILURE);
			}
			// Done
			break;

		}

		// Sleep a while before re-polling connected devices
		nanosleep(&(struct timespec){1, 0}, NULL);

	}

	XDTUSB_Exit();

	exit(EXIT_SUCCESS);
}


/* EOF */




