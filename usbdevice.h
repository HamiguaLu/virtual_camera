/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

	usbdevice.h

Environment:

	Kernel-mode Driver Framework

--*/

#pragma once
#pragma pack(1)
typedef struct _DESCRIPTORH {
	int type;
	int len;
} VDEV_DESCRIPTOR_HDR;

typedef struct _DESCRIPTOR {
	int type;
	int len;
	UCHAR* data;
} VDEV_DESCRIPTOR;


typedef struct _PTZ_DATA
{
	UINT16 focus_abs_data[1];
	UINT8 focus_rel_data[2];
	UINT8 focus_auto_data[1];

	UINT16 zoom_abs_data[1];
	UINT8 zoom_rel_data[3];

	INT32 pantilt_abs_data[2];
	UINT8 pantilt_rel_data[4];
} PtzData;



typedef struct _FrameData
{
	int     bitsPerPixel;
	int     width;
	int     height;
	int     frame_length;
	int     frame_pos;
	int		frame_rate;
	int		iso_timer_interval;
	int		needChangeFormat;
	UCHAR   frame_flip;
	UCHAR	bFrameIndex;
	UCHAR	bFormatIndex;
	UCHAR*	frame_buf;


}FrameData;


#if 0
typedef struct _AudioData
{
	int     sample_rate;
	int     bitResolution;
	int     frame_length;
	int     frame_pos;
	UCHAR*	frame_buf;

}AudioData;


typedef struct audio_prop {
	int     sample_rate;
	int     bitResolution;
} AudioProp;

#endif



#pragma pack(1)
typedef struct _PTZ_Control {
	int controlType;
	int v1;
	int v2;
	int v3;
	int v4;
} PTZ_CTRL_Data;



typedef struct video_prop {
	int     width;
	int     height;
	int     bitsPerPixel;
	int		frameRate;//30/60/120
	UCHAR	bFrameIndex;
	UCHAR	bFormatIndex;
} VideoProp;


#pragma pack()


typedef struct _ptz_list {
	PTZ_CTRL_Data		ctrl;
	LIST_ENTRY			ListEntry;
} PTZ_CTRL_LIST_ITEM;



typedef struct _USB_CONTEXT {
	//virtual camera device
	PUDECXUSBDEVICE_INIT    UdecxCamInit;
	UDECXUSBDEVICE          VCamDevice;
	VDEV_DESCRIPTOR         VCamDeviceDescriptor;
	VDEV_DESCRIPTOR         VCamConfigDescriptorSet;
	UDECXUSBENDPOINT        ControlEndpoint;
	WDFQUEUE                ControlQueue;
	WDFQUEUE                VideoQueue;
	//WDFQUEUE                PTZCtrlQueue;
	FrameData               frameData;
	PtzData					ptzData;
	//PTZ_CTRL_LIST			ptzCmdList;
	LIST_ENTRY				ptzListHdr;

	UDECXUSBENDPOINT        IsoTransferVideoEndpoint;
	WDFQUEUE                IsoVideoInQueue;

	UDECXUSBENDPOINT        InterruptEndpoint;
	WDFQUEUE                InterruptInQueue;

	WDFREQUEST              currentRequest;
	WDFREQUEST              currentVideoRequest;

	WDFDPC                  completeUrbDpc;

#if 0
	//virtual mic device
	PUDECXUSBDEVICE_INIT    UdecxMicInit;
	UDECXUSBDEVICE          VMicDevice;
	VDEV_DESCRIPTOR         VMicDeviceDescriptor;
	VDEV_DESCRIPTOR         VMicConfigDescriptorSet;
	WDFDPC                  completeAudioUrbDpc;
	//UDECXUSBENDPOINT        ControlEndpoint;
	//UDECXUSBENDPOINT        IsoTransferAudioEndpoint;
	WDFQUEUE                AudioQueue;
	WDFQUEUE                IsoAudioInQueue;
	AudioData				audioData;
	WDFREQUEST              currentAudioRequest;
#endif

	WDFQUEUE                DefaultQueue;

	WDFTIMER                isoTransferTimer;

} USB_CONTEXT, * PUSB_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(USB_CONTEXT, WdfDeviceGetUsbContext);


typedef struct _UDECX_USBDEVICE_CONTEXT {

	WDFDEVICE   WdfDevice;
} UDECX_USBDEVICE_CONTEXT, * PUDECX_USBDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(UDECX_USBDEVICE_CONTEXT, UdecxDeviceGetContext);


NTSTATUS
Usb_Initialize(
	_In_
	WDFDEVICE WdfDevice
	);

NTSTATUS
Usb_ReadDescriptorsAndPlugIn(
	_In_
	WDFDEVICE WdfDevice
	);

NTSTATUS
Usb_Disconnect(
	_In_
	WDFDEVICE WdfDevice
	);

VOID
Usb_Destroy(
	_In_
	WDFDEVICE WdfDevice
	);



EVT_UDECX_USB_DEVICE_ENDPOINTS_CONFIGURE      UsbDevice_EvtUsbDeviceEndpointsConfigure;

EVT_UDECX_USB_DEVICE_D0_ENTRY                         UsbDevice_EvtUsbDeviceLinkPowerEntry;
EVT_UDECX_USB_DEVICE_D0_EXIT                          UsbDevice_EvtUsbDeviceLinkPowerExit;
EVT_UDECX_USB_DEVICE_SET_FUNCTION_SUSPEND_AND_WAKE    UsbDevice_EvtUsbDeviceSetFunctionSuspendAndWake;

EVT_UDECX_USB_ENDPOINT_RESET UsbEndpointReset;
NTSTATUS
EvtUdecxUsbDeviceControlEndpointAdd(
	_In_
	UDECXUSBDEVICE            UdecxUsbDevice,
	_In_
	PUDECXUSBENDPOINT_INIT    UdecxUsbEndpointInit
	);

FORCEINLINE
VOID
UsbValidateConstants(
	)
{

	NT_ASSERT(((PUSB_STRING_DESCRIPTOR)g_LanguageDescriptor)->bString[0] == AMERICAN_ENGLISH);
	NT_ASSERT(((PUSB_STRING_DESCRIPTOR)g_LanguageDescriptor)->bLength == sizeof(g_LanguageDescriptor));
	NT_ASSERT(((PUSB_STRING_DESCRIPTOR)g_LanguageDescriptor)->bDescriptorType == USB_STRING_DESCRIPTOR_TYPE);

}


#include "uvc_struct.h"

#define EP_ADDR    0x82 //用于视频流传输的端口
#define VID_HDR_LEN    2




void init_cam_prop(PUSB_CONTEXT usbContext);


#if 0

//////////////////////////////////////////////////////////////////
//audio implementation

NTSTATUS Usb_Audio_Initialize( 	_In_ 	WDFDEVICE WdfDevice);
NTSTATUS Usb_Audio_ReadDescriptorsAndPlugIn(_In_ WDFDEVICE WdfDevice);

NTSTATUS Usb_Audio_Disconnect(_In_ 	WDFDEVICE WdfDevice);

VOID Usb_Audio_Destroy(	_In_	WDFDEVICE WdfDevice);

VOID Usb_Audio_UdecxUsbEndpointEvtReset(
	_In_ UCXCONTROLLER,
	_In_ UCXENDPOINT,
	_In_ WDFREQUEST
);


VOID UsbAudioEndpointReset(
	_In_
	UDECXUSBENDPOINT UdecxUsbEndpoint,
	_In_
	WDFREQUEST     Request
);


NTSTATUS UsbAudioDevice_EvtUsbDeviceLinkPowerEntry(
	_In_
	WDFDEVICE       UdecxWdfDevice,
	_In_
	UDECXUSBDEVICE    UdecxUsbDevice
);

NTSTATUS UsbAudioDevice_EvtUsbDeviceLinkPowerExit(
	_In_
	WDFDEVICE                   UdecxWdfDevice,
	_In_
	UDECXUSBDEVICE                UdecxUsbDevice,
	_In_
	UDECX_USB_DEVICE_WAKE_SETTING WakeSetting
);

NTSTATUS UsbAudioDevice_EvtUsbDeviceSetFunctionSuspendAndWake(
	_In_
	WDFDEVICE                      UdecxWdfDevice,
	_In_
	UDECXUSBDEVICE                   UdecxUsbDevice,
	_In_
	ULONG                          Interface,
	_In_
	UDECX_USB_DEVICE_FUNCTION_POWER  FunctionPower
);


void fill_mic_cfg_desc_video(PUSB_CONTEXT deviceContext);
void fill_mic_dev_desc_video(PUSB_CONTEXT deviceContext);
void init_mic_prop(PUSB_CONTEXT usbContext);

#endif