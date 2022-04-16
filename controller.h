
#pragma once

//
// Public functions
//
EVT_WDF_DRIVER_DEVICE_ADD       ControllerWdfEvtDeviceAdd;

//
// Private functions
//
EVT_WDF_DEVICE_PREPARE_HARDWARE                 ControllerWdfEvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE                 ControllerWdfEvtDeviceReleaseHardware;
EVT_WDF_DEVICE_D0_ENTRY                         ControllerWdfEvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT                          ControllerWdfEvtDeviceD0Exit;
EVT_WDF_DEVICE_D0_ENTRY_POST_INTERRUPTS_ENABLED ControllerWdfEvtDeviceD0EntryPostInterruptsEnabled;
EVT_WDF_DEVICE_D0_EXIT_PRE_INTERRUPTS_DISABLED  ControllerWdfEvtDeviceD0ExitPreInterruptsDisabled;
EVT_WDF_OBJECT_CONTEXT_CLEANUP                  ControllerWdfEvtCleanupCallback;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL              ControllerEvtIoDeviceControl;

EVT_UDECX_WDF_DEVICE_QUERY_USB_CAPABILITY         ControllerEvtUdecxWdfDeviceQueryUsbCapability;

NTSTATUS
ControllerCreateWdfDeviceWithNameAndSymLink(
    _Inout_
        PWDFDEVICE_INIT * WdfDeviceInit,
    _In_
        PWDF_OBJECT_ATTRIBUTES WdfDeviceAttributes,
    _Out_
        WDFDEVICE * WdfDevice
    );


NTSTATUS
Misc_WdfDeviceAllocateUsbContext(
    _In_
    WDFDEVICE Object
);


typedef struct _REQUEST_CONTEXT {

} REQUEST_CONTEXT, *PREQUEST_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE(REQUEST_CONTEXT);

#define RESET_INPUT_REPORT_LENGTH 2

#define USB_HOST_DEVINTERFACE_REF_STRING L"GUID_DEVINTERFACE_USB_HOST_CONTROLLER"

#define MAX_SUFFIX_SIZE                         9*sizeof(WCHAR) // all ULONGs fit in 9 characters
#define BASE_DEVICE_NAME                        L"\\Device\\VCAM"
#define BASE_SYMBOLIC_LINK_NAME                 L"\\DosDevices\\VCAM"

#define DeviceNameSize                          sizeof(BASE_DEVICE_NAME)+MAX_SUFFIX_SIZE
#define SymLinkNameSize                         sizeof(BASE_SYMBOLIC_LINK_NAME)+MAX_SUFFIX_SIZE


#define IOCTL_VIDEO_DATA_PUSH \
	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x901, METHOD_IN_DIRECT, FILE_ANY_ACCESS)

#if 0
#define IOCTL_AUDIO_DATA_PUSH \
	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x902, METHOD_IN_DIRECT, FILE_ANY_ACCESS)

#define IOCTL_DEVICE_PLUGIN \
	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x903, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_DEVICE_PLUGOUT \
	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x904, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_SET_DEVICE_DESCRIPTOR \
	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x905, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_SET_CFG_DESCRIPTOR \
	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x906, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif
#define IOCTL_GET_UVC_CMD \
	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x907, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_SET_VIDEO_PROP \
	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x908, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_SET_AUDIO_PROP \
	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x909, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define VDEV_TYPE_CAM       0
#define VDEV_TYPE_MIC       1



VOID Control_EVT_WDF_FILE_CLEANUP(
	_In_
	WDFFILEOBJECT FileObject
);


