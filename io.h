

#pragma once

#define FOCUS_REL_MIN					1
#define FOCUS_REL_MAX					10
#define FOCUS_REL_DEFAULT				5

#define ZOOM_REL_MIN					1
#define ZOOM_REL_MAX					10
#define ZOOM_REL_DEFAULT				5

#define ZOOM_ABS_MIN					1
#define ZOOM_ABS_MAX					10
#define ZOOM_ABS_DEFAULT				5

#define PANTILT_REL_SPEED_MIN			1
#define PANTILT_REL_SPEED_MAX			10
#define PANTILT_REL_SPEED_DEFAULT		5


#define NDI_PTZ_PAN_TILT		0x01
#define NDI_PTZ_FOCUS			0x02
#define NDI_PTZ_ZOOM			0x03

#define NDI_PTZ_PAN_TILT_ABS		0x04
#define NDI_PTZ_FOCUS_ABS			0x05
#define NDI_PTZ_ZOOM_ABS			0x06

#define NDI_PTZ_FOCUS_AUTO			0x05

VOID CompleteURB(
	_In_ WDFREQUEST request,
	_In_ ULONG transferBufferLength,
	NTSTATUS status
	);



VOID DpcCompleteAudioIsoUrb(IN WDFDPC Dpc);
VOID DpcCompleteVidoeIsoUrb(IN WDFDPC Dpc);


NTSTATUS videoControl(
	_In_ PUSB_CONTEXT usbContext,
	_In_
	WDF_USB_CONTROL_SETUP_PACKET setupPacket,
	_In_
	PUCHAR transferBuffer,
	_Out_
	PULONG transferedLen
);


NTSTATUS ptzControl(
	_In_ PUSB_CONTEXT usbContext,
	_In_
	WDF_USB_CONTROL_SETUP_PACKET setupPacket,
	_In_
	PUCHAR transferBuffer,
	_Out_
	PULONG transferedLen

);

VOID EVT_IO_PTZ_QUEUE_IO_CANCELED_ON_QUEUE(
	_In_
	WDFQUEUE Queue,
	_In_
	WDFREQUEST Request
);

//
// Private functions
//
EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL IoEvtControlUrb;
EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL IoAudioEvtControlUrb;


EVT_WDF_TIMER EvtIsochTimer;

void init_ptz_ctrl_data(PUSB_CONTEXT usbContext);

#define MAX_LEN_OF_FULL_SPEED_ISO_TRANSFER_PER_FRAME		(255 * 1024)