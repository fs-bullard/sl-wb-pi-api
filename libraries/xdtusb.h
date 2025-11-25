/** \file
 *
 * \author 2008-2025 Ingenieurbuero Dietzsch und Thiele PartG https://www.ib-dt.de
 *
 * \defgroup _grpInitAndDetect Initialization and device detection
 *
 * \addtogroup _grpInitAndDetect
 * @{
 *
 * In order to use the library the following steps need to be performed:
 * -# Initialize the library. The library needs to initialized via \ref XDTUSB_Init,
 *    before running any other function. An exception is the \ref XDTUSB_SetTraceLevel()
 *    function which can be called before, allowing to set a higher trace level during
 *    library initialization for development.
 * -# Detect devices. Currently connected devices,
 *    have to be polled via \ref XDTUSB_PollDevices(), which returns a list
 *    of devices that are suitable for the library.
 * -# A listed device must be opened prior to other calls to the device (see \ref XDTUSB_DeviceOpen()).
 *    On opening a defined number of frame buffers will be allocated with the device.
 * -# The required calls can now happen on the opened device.
 *
 * On Application exit the following steps should be performed:
 * -# Close opened devices (see \ref XDTUSB_DeviceClose())
 * -# Exit the library context (\ref XDTUSB_Exit()).
 *
 * @}
 *
 *
 * \defgroup _grpImageAcquisition Image acquisition
 *
 * \addtogroup _grpImageAcquisition
 * @{
 *
 * Each opened device has a number of frame buffer allocated with it where it can store
 * received frames from the hardware.
 *
 * A frame buffer is either exclusively associated with the device or
 * associated with the Application.
 *
 * On device opening all allocated frame buffers are initially associated with the
 * device.
 *
 * During Image acquisition (i.e. when the device is in Streaming mode, see \ref XDTUSB_DeviceStartStreaming()),
 * received images are stored to the frame buffers associated to the device.
 *
 * After a frame was completely received the frame buffer is 'filled' and now exclusively
 * available to the Application. Filled frame buffers are to be processed by the
 * Application and need to be committed back to the device, when the processing
 * finished, in order to allow the device to store further frames to them (if required).
 *
 * There are two ways to signal the Application filled frame buffers:
 * -# The prefered way is to install a frame call-back, when starting streaming mode (see \ref XDTUSB_DeviceStartStreaming()).
 *    The call-back is asynchronously (i.e. from a thread different to the Applications main loop)
 *    called as soon as a frame is available for the Application.
 *    In this callback any processing (displaying, copying, analysis) can take
 *    place immediately or the frame buffer or the buffer may be passed to another
 *    thread for processing. However, the Application should re-commit the provided
 *    buffer to the device as soon as possible, in case it needs to be re-used
 *    for another frame reception (i.e. when the number of frames to be received
 *    is larger than the number of frame buffers allocated to the device).
 *    Note that within the callback only the following API-functions are allowed to
 *    be called in order to retrieve the data from the announced framebuffer and
 *    to commit back to the device:
 *    - \ref XDTUSB_FramebufGetDimensions()
 *    - \ref XDTUSB_FramebufGetPixelWidth()
 *    - \ref XDTUSB_FramebufGetData()
 *    - \ref XDTUSB_FramebufCommit().
 *
 * -# Another option is not to use a frame call-back, but to poll for received
 *    frames in the Applications event loop or asynchonously from different dedicated threads.
 *    In order to poll for ready frame-buffers the following functions can be used:
 *    - \ref XDTUSB_DeviceFramebufPollFilledCount()
 *    - \ref XDTUSB_DeviceFramebufPollFilledList()
 *    - \ref XDTUSB_DeviceFramebufFreeFilledList()
 *    - \ref XDTUSB_DeviceFramebufGetFirstFilled().
 *    As with the callback mechanism, the following functions can then be used to access the frame buffer data
 *    and to commit it back to the library for further data reception:
 *    - \ref XDTUSB_FramebufGetDimensions()
 *    - \ref XDTUSB_FramebufGetPixelWidth()
 *    - \ref XDTUSB_FramebufGetData()
 *    - \ref XDTUSB_FramebufCommit().
 *
 * In general, the process of image acquisition needs the following steps to happen:
 * -# When streaming is stopped (\ref XDTUSB_DeviceStopStreaming()), configure the detector for the wanted modes
 *    e.g. XDTUSB_DeviceConfigureExposureModeSequenceManual(), XDTUSB_DeviceConfigureExposureModeContinuous25Fps() or use the lower-level
 *    functions:
 *    - \ref XDTUSB_DeviceSetSequenceModeParameters()
 *    - \ref XDTUSB_DeviceSetDigitalBinningInteger()
 *    - \ref XDTUSB_DeviceSetDigitalBinning3to2()
 *    - \ref XDTUSB_DeviceSetRoiVertical()
 *    - \ref XDTUSB_DeviceSetRoiHorizontal()
 *    - \ref XDTUSB_DeviceSetFullWellMode()
 *    - \ref XDTUSB_DeviceSetTestPattern()
 *    - \ref XDTUSB_DeviceSetAutotriggerRoi()
 *    - \ref XDTUSB_DeviceSetAveraging()
 *    - ...
 *    - especially select the wanted acquisition mode: \ref XDTUSB_DeviceSetAcquisitionMode()
 * -# Start streaming (\ref XDTUSB_DeviceStartStreaming()), which sets up the detector
 *    to send-out image frames and the libxdtusb device to receive frames.
 * -# If the \ref xdt_acq_mode_t requires, e.g. after \ref XDT_ACQ_MODE_SW, XDT_ACQ_MODE_SEQ, XDT_ACQ_MODE_HWSW,
 *    issue a software or hardware trigger, respectively
 *    If the \ref xdt_acq_mode_t is an auto-triggered mode (also \see XDTUSB_DeviceSetAutotriggerRoi()), arm the auto trigger via XDTUSB_DeviceArmAutoTriggerRelative()
 * -# Process the received frames, if required commit them back to the device.
 * -# After all frames of interest have been received stop streaming, which allows for
 *    changing the exposure mode program.
 *
 * @}
 *
 */


#ifndef XDTUSB_H_
#define XDTUSB_H_

#ifdef _WIN32
	#ifdef BUILD_API
		#define API_EXPORT(RETVAL) __declspec(dllexport) RETVAL __cdecl
		#define API_DEPRECATED(RETVAL) __declspec(dllexport) __declspec(deprecated) RETVAL __cdecl
	#else
		#define API_EXPORT(RETVAL) __declspec(dllimport) RETVAL __cdecl
		#define API_DEPRECATED(RETVAL) __declspec(dllimport) __declspec(deprecated) RETVAL __cdecl
	#endif
#else
	#define API_EXPORT(RETVAL) RETVAL
	#define API_DEPRECATED(RETVAL) RETVAL __attribute__((deprecated))
#endif


#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*\
	Headers
\*----------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/*----------------------------------------------------------------------------*\
	Macros
\*----------------------------------------------------------------------------*/

/** XDTUSB API version */
#define XDTUSB_API_VERSION_MAJOR 1
#define XDTUSB_API_VERSION_MINOR 7
#define XDTUSB_API_VERSION_PATCH 0

/** Required Application Firmware Package version
 *
 * Minimum AFP version supported by this version of libxdtusb
 */
#define XDTUSB_AFP_VERSION_MAJOR 0
#define XDTUSB_AFP_VERSION_MINOR 4
#define XDTUSB_AFP_VERSION_PATCH 0

/*----------------------------------------------------------------------------*\
	Constants
\*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*\
	Types
\*----------------------------------------------------------------------------*/

/** XDTUSB error codes */
typedef enum {
	XDTUSB_ERROR_SUCCESS         ,
	XDTUSB_ERROR_IO              ,
	XDTUSB_ERROR_INVALID_PARAM   ,
	XDTUSB_ERROR_ACCESS          ,
	XDTUSB_ERROR_NO_DEVICE       ,
	XDTUSB_ERROR_NOT_FOUND       ,
	XDTUSB_ERROR_BUSY            ,
	XDTUSB_ERROR_TIMEOUT         ,
	XDTUSB_ERROR_OVERFLOW        ,
	XDTUSB_ERROR_PIPE            ,
	XDTUSB_ERROR_INTERRUPTED     ,
	XDTUSB_ERROR_NO_MEM          ,
	XDTUSB_ERROR_NOT_SUPPORTED   ,
	XDTUSB_ERROR_ALREADY_EXISTS  ,
	XDTUSB_ERROR_INTERNAL        ,
	XDTUSB_ERROR_OTHER           ,
} xdtusb_error_t;

/** XDTUSB trace modes */
typedef enum {
	XDTUSB_TRACEMODE_NONE  = 0,
	XDTUSB_TRACEMODE_FATAL = 1,
	XDTUSB_TRACEMODE_ERROR = 2,
	XDTUSB_TRACEMODE_WARN  = 3,
	XDTUSB_TRACEMODE_INFO  = 4,
	XDTUSB_TRACEMODE_VERBOSE = 5,
	XDTUSB_TRACEMODE_DEBUG = 6,
} xdtusb_tracemode_t;

/** XDTUSB USB device speed */
typedef enum {
	XDTUSB_SPEED_UNKNOWN,   /**< USB speed unknown */
	XDTUSB_SPEED_LOW,       /**< USB Low Speed */
	XDTUSB_SPEED_FULL,      /**< USB Full Speed */
	XDTUSB_SPEED_HIGH,      /**< USB High Speed */
	XDTUSB_SPEED_SUPER,     /**< USB Super Speed */
	XDTUSB_SPEED_SUPER_PLUS /**< USB SuperPlus Speed */
} xdtusb_speed_t;

/** XDTUSB USB device information */
typedef struct {
	uint8_t usb_bus;        /**< USB bus the device is connected to */
	uint8_t usb_prt;        /**< USB port of the bus the device is connected to */
	uint8_t usb_adr;        /**< Assigned USB address */
	xdtusb_speed_t usb_spd; /**< Speed the device is connected with */
	char* serialNumber;     /**< Serial number of the XDT device */
	uint16_t platform_hwid; /**< Platform Hardware ID (Board type) */
	uint16_t platform_varid; /**< Platform Variant ID (FW variant) */
	const char* platform_hwstr; /**< String representation of HW ID / Board type */
	const char* platform_varstr; /**< String representation of HW ID / Board type */
} xdtusb_device_info_t;

/** XDTUSB Software/Firmware version/build information */
typedef struct {
	uint16_t maj;           /**< Major version */
	uint16_t min;           /**< Minor version */
	uint16_t rev;           /**< Patch Revision */
	uint16_t bld;           /**< Build counter */
	uint16_t year;          /**< Build time year */
	uint8_t  month;         /**< Build time month of year */
	uint8_t  day;           /**< Build time day of month */
	uint8_t  hour;          /**< Build time hour of day */
	uint8_t  minute;        /**< Build time minute of hour */
} xdtusb_swinfo_t;

/** XDTUSB Firmware information
 * \deprecated Use xdtusb_swinfo_t instead.
 * */
typedef xdtusb_swinfo_t xdtusb_fwinfo_t;


/** XDT Acquisition Mode */
typedef enum {
	XDT_ACQ_MODE_SW,   /**< Single snap (SW-Triggered via register write) */
	XDT_ACQ_MODE_25FPS, /**< 35 frames per second */
	XDT_ACQ_MODE_30FPS, /**< 30 frames per second */
	XDT_ACQ_MODE_XFPS, /**< Arbitrary frame rate
	                        Note: This is not supported on EIO-LCY detectors. */
	XDT_ACQ_MODE_SEQ,  /**< Sequence mode acquisiton (SW- or HW-triggered) */
	XDT_ACQ_MODE_HWSW,  /**< Single snap (SW- or HW-triggered) */
	XDT_ACQ_MODE_SEQAUTO, /**< Sequence mode acquisiton (Auto trigger)
	                          Note: This is only supported on EIO-LCY detectors. */
	XDT_ACQ_MODE_LPSE ,  /**< Low-power single exposure (SW- or HW-triggered)
	                          Enters sensor's low-power mode, before a a timed single exposure. */

} xdt_acq_mode_t;

/** Full well mode */
typedef enum {
	XDTUSB_FWM_LOWFULLWELL = 0,    /** Low Full Well mode (High gain) */
	//XDTUSB_FWM_MIDFULLWELL = 1, /** Medium Full Well mode (Medium gain) */
	XDTUSB_FWM_HIGHFULLWELL = 2   /** High Full Well mode (Low gain) */
} xdtusb_fwm_t;

/** XDTUSB Reference voltages */
typedef struct {
	uint16_t vreset;
	uint16_t vmidh;
	uint16_t ibias;
	uint16_t vcm;
	uint16_t in100;
	uint16_t vrefp;
	uint16_t vrefn;
	uint16_t vrefd;
} xdtusb_vrefs_t;

/** Image dimensions */
typedef struct {
	uint16_t width; /**< Width of the image in pixel columns */
	uint16_t height; /**< Width of the image in pixel rows */
} xdtusb_frame_dimensions_t;

/** Throughput statistics */
typedef struct {
	uint16_t fbs_avail; /**< Number of detector HW frames available (free) at the time of reading. Normally this value will settle
	                        near the maximum value (number of total hardware frame buffers of the detector).
	                        However, when the effective average sensor throughput is larger than the effective average throughput of
	                        the host interface, the number of available frame-buffers decreases over time, when it reached a value of 0,
	                        the detector will drop frames. */
	uint16_t num_drop; /**< Number of dropped frames since last read. This value should normally read as 0.
	                        However, if the effective sensor throughput is higher than the throughput of the
	                        host interface, the internal frame buffer will be filled as a first buffering mechanism
	                        (\c xdtusb_tpstats_t.fbs_avail decreases). When there is no more hardware frame buffer,
	                        the detector is dropping frames. Note that frames are only dropped for buffering
	                        and thus not be transfered to the host; but they will be actually be read from the sensor
	                        (and hence will also refresh the sensor).

	                        If one experiences frame drops, one should consider decreasing the frame-rate (e.g. increase exposure time or frame period),
	                        or reduce the amount of data to sent to the host (e.g. use digital binning or (horizontal) ROI).
                        */
} xdtusb_tpstats_t;

/** Opaque Framebuffer Handle */
typedef struct xdtusb_framebuf xdtusb_framebuf_t;

/** Opaque XDTUSB Device Handle */
typedef struct xdtusb_device xdtusb_device_t;

/** @brief Frame received callback
 *
 * The Application-defined frame received callback is called when an frame was received after a device was put into streaming mode:
 *
 * Before the \ref xdtusb_frame_cb_t callback is called, the framebuffer is automatically released from the device,
 * giving the Application exclusive access to its content, it however is then not available for the device to store
 * incoming image frames.
 *
 * When the Application processed the data of the provided framebuffer it should re-commit it to the device (\ref XDTUSB_FramebufCommit())
 * in order to allow the device to receive another image frame to the buffer.
 * Re-committing can be done right away after processing the data in this callback context or later from the Applications processing thread/loop.
 *
 * \note In the callback context, no operations to the device are allowed (no calls to the \c XDTUSB_Device...(), including \ref ),
 *
 * The only functions that are safe to call are
 * - \ref XDTUSB_FramebufGetDimensions()
 * - \ref XDTUSB_FramebufGetPixelWidth()
 * - \ref XDTUSB_FramebufGetData()
 * - \ref XDTUSB_FramebufCommit().
 *
 * @param pdev Opaque pointer to device handle that filled the framebuffer.
 * @param pfb Opaque pointer to the filled frame buffer handle.
 * @param puserargs Pointer to optional user arguments that will be passed to the callback on device detection event.
 *
 */
typedef void (*xdtusb_frame_cb_t)(xdtusb_device_t* pdev, xdtusb_framebuf_t* pfb, void* puserargs);

/** XDTUSB pixel type
 *
 * The pixel type represents the gray-value of a single pixel.
 */
typedef uint16_t xdtusb_pixel_t;

/*----------------------------------------------------------------------------*\
	Variables
\*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*\
	Functions
\*----------------------------------------------------------------------------*/

/** @brief
 * Get XDTUSB library version and build information.
 *
 */
API_EXPORT(xdtusb_error_t) XDTUSB_GetSwInfo(xdtusb_swinfo_t *pSwInfo);

API_EXPORT(const char*) XDTUSB_UtilErrorString(xdtusb_error_t err);

API_EXPORT(const char*) XDTUSB_UtilUsbSpeedString(xdtusb_speed_t spd);

/** @brief
 * Initialize XDTUSB library.
 *
 * @note This function is to be called ONCE prior accessing any other library function.
 */
API_EXPORT(xdtusb_error_t) XDTUSB_Init(void);

/** @brief
 * Exit XDTUSB library.
 *
 * @note This function should be called on application exit and will clean-up the library context and
 * state of used devices.
 */
API_EXPORT(xdtusb_error_t) XDTUSB_Exit(void);

/** @brief Set general library trace mode.
 */
API_EXPORT(xdtusb_error_t) XDTUSB_SetTraceMode(xdtusb_tracemode_t mode);

/** @brief Open tracing log file.
 *
 * Will open the given filename for logging and redirects all XDTUSB traces to this file.
 *
 * Must be run before XDTUSB_Init().
 *
 * @param[in] fn Filename of logfile to be used.
 */
API_EXPORT(xdtusb_error_t) XDTUSB_OpenTraceLogfile(const char* fn);

/** @brief Open tracing log file.
 *
 * Will close the opened logfile.
 *
 * Must be run after XDTUSB_Exit().
 *
 */
API_EXPORT(xdtusb_error_t) XDTUSB_CloseTraceLogfile();

/** @brief Poll for connected devices
 *
 * This generates the list of connected XDTUSB devices.
 * The application is allowed to open devices included in this device list.
 *
 * \note This function is not re-entrant. The generated list is valid until recalling the function.
 *
 * @param[out] pNumDevs Number of XDTUSB devices found
 * @param[out] ppDevList Location to store the pointer to first element of the NULL-terminated device pointer list to.
 */
API_EXPORT(xdtusb_error_t) XDTUSB_PollDevices(uint8_t* pNumDevs, xdtusb_device_t*** ppDevList);

/** @brief Open a connected device
 *
 * Opens a connected device in order to allow for further device operations.
 *
 * @param[in] pdev Pointer to device.
 * @param numFbs Number of frame buffer to be allocated with the device.
 */
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceOpen(xdtusb_device_t* pdev, uint32_t numFbs);

/** @brief Open close an opened device
 *
 * Opens a connected device (see \ref xdtusb_detect_cb_t) in order to allow for further device operations.
 * @param[in] pdev Pointer to device.
 */
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceClose(xdtusb_device_t* pdev);

/** Set trace mode for a specific device
 *
 * Sets the trace mode for a specific device.
 * @param[in] pdev Pointer to device.
 * @param mode Trace mode to use for the device.
 */
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceSetTraceMode(xdtusb_device_t* pdev, xdtusb_tracemode_t mode);

API_EXPORT(xdtusb_error_t) XDTUSB_DeviceGetInfo(xdtusb_device_t* pdev, xdtusb_device_info_t* pdevinfo);

/** Reboot a device.
 *
 * Triggers a detector reboot.
 */
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceReboot(xdtusb_device_t* pdev);


API_EXPORT(xdtusb_error_t) XDTUSB_DeviceGetAfpInfo(xdtusb_device_t* pdev, xdtusb_fwinfo_t* pfwinfo);
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceGetBtlInfo(xdtusb_device_t* pdev, xdtusb_fwinfo_t* pfwinfo);


/** Get per-frame sensor readout time.
 *
 * Returns the time required by the detector to readout a frame from the sensor (\c t_R).
 * This value can be used to calculate the total per-frame integration time for sequence mode exposure
 * (\see XDTUSB_DeviceConfigureExposureModeSequenceManual(), \see XDTUSB_DeviceConfigureExposureModeSequenceAuto())
 * or the likewise the remaining exposure time exposure time for the 25fps continous exposure mode (XDTUSB_DeviceConfigureExposureModeContinuous25Fps()).
 *
 * @note The readout time is implementation specific and may change in future firmware revisions.
 *
 * @param[in] pdev Pointer to device.
 * @param[out] pReadoutTimeUs Sensor readout time in micro seconds.
 */
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceGetReadoutTime(xdtusb_device_t* pdev, uint32_t* pReadoutTimeUs);


/** Set Acquisition Mode
 *
 * @param[in] pdev Pointer to device.
 * @param     acq_mode Acquisition mode to use.
 */
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceSetAcquisitionMode(xdtusb_device_t* pdev, xdt_acq_mode_t acq_mode);


/** Set averaging
 *
 * Image averaging HW-averages each \ref numAvg captured images (that would otherwise be transported to the
 * host) and outputs the averaged result as one frame (every \ref numAvg captured images).
 *
 * @note the internal averaging counter is reset on XDTUSB_DeviceStopStreaming().
 *
 * @param[in] pdev Pointer to device.
 * @param     numAvg Number of frames to average per on output frame (0: reserved).
 *
 */
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceSetAveraging(xdtusb_device_t* pdev, uint16_t numAvg);


/** Set SequenceMode acquisition parameters.
 *
 * This configures the parameters for sequence mode acquisition (in order to use \ref XDT_ACQ_MODE_SEQ or \ref XDT_ACQ_MODE_SEQAUTO).
 *
 * Sequence acquires \ref numFrames images from the sensor.
 * When \c Sequence-Auto-Cutoff is used (\ref XDTUSB_DeviceSetSequenceAutoCutoff()), the actual number of
 * acquired frames may be lower.
 * When \c Averaging is used (\ref XDTUSB_DeviceSetAveraging(AVERAGING_COUNT>1)), the number of frames acquired will be lower.
 *
 * Before the readout of each of the \ref numFrames sequence images, an exposure time gap of \ref expTimeUs is pre-pended.
 *
 * After the sequence acquistion is triggered, internally a dummy frame is read from the sensor in order to reset the sensor matrix.
 * Optionally, an additional number of \ref numSkp skip frames can be readout before acquisition
 * of the actual requested frames. The skipped frames have an prepended exposure time of \ref skpTimeUs and are not provided to the
 * Application.
 *
 * @note The total per-frame integration time \c t_Int calculates from the configured exposure times plus the sensor readout time \c tRead (\see XDTUSB_DeviceGetReadoutTime()).
 * @note The total time of the sequence calculates from <tt> tRd + numSkp * (tSkp + tRead) + numFrames * (tExp + tRead) </tt>.
 * @note This function must be called while streaming is stopped (see \ref XDTUSB_DeviceStopStreaming()).
 * @note After configuring sequence mode parameters, the device's acquisition mode needs to be set to \ref XDT_ACQ_MODE_SEQ or \ref XDT_ACQ_MODE_SEQAUTO in order to enable a
 *       sequence exposure.
 *
 * @param[in] pdev Pointer to device.
 * @param numFrames Number of frames to acquire
 * @param expTimeUs Exposure time (\c tExp ) added prior to each acquired frame readout.
 * @param numSkp Additional number of dummy readouts (not transported to the Application)
 *                 that are performed before acquiring the \ref numFrames images.
 * @param skpTimeUs Exposure time (\c tSkp ) added prior to each additional dummy frame readout.
  */
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceSetSequenceModeParameters(xdtusb_device_t* pdev, uint16_t numFrames, uint32_t expTimeUs, uint16_t numSkp, uint32_t skpTimeUs);
#define XDTUSB_DeviceConfigureExposureModeSequence XDTUSB_DeviceSetSequenceModeParameters


/** Configure device for Manual Sequence Exposure
 *

 * This configures the given device for sequence exposure which is manually triggered by Software,
 * by setting the given sequence mode parameters and additionally entering \ref XDT_ACQ_MODE_SEQ acquisition mode.
 *
 * Manual Sequence exposure mode is a triggered exposure mode.
 * It acquires \ref numFrames images from the sensor.
 * When \c Sequence-Auto-Cutoff is used (\ref XDTUSB_DeviceSetSequenceAutoCutoff()), the actual number of acquired frames may be lower.
 * When \ref average = \c false, all of the \ref NumFrames sequence images are transported to
 * the Software.
 * When \ref average = \c true, the \ref numFrames sequence images are averaged in hardware and only _one_ (average) frame is transported to the Software. \ref numFrames is restricted to be in range 1 to 256 in this case.
 *
 * Before the readout of each of the \ref numFrames sequence images, an exposure time gap of \ref expTimeUs is pre-pended.
 *
 * After triggering the sequence exposure, internally a dummy frame is read from the sensor in order to reset the sensor matrix.
 * Optionally, an additional number of \ref numSkp skip frames can be readout before acquisition
 * of the actual requested frames. The skipped frames have an prepended exposure time of \ref skpTimeUs and are not provided to the
 * Application.
 *
 * @note The total per-frame integration time \c t_Int calculates from the configured exposure times plus the sensor readout time \c tRead (\see XDTUSB_DeviceGetReadoutTime()).
 * @note The total time of the sequence calculates from <tt> tRd + numSkp * (tSkp + tRead) + numFrames * (tExp + tRead) </tt>.
 * @note This function must be called while streaming is stopped (see \ref XDTUSB_DeviceStopStreaming()).
 * @note To start the sequence exposure, a Software Trigger needs to be issued (see \ref XDTUSB_DeviceIssueSwTrigger()) after
 *       entering Streaming mode (see \ref XDTUSB_DeviceStartStreaming())
 * @note The equivalent functionality of this function be generated by calling the following functions:
 *       -# \ref XDTUSB_DeviceSetSequenceModeParameters(pdev, numFrames, expTimeUs, numSkp, skpTimeUs)
 *       -# \ref XDTUSB_DeviceSetAveraging(pdev, numFrames)
 *       -# \ref XDTUSB_DeviceSetAcquisitionMode(pdev, XDT_ACQ_MODE_SEQ)
 *
 * @param[in] pdev Pointer to device.
 * @param numFrames Number of frames to acquire
 * @param expTimeUs Exposure time (\c tExp ) added prior to each acquired frame readout.
 * @param numSkp Additional number of dummy readouts (not transported to the Application)
 *                 that are performed before acquiring the \ref numFrames images.
 * @param skpTimeUs Exposure time (\c tSkp ) added prior to each additional dummy frame readout.
 * @param average Enables / disables sequence image averaging.
 */
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceConfigureExposureModeSequenceManual(xdtusb_device_t* pdev, uint16_t numFrames, uint32_t expTimeUs, uint16_t numSkp, uint32_t skpTimeUs, bool average);

/** Configure device for Auto Sequence Exposure
 *
 * This configures the device for sequence exposure which is automatically triggered (after arming, (\see XDTUSB_DeviceArmAutoTriggerRelative()) when
 * a given threshold of the average of a defined trigger ROI is exceeded.
 *
 * After configuring Auto Sequence Exposure and entering Streaming Mode, the detector continuously reads out the configured trigger ROI and calculates
 * its average.
 * When the application then arms the AutoTrigger (\see XDTUSB_DeviceArmAutoTriggerRelative()), the current average is latched and used as the dark reference.
 * The detector continuous to readout the trigger ROI and as soon as the trigger ROI's average exceeds the reference value + the set threshold, the
 * sequence is acquired.
 *
 * The exposure mode acquires \ref numFrames images from the sensor. When \c Sequence-Auto-Cutoff is used (\ref XDTUSB_DeviceSetSequenceAutoCutoff()), the actual number of
 * acquired frames may be lower.
 * When \ref average = \c false, all of the \ref numFrames sequence images are transported to
 * the Software.
 * When \ref average = \c true, the \ref NumFrames sequence images are averaged in hardware and only _one_ (average) frame is transported to the Software. \ref numFrames is restricted to be in range 1 to 256 in this case.
 *
 * Before the readout of each of the \ref numFrames sequence images, an exposure time gap of \ref expTimeUs is pre-pended.
 *
 * Internally a dummy frame is read from the sensor (this readout frame is not sent to the Application)
 * in order to reset the sensor matrix.
 * Optionally, an additional number of \ref numSkp skip frames can be readout before acquisition
 * of the actual requested frames. The skipped frames have an prepended exposure time of \ref skpTimeUs and are not provided to the
 * Application.
 *
 * @note The total per-frame integration time \c t_Int calculates from the configured exposure times plus the sensor readout time \c tRead (\see XDTUSB_DeviceGetReadoutTime()).
 * @note The total time of the sequence calculates from <tt> tRd + numSkp * (tSkp + tRead) + numFrames * (tExp + tRead) </tt>.
 * @note This function must be called while streaming is stopped (see \ref XDTUSB_DeviceStopStreaming()).
 * @note To start the sequence exposure, the auto trigger of the detector needs to be armed (see \ref XDTUSB_DeviceArmAutoTriggerRelative()) after
 *       entering Streaming mode (see \ref XDTUSB_DeviceStartStreaming()).
 * @note The equivalent functionality of this function be generated by calling the following functions:
 *       -# \ref XDTUSB_DeviceSetSequenceModeParameters(pdev, numFrames, expTimeUs, numSkp, skpTimeUs)
 *       -# \ref XDTUSB_DeviceSetAutotriggerRoi(pdev, troiy, troih)
 *       -# \ref XDTUSB_DeviceSetAveraging(pdev, numFrames)
 *       -# \ref XDTUSB_DeviceSetAcquisitionMode(pdev, XDT_ACQ_MODE_SEQAUTO)
 *
 * @param[in] pdev Pointer to device.
 * @param numFrames Number of frames to acquire
 * @param expTimeUs Exposure time (\c tExp ) added prior to each acquired frame readout.
 * @param numSkp Additional number of dummy readouts (not transported to the Application)
 *                 that are performed before acquiring the \ref numFrames images.
 * @param skpTimeUs Exposure time (\c tSkp ) added prior to each additional dummy frame readout.
 * @param aroiy Image line to start trigger ROI at (must be even for Slingshot).
 * @param aroih Trigger ROI height in image lines, must be even for Slingshot).
 * @param average Enables / disables sequence image averaging.
 */
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceConfigureExposureModeSequenceAuto(xdtusb_device_t* pdev, uint16_t numFrames, uint32_t expTimeUs, uint16_t numSkp, uint32_t skpTimeUs, uint16_t aroiy, uint16_t aroih, bool average);


/** Configure device for Continuous 25fps mode.
 *
 * This configures the given device for continuous (free-running) exposure mode at a rate of 25 frames per second.
 * Any configured averaging will be disabled.
 *
 * Continuous Exposure Mode is an untriggered (free-running) mode, Image aqcuisition will start as soon as the
 * device is put into Streaming mode (\see XDTUSB_DeviceStartStreaming()).
 *
 * @note This function must be called while streaming is stopped (see \ref XDTUSB_DeviceStopStreaming()).
 *
 * @note The equivalent functionality can be generated by calling the following functions:
 *       -# \ref XDTUSB_DeviceSetAveraging(pdev, 1)
 *       -# \ref XDTUSB_DeviceSetAcquisitionMode(pdev, XDT_ACQ_MODE_25FPS)
 *
 * @param[in] pdev Pointer to device.
 */
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceConfigureExposureModeContinuous25Fps(xdtusb_device_t* pdev);


/** Configure device for Continuous arbitrary frame-rate mode.
 *
 * This configures the given device for continuous (free-running) exposure mode at the rate defined by the given frame period.
 *
 * Continuous Exposure Mode is an untriggered (free-running) mode, Image acquisition will start as soon as the
 * device is put into Streaming mode (\see XDTUSB_DeviceStartStreaming()).
 *
 * @note This function must be called while streaming is stopped (see \ref XDTUSB_DeviceStopStreaming()).
 *
 * @note The equivalent functionality can be generated by calling the following functions:
 *       -# \ref XDTUSB_DeviceSetAveraging(pdev, 1)
 *       -# \ref XDTUSB_DeviceSetAcquisitionMode(pdev, XDT_ACQ_MODE_XFPS)
 *
 * @param[in] pdev Pointer to device.
 * @param frame_period_us Frame period in in us.
 */
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceConfigureExposureModeContinuousXFps(xdtusb_device_t* pdev, uint32_t frame_period_us);


/** Set Auto-Trigger ROI.
 *
 * Currently supported only for \c EIO-LCY detectors.
 *
 * Sets the ROI used for auto-trigger evaluation, when acqusition mode \ref XDT_ACQ_MODE_SEQAUTO is used.
 *
 * The auto-trigger ROI can be different from the actual image ROI.
 *
 * @note This function must be called while streaming is stopped (see \ref XDTUSB_DeviceStopStreaming()).
 *
 * @param[in] pdev Pointer to device.
 * @param aroiy Image line to start trigger ROI at (must be even for \c EIO-LCY detectors).
 * @param aroih Trigger ROI height in image lines, must be even for \c EIO-LCY detectors).
 */
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceSetAutotriggerRoi(xdtusb_device_t* pdev, uint16_t aroiy, uint16_t aroih);

/** Arm auto-trigger
 *
 * Currently supported only for \c EIO-LCY detectors.
 * Arms the detector's auto trigger with the given trigger ROI threshold.
 *
 * The detector must be configured for auto-triggered acquisiton mode (\ref XDT_ACQ_MODE_SEQAUTO).
 *
 * Arming the auto trigger sets up the given trigger threshold and activates the auto trigger threshold detection in the detector.
 * The threshold is 'relative', i.e. the auto trigger fires when the average of the trigger ROI is larger than the reference average (taken at the time
 * of arming) plus the given threshold.
 *
 * @note This function must be called while streaming running in auto-triggered acquisition mode (i.e. after \ref XDTUSB_DeviceStartStreaming()).
 *
 * @param[in] pdev Pointer to device.
 * @param relativeThreshold Relative average pixel value threshold.
 */
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceArmAutoTriggerRelative(xdtusb_device_t* pdev, uint16_t relativeThreshold);

/** Force auto trigger
 *
 * Currently supported only for \c EIO-LCY detectors.
 *
 * Forces an auto-trigger event in order to acquire the configured image sequence when inAuto Trigger mode, regardless of the
 * an armed auto trigger (\ref XDTUSB_DeviceArmAutoTriggerRelative()) threshold is exceeded or not.
 *
 * The detector must be configured for Auto Sequence Exposure (\see XDTUSB_DeviceConfigureExposureModeSequenceAuto()) before issuing \ref XDTUSB_DeviceArmAutoTriggerRelative().
 *
 * @note This function must be called while streaming running in auto-triggered acquisition mode (i.e. after \ref XDTUSB_DeviceStartStreaming()).
 *
 * @param[in] pdev Pointer to device.
 * @param relativeThreshold Relative average pixel value threshold.
 */
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceForceAutoTrigger(xdtusb_device_t* pdev);

/** Set Sequence-Auto-CutOff
 *
 * Enables or disables Sequence-Auto-CutOff
 *
 * Currently supported only for \c EIO-LCY detectors.
 *
 * When enabled, an acquired sequence is stopped, when a readout frame's pixel value average is \c relativeThreshould lower than the average of the previous frame.
 * The frame that fulfills the above requirement will not be included in the sequence.
 * When enabled, the number of actual frames delivered may be lower than the number of frames configured for the sequence (\see XDTUSB_DeviceConfigureExposureModeSequenceManual(),
 * \see XDTUSB_DeviceConfigureExposureModeSequenceAuto() ), however at least one frame is guaranteed to be acquired from the sensor and to be output.
 *
 * @note This function must be called while streaming running in auto-triggered acquisition mode (i.e. after \ref XDTUSB_DeviceStartStreaming()).
 *
 * @param[in] pdev Pointer to device.
 * @param enable Enable/Disable Sequence-Auto-CutOff.
 * @param relativeThreshold Relative average pixel value cut-off threshold (when \ref enabled = \c true).
 */
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceSetSequenceAutoCutoff(xdtusb_device_t* pdev, bool enable, uint16_t relativeThreshold);


/** Start streaming.
 *
 * Starts streaming on the given device.
 *
 * This will enable the reception of frames coming from the detector, which will be received
 * into the available (committed) frame buffers.
 *
 * The Application provided \ref frame_cb is called whenever a frame was received into an frame buffer.
 *
 * @param[in] pdev Pointer to opened device.
 * @param[in] frame_cb Application-defined frame reception call-back (see \xdtusb_frame_cb_t)
 * @param[in] puserargs Pointer to optional user arguments provided when calling the \ref frame_cb call-back.
 *
 */
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceStartStreaming(xdtusb_device_t* pdev, xdtusb_frame_cb_t frame_cb, void *puserargs);


/** Start streaming.
 *
 * Starts streaming on the given device.
 *
 * This will enable the reception of frames coming from the detector, which will be received
 * into the available (committed) frame buffers.
 *
 * The Application provided \ref frame_cb is called whenever a frame was received into an frame buffer.
 *
 * @param[in] pdev Pointer to opened device.
 * @param[in] frame_cb Application-defined frame reception call-back (see \xdtusb_frame_cb_t)
 * @param[in] puserargs Pointer to optional user arguments provided when calling the \ref frame_cb call-back.
 *
 */
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceStartStreaming(xdtusb_device_t* pdev, xdtusb_frame_cb_t frame_cb, void *puserargs);

/** Set Full Well Mode
 *
 * Sets the sensor Full-Well mode.
 *
 * @param[in] pdev Pointer to opened device.
 * @param     fwm Full well mode to set.
 */
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceSetFullWellMode(xdtusb_device_t* pdev, xdtusb_fwm_t fwm);

/** Get Full Well Mode
 *
 * Gets the currently set sensor Full-Well mode.
 *
 * @param[in] pdev Pointer to opened device.
 * @param     *pFwm Location to store the current Full Well Mode to.
 */
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceGetFullWellMode(xdtusb_device_t* pdev, xdtusb_fwm_t* pFwm);

/** Enable/Disable Test Pattern
 *
 * Enable / disable detector test pattern generation.
 * When enabled, the detector outputs a test-pattern instead of the actual image data.
 *
 * @param[in] pdev Pointer to opened device.
 * @param     enable Indicates, whether to enable (when \c true) or disable (when \c false) test pattern generation.
 */
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceSetTestPattern(xdtusb_device_t* pdev, bool enable);

/** Get test pattern generation state.
 *
 * Checks whether the test pattern generation is currently enabled or disabled.
 *
 * @param[in]  pdev Pointer to opened device.
 * @param[out] pEnabled Location to store the state of the test pattern generation to.
 */
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceGetTestPattern(xdtusb_device_t* pdev, bool *pEnabled);
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceSetRoiVertical(xdtusb_device_t* pdev, uint32_t y, uint32_t h);
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceGetRoiVertical(xdtusb_device_t* pdev, uint32_t *pY, uint32_t *pH);
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceSetRoiHorizontal(xdtusb_device_t* pdev, uint32_t x, uint32_t w);
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceGetRoiHorizontal(xdtusb_device_t* pdev, uint32_t *pX, uint32_t *pW);
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceGetMaxDimensions(xdtusb_device_t* pdev, uint32_t *pW, uint32_t *pH);

API_EXPORT(xdtusb_error_t) XDTUSB_DeviceSetDirections(xdtusb_device_t* pdev, bool reverseYReadout, bool reverseYFrame, bool reverseXFrame);

/** Set digital binning (deprecated).
 *
 * Set digital hardware-side image binning.
 *
 * The sensor-ROI will be cropped in order to provide integer multiples of set ratio.
 *
 * This function is deprecated. Use the \ref XDTUSB_DeviceSetDigitalBinningInteger() instead.
 *
 * @note This function must be called while streaming running in auto-triggered acquisition mode (i.e. after \ref XDTUSB_DeviceStartStreaming()).
 *
 * @param[in] pdev Pointer to opened device.
 * @param     verticalRatio Vertical binning ratio. Allowed values: 1, 2, 4, 8, 16
 * @param     horizontalRatio Horizontal binning ratio. Allowed values: 1, 2, 4, 8, 16
 */
API_DEPRECATED(xdtusb_error_t) XDTUSB_DeviceSetDigitalBinning(xdtusb_device_t* pdev, uint8_t verticalRatio, uint8_t horizontalRatio);
API_DEPRECATED(xdtusb_error_t) XDTUSB_DeviceGetDigitalBinning(xdtusb_device_t* pdev, uint8_t *pVerticalRatio, uint8_t *pHorizontalRatio);

/** Set digital binning.
 *
 * Set digital hardware-side image binning.
 *
 * The sensor-ROI will be cropped in order to provide integer multiples of set ratio.
 *
 * @note This function must be called while streaming running in auto-triggered acquisition mode (i.e. after \ref XDTUSB_DeviceStartStreaming()).
 *
 * @param[in] pdev Pointer to opened device.
 * @param     verticalRatio Vertical binning ratio. Allowed values: 1, 2, 4, 8, 16
 * @param     horizontalRatio Horizontal binning ratio. Allowed values: 1, 2, 4, 8, 16
 */
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceSetDigitalBinningInteger(xdtusb_device_t* pdev, uint8_t verticalRatio, uint8_t horizontalRatio);
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceGetDigitalBinningInteger(xdtusb_device_t* pdev, uint8_t *pVerticalRatio, uint8_t *pHorizontalRatio);

/** Set digital 3-to-2 binning.
 *
 * Set digital hardware-side 3-to-2 (a.k.a. 1.5x) image binning.
 *
 * This will be applied on to to the integer binning modes to configured via \ref XDTUSB_DeviceSetDigitalBinning().
 * The sensor-ROI will be cropped in order to provide integer multiples of set ratio.
 *
 * @note This function must be called while streaming running in auto-triggered acquisition mode (i.e. after \ref XDTUSB_DeviceStartStreaming()).
 *
 * @param[in] pdev Pointer to opened device.
 * @param     en3to2 Enable 3-to-2  binning.
 */
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceSetDigitalBinning3to2(xdtusb_device_t* pdev, bool en3to2);
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceGetDigitalBinning3to2(xdtusb_device_t* pdev, bool *pen3to2);

API_EXPORT(xdtusb_error_t) XDTUSB_DeviceSetAnalogBinningInteger(xdtusb_device_t* pdev, uint8_t verticalRatio, uint8_t horizontalRatio);
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceGetAnalogBinningInteger(xdtusb_device_t* pdev, uint8_t *pVerticalRatio, uint8_t *pHorizontalRatio);

/** Set pixel depth.
 *
 * Sets the pixel depth on supported platforms.
 *
 * @note This function must be called while streaming running in auto-triggered acquisition mode (i.e. after \ref XDTUSB_DeviceStartStreaming()).
 *
 * @param[in]  pdev Pointer to opened device.
 * @param bitsPerPixel Number of bits per pixel (14 normally, different values on some platforms might be supported).
 */
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceSetPixelWidth(xdtusb_device_t* pdev, uint8_t bitsPerPixel);

/** Get pixel depth.
 *
 * Gets the current pixel depth set for readout.
 *
 * @param[in]  pdev Pointer to opened device.
 * @param[out] pBitsPerPixel Pointer to location to store current pixel width to.
 */
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceGetPixelWidth(xdtusb_device_t* pdev, uint8_t* pBitsPerPixel);


/** Stop streaming.
 *
 * Stop streaming on the given device.
 *
 * This will disable the reception of frames coming from the detector.
 *
 * @param[in] pdev Pointer to opened device.
 *
 */
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceStopStreaming(xdtusb_device_t* pdev);

/** Issue Software Trigger.
 *
 * Issues a SW trigger on the given device.
 * This will start a configured triggered Exposure Mode Program (such as \ref XDTUSB_DeviceConfigureExposureModeSequenceManual())
 * when the device is put into Streaming Mode (\ref XDTUSB_DeviceStartStreaming()).
 *
 * @param[in] pdev Pointer to opened device.
 *
 */
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceIssueSwTrigger(xdtusb_device_t* pdev);


/** @brief Get image dimensions stored in the frame buffer
 *
 * Returns the dimensions of the image currently stored in the frame buffer.
 *
 * @param[in] pFb Pointer to frame buffer.
 * @param[out] ppFd Location to the pointer of the current framebuffer dimensions to.
 */
API_EXPORT(xdtusb_error_t) XDTUSB_FramebufGetDimensions(xdtusb_framebuf_t* pfb, xdtusb_frame_dimensions_t** ppFd);

/** @brief Get pixel depth of image stored in the frame buffer
 *
 * Returns number of bits representing a pixel.
 *
 * @param[in] pFb Pointer to frame buffer.
 * @param[out] pBitsPerPixel Location to store the pixel depth to.
 */
API_EXPORT(xdtusb_error_t) XDTUSB_FramebufGetPixelWidth(xdtusb_framebuf_t* pfb, uint8_t* pBitsPerPixel);

/** @brief Get image data of a frame buffer
 *
 * Get the pointer to the frame buffer's image data.
 *
 * The pointer to the image data holds the location of the first pixel (left-most pixel of top image row).
 * The image rows are consecutively stored in memory, starting with the first (top) row .
 * Each image row is consecutively stored in memory, starting with the first (left-most) column.
 *
 * To retrieve the valid image dimensions, use \ref XDTUSB_FramebufGetImageInfo().
 *
 * For example to access the right-most bottom pixel of an image stored in a frame buffer:
 * \code
 * xdtusb_frame_dimensions_t frameDimensions;
 * xdtusb_pixel_t* pImagedata;
 * XDTUSB_FramebufGetImageInfo(pFrameBuffer, &frameDimensions);
 * XDTUSB_FramebufGetImage(pFrameBuffer, &pImagedata);
 *
 * xdtusb_pixel_t rightMostPixel = image[pImageInfo->height-1][pImageInfo->width-1];
 * \endcode
 *
 * @param[in] pFb Pointer to frame buffer.
 * @param[out] ppData Pointer to location to store the pointer to the image data to.
 *
 */
API_EXPORT(xdtusb_error_t) XDTUSB_FramebufGetData(xdtusb_framebuf_t* pfb, xdtusb_pixel_t** ppPixel);

/** @brief Poll number of filled frame buffers.
 *
 * Gets the number of frame buffers that are currently filled and exclusivy available to
 * the Application.
 *
 * @param[in] pDev Pointer to Device.
 * @param[out] pNumFilled Pointer to location to store the number of filled frame buffers to.
 *
 */
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceFramebufPollFilledCount(xdtusb_device_t* pdev, uint32_t* pNumFilled);

/** @brief Poll list of filled frame buffers.
 *
 * This allocates and assembles a NULL-terminated array list of currently filled frame buffers.
 *
 * The order of the frame buffers within the list correspond to the order of reception.
 *
 * \note The list must be free'd after processing (\see XDTUSB_DeviceFramebufFreeFilledList()).
 * \note The assembled list holds the filled frame buffers at the time of calling XDTUSB_DeviceFramebufPollFilledCount().
 *       Thus, when the application commits a buffer of the polled list (see XDTUSB_FramebufCommit()), the corresponding list entry gets invalid.
 *
 * @param[in] pDev Pointer to Device.
 * @param[out] pNumFilled Pointer to location to store the number of filled frame buffers to.
 * @param[out] ppFbList Location to store the pointer to the null-terminated list to.
 *
 */
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceFramebufPollFilledList(xdtusb_device_t* pdev, uint32_t* pNumFilled, xdtusb_framebuf_t*** ppFbList);

/** @brief Free list of filled frame buffers.
 *
 * This frees the filled frame buffer list, previously generated by \ref XDTUSB_DeviceFramebufPollFilledList().
 *
 * @param[in] pDev Pointer to Device.
 * @param[in] pFbList Pointer to the null-terminated list.
 *
 */
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceFramebufFreeFilledList(xdtusb_device_t* pdev, xdtusb_framebuf_t** pFbList);

/** @brief Get the first filled frame buffer.
 *
 * This gets the location of the first filled frame buffer.
 *
 * \note The first filled frame buffer will not change, until the frame buffer was committed (see \ref XDTUSB_FramebufCommit())
 *
 * @param[in] pDev Pointer to Device.
 * @param[in] ppFb Location to store the pointer to the first filled frame buffer to.
 *
 */
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceFramebufGetFirstFilled(xdtusb_device_t* pdev, xdtusb_framebuf_t** ppFb);

/** Commit a filled frame buffer.
 *
 * This commits a filled frame buffer (i.e. a frame buffer that contains image data
 * and the application currently has exclusive access to) back to the device, allowing
 * the device to store received data to the buffer.
 * After committing a frame buffer, the application looses its exclusive access to it,
 * thus should not access it anymore.
 *
 * @param[in] pFb Pointer to frame buffer to be committed.
 */
API_EXPORT(xdtusb_error_t) XDTUSB_FramebufCommit(xdtusb_framebuf_t* pFb);


/** Set detector sensor version
 *
 * Set sensor version on \c EIO detectors.
 * @param[in] pdev Pointer to device.
 * @param version (\c 1 for Slingshot M1 devices, \c 2 for Slingshot M2 devices, \c 4 for Slingshot M4 devices)
 */
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceSetSensorVersion(xdtusb_device_t* pdev, uint8_t version);

/** Get current throughput information
 *
 * Get throughput statistics information.
 *
 * \note The \c xdtusb_tpstats_t.num_drop value holds the number of dropped frames since the
 *       last call of \ref XDTUSB_DeviceGetTPStats(). The total number of drops need to
 *       be accumulated by the application.
 *       If the exact number of dropped frame is cruicial for the application, XDTUSB_DeviceGetTPStats() should
 *       be called on a regular basis in order to avoid clipping.
 *       The number of dropped frames is reset to 0 when the device is not in streaming mode.
 *
 * @param[in] pdev Pointer to device.
 * @param[out] ptpstats Pointer to xdtusb_tpstats_t to store the acquired information to.
 */
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceGetTPStats(xdtusb_device_t* pdev, xdtusb_tpstats_t* ptpstats);


/** Read an FPGA register.
 *
 * \note For internal use only.
 *
 * @param[in] pDev Pointer to Device.
 * @param     adr Register address.
 * @param[out] pVal Location to store the read register value to.
 */
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceFpgaRegisterRead(xdtusb_device_t* pdev, uint16_t adr, uint16_t *pVal);

/** Write an FPGA register.
 *
 * \note For internal use only.
 *
 * @param[in] pDev Pointer to Device.
 * @param     adr Register address.
 * @param     val Register value to write.
 */
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceFpgaRegisterWrite(xdtusb_device_t* pdev, uint16_t adr, uint16_t val);

/** Read-Modify-Write an FPGA register.
 *
 * \note For internal use only.
 *
 * Sets register bits MSK<<POS with the value VAL(MSK<<0), while keeping the values of all other bits.
 * E.g. to set bits [3:2] in register 10 to 1, one would call XDTUSB_DeviceFpgaRegisterReadModifyWrite(pdev, 10, 1, 2, 7);
 *
 * @param[in] pDev Pointer to Device.
 * @param     adr Register address.
 * @param     val Register value to write.
 */
API_EXPORT(xdtusb_error_t) XDTUSB_DeviceFpgaRegisterReadModifyWrite(xdtusb_device_t* pdev, uint16_t adr, uint16_t val, uint8_t pos, uint16_t msk);

#ifdef __cplusplus
}
#endif

#endif /* XDTUSB_H_ */
