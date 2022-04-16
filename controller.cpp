

#include "pch.h"

#include "controller.h"

NTSTATUS
Misc_WdfDeviceAllocateUsbContext(
	_In_
	WDFDEVICE Object
	)
	/*++

	Routine Description:

		Object context allocation helper

	Arguments:

		Object - WDF object upon which to allocate the new context

	Return value:

		NTSTATUS. Could fail on allocation failure or the same context type already exists on the object

	--*/
{
	NTSTATUS status;
	WDF_OBJECT_ATTRIBUTES attributes;

	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, USB_CONTEXT);

	status = WdfObjectAllocateContext(Object, &attributes, NULL);

	if (!NT_SUCCESS(status)) {

		DbgPrint("Unable to allocate new context for WDF object %p", Object);
		goto exit;
	}

exit:

	return status;
}

DEFINE_GUID(GUID_DEVINTERFACE_VDEV, 0x8a7d7abf, 0xde9b, 0x4490, 0x8e, 0xc1, 0x41, 0xa7, 0x3, 0x8d, 0x73, 0x37);

NTSTATUS
ControllerWdfEvtDeviceAdd(
	_In_
	WDFDRIVER Driver,
	_Inout_
	PWDFDEVICE_INIT WdfDeviceInit
	)
{
	NTSTATUS                            status;
	WDFDEVICE                           wdfDevice;
	WDF_PNPPOWER_EVENT_CALLBACKS        wdfPnpPowerCallbacks;
	WDF_OBJECT_ATTRIBUTES               wdfDeviceAttributes;
	WDF_OBJECT_ATTRIBUTES               wdfRequestAttributes;
	UDECX_WDF_DEVICE_CONFIG               controllerConfig;
	WDF_FILEOBJECT_CONFIG               fileConfig;
	PUSB_CONTEXT	                   usbContext;
	WDF_IO_QUEUE_CONFIG                 defaultQueueConfig, manualQueueConfig;
	WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS   	idleSettings;
	UNICODE_STRING                      refString;


	UNREFERENCED_PARAMETER(Driver);
	////FuncEntry(TRACE_FLAG_Driver);

	WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&wdfPnpPowerCallbacks);
	wdfPnpPowerCallbacks.EvtDevicePrepareHardware = ControllerWdfEvtDevicePrepareHardware;
	wdfPnpPowerCallbacks.EvtDeviceReleaseHardware = ControllerWdfEvtDeviceReleaseHardware;
	wdfPnpPowerCallbacks.EvtDeviceD0Entry = ControllerWdfEvtDeviceD0Entry;
	wdfPnpPowerCallbacks.EvtDeviceD0Exit = ControllerWdfEvtDeviceD0Exit;
	//wdfPnpPowerCallbacks.EvtDeviceD0EntryPostInterruptsEnabled = ControllerWdfEvtDeviceD0EntryPostInterruptsEnabled;
	//wdfPnpPowerCallbacks.EvtDeviceD0ExitPreInterruptsDisabled = ControllerWdfEvtDeviceD0ExitPreInterruptsDisabled;

	WdfDeviceInitSetPnpPowerEventCallbacks(WdfDeviceInit, &wdfPnpPowerCallbacks);

	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&wdfRequestAttributes, REQUEST_CONTEXT);
	WdfDeviceInitSetRequestAttributes(WdfDeviceInit, &wdfRequestAttributes);

	//
	// To distinguish I/O sent to GUID_DEVINTERFACE_USB_HOST_CONTROLLER, we will enable
	// interface reference strings. This requires calling WdfDeviceInitSetFileObjectConfig
	// with FileObjectClass WdfFileObjectWdfXxx.
	WDF_FILEOBJECT_CONFIG_INIT(&fileConfig,
		WDF_NO_EVENT_CALLBACK,
		WDF_NO_EVENT_CALLBACK,
		Control_EVT_WDF_FILE_CLEANUP // No cleanup callback function
		);

	//
	// Safest value forces WDF to track handles separately. If the driver stack allows it, then
	// for performance, we should change this to a different option.
	//
	fileConfig.FileObjectClass = WdfFileObjectWdfCannotUseFsContexts;

	WdfDeviceInitSetFileObjectConfig(WdfDeviceInit, &fileConfig, WDF_NO_OBJECT_ATTRIBUTES);

	//
	// Set the security descriptor for the device.
	//
	status = WdfDeviceInitAssignSDDLString(WdfDeviceInit, &SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RW_RES_R);

	if (!NT_SUCCESS(status)) {

		DbgPrint("WdfDeviceInitAssignSDDLString Failed %d", status);
		goto exit;
	}

	//
	// Do additional setup required for USB controllers.
	//
	status = UdecxInitializeWdfDeviceInit(WdfDeviceInit);
	if (!NT_SUCCESS(status)) {

		DbgPrint("UdecxInitializeDeviceInit failed %d", status);
		goto exit;
	}

	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&wdfDeviceAttributes, USB_CONTEXT);
	wdfDeviceAttributes.EvtCleanupCallback = ControllerWdfEvtCleanupCallback;

	//
	// Call WdfDeviceCreate with a few extra compatibility steps to ensure this device looks
	// exactly like other USB host controllers.
	//
	status = ControllerCreateWdfDeviceWithNameAndSymLink(&WdfDeviceInit,
		&wdfDeviceAttributes,
		&wdfDevice);

	if (!NT_SUCCESS(status)) {

		goto exit;
	}

	//
	// Create the device interface.
	//
	RtlInitUnicodeString(&refString, USB_HOST_DEVINTERFACE_REF_STRING);

	status = WdfDeviceCreateDeviceInterface(wdfDevice,
		(LPGUID)&GUID_DEVINTERFACE_VDEV,
		&refString);

	if (!NT_SUCCESS(status)) {
		DbgPrint("WdfDeviceCreateDeviceInterface Failed %d", status);
		goto exit;
	}

	UDECX_WDF_DEVICE_CONFIG_INIT(&controllerConfig, ControllerEvtUdecxWdfDeviceQueryUsbCapability);

	status = UdecxWdfDeviceAddUsbDeviceEmulation(wdfDevice, &controllerConfig);

	//
	// Initialize controller data members.
	// TODO: reset using UCX?
	//
	usbContext = WdfDeviceGetUsbContext(wdfDevice);

	status = Misc_WdfDeviceAllocateUsbContext(wdfDevice);
	if (!NT_SUCCESS(status)) {
		DbgPrint("UsbContext creation failed %d", status);
		goto exit;
	}


	//
	// Create default queue. It only supports USB controller IOCTLs. (USB I/O will come through
	// in separate USB device queues.)
	//
	WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&defaultQueueConfig, WdfIoQueueDispatchParallel);
	defaultQueueConfig.EvtIoDeviceControl = ControllerEvtIoDeviceControl;
	defaultQueueConfig.PowerManaged = WdfFalse;

	status = WdfIoQueueCreate(wdfDevice, &defaultQueueConfig, WDF_NO_OBJECT_ATTRIBUTES, &usbContext->DefaultQueue);
	if (!NT_SUCCESS(status)) {
		DbgPrint("Default queue creation failed %d", status);
		goto exit;
	}

	WDF_IO_QUEUE_CONFIG_INIT(&manualQueueConfig, WdfIoQueueDispatchManual);
	manualQueueConfig.PowerManaged = WdfFalse;
	status = WdfIoQueueCreate(wdfDevice, &manualQueueConfig, WDF_NO_OBJECT_ATTRIBUTES, &usbContext->VideoQueue);
	if (!NT_SUCCESS(status)) {
		DbgPrint("Default queue creation failed %d", status);
		goto exit;
	}

#if 0
	WDF_IO_QUEUE_CONFIG_INIT(&manualQueueConfig, WdfIoQueueDispatchManual);
	manualQueueConfig.PowerManaged = WdfFalse;
	//manualQueueConfig.EvtIoCanceledOnQueue = EVT_IO_PTZ_QUEUE_IO_CANCELED_ON_QUEUE;
	status = WdfIoQueueCreate(wdfDevice, &manualQueueConfig, WDF_NO_OBJECT_ATTRIBUTES, &usbContext->PTZCtrlQueue);
	if (!NT_SUCCESS(status)) {
		DbgPrint("Default queue creation failed %d", status);
		goto exit;
	}

	WDF_IO_QUEUE_CONFIG_INIT(&manualQueueConfig, WdfIoQueueDispatchManual);
	manualQueueConfig.PowerManaged = WdfFalse;
	status = WdfIoQueueCreate(wdfDevice, &manualQueueConfig, WDF_NO_OBJECT_ATTRIBUTES, &usbContext->AudioQueue);
	if (!NT_SUCCESS(status)) {
		DbgPrint("Default queue creation failed %d", status);
		goto exit;
	}
#endif


	InitializeListHead(&usbContext->ptzListHdr);

	init_cam_prop(usbContext);
	init_ptz_ctrl_data(usbContext);

	//
	// Initialize virtual USB device software objects.
	//

	status = Usb_Initialize(wdfDevice);
	if (!NT_SUCCESS(status)) {

		goto exit;
	}

	//
	// Setup the S0 Idle settings just so that we get registered with power
	// framework as some SOC platforms depend on it. 
	//
	WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(&idleSettings, IdleCannotWakeFromS0); // TODO:can

	//idleSettings.IdleTimeoutType = SystemManagedIdleTimeoutWithHint;
	//idleSettings.Enabled = WdfFalse;

	if (!NT_SUCCESS(status)) {

		DbgPrint("WdfDeviceAssignS0IdleSettings failed %d", status);
		goto exit;
	}



exit:

	return status;
}

NTSTATUS
ControllerCreateWdfDeviceWithNameAndSymLink(
	_Inout_
	PWDFDEVICE_INIT* WdfDeviceInit,
	_In_
	PWDF_OBJECT_ATTRIBUTES WdfDeviceAttributes,
	_Out_
	WDFDEVICE* WdfDevice
	)
	/*++

	Routine Description:

		Create the WDFDEVICE with a few extra compatibility steps, to ensure this device looks exactly
		like other USB host controllers. This function should be called from EvtDriverDeviceAdd.

		The extra compatibility steps are:

		1) Assign a USB host controller FDO name to the device.
		2) Create a USB host controller symbolic link to the device.

	Arguments:

		WdfDeviceInit - Initialization parameters for the new device. This function does not modify the
						parameters, but sets the pointer to NULL to indicate the structure has been
						used up in creating the device.

		WdfDeviceAttributes - Attributes of the device (cleanup callback, context type, etc.)

		WdfDevice - The created WdfDevice. Note, when failure is returned this may be NULL or non-NULL.
					Non-NULL indicates the function encountered a failure after successfully calling
					WdfDeviceCreate. WDF will delete the WdfDevice itself if EvtDriverDeviceAdd returns
					a failure.

	Return Value:

		NTSTATUS

	--*/
{
	NTSTATUS status;
	ULONG instanceNumber;
	BOOLEAN isCreated;

	DECLARE_UNICODE_STRING_SIZE(uniDeviceName, DeviceNameSize);
	DECLARE_UNICODE_STRING_SIZE(uniSymLinkName, SymLinkNameSize);
	////FuncEntry(TRACE_FLAG_Driver);

	*WdfDevice = NULL;

	//
	// Generate a unique static device name in order to provide compatibility to look like with
	// existing USB host controller driver implementations.
	//
	isCreated = FALSE;

	for (instanceNumber = 0; instanceNumber < ULONG_MAX; instanceNumber++) {

		status = RtlUnicodeStringPrintf(&uniDeviceName,
			L"%ws%d",
			BASE_DEVICE_NAME,
			instanceNumber);

		if (!NT_SUCCESS(status)) {

			DbgPrint("RtlUnicodeStringPrintf (uniDeviceName) failed %d\n", status);
			goto exit;
		}

		status = WdfDeviceInitAssignName(*WdfDeviceInit, &uniDeviceName);

		if (!NT_SUCCESS(status)) {

			DbgPrint("WdfDeviceInitAssignName Failed %d", status);
			goto exit;
		}

		status = WdfDeviceCreate(WdfDeviceInit, WdfDeviceAttributes, WdfDevice);

		if (status == STATUS_OBJECT_NAME_COLLISION) {

			//
			// This is expected to happen at least once when another USB host controller
			// already exists on the system.
			//
			DbgPrint("WdfDeviceCreate Object Name Collision %d", instanceNumber);

		}
		else if (!NT_SUCCESS(status)) {

			DbgPrint("WdfDeviceCreate Failed %u\n", status);
			goto exit;

		}
		else {

			isCreated = TRUE;
			break;
		}

	}

	if (!isCreated) {

		status = STATUS_OBJECT_NAME_COLLISION;
		DbgPrint("All instance numbers of USB host controller are already used %d\n", status);
		goto exit;
	}


	//
	// Create the symbolic link (also for compatibility).
	//
	status = RtlUnicodeStringPrintf(&uniSymLinkName, L"%ws%d", BASE_SYMBOLIC_LINK_NAME, instanceNumber);

	if (!NT_SUCCESS(status)) {

		DbgPrint("RtlUnicodeStringPrintf (SymLinkName) Failed %d", status);
		goto exit;
	}

	status = WdfDeviceCreateSymbolicLink(*WdfDevice, &uniSymLinkName);

	if (!NT_SUCCESS(status)) {

		DbgPrint("WdfDeviceCreateSymbolicLink Failed %d", status);
		goto exit;
	}

exit:
	////FuncExit(TRACE_FLAG_Driver, status);
	return status;
}

NTSTATUS
ControllerWdfEvtDevicePrepareHardware(
	_In_
	WDFDEVICE       WdfDevice,
	_In_
	WDFCMRESLIST    WdfResourcesRaw,
	_In_
	WDFCMRESLIST    WdfResourcesTranslated
	)
{
	////FuncEntry(TRACE_FLAG_Driver);
	UNREFERENCED_PARAMETER(WdfDevice);
	UNREFERENCED_PARAMETER(WdfResourcesRaw);
	UNREFERENCED_PARAMETER(WdfResourcesTranslated);
	PRINTLINE("Enter");
	return STATUS_SUCCESS;
}

NTSTATUS
ControllerWdfEvtDeviceD0Entry(
	_In_
	WDFDEVICE              WdfDevice,
	_In_
	WDF_POWER_DEVICE_STATE PreviousState
	)
{
	NTSTATUS status = STATUS_SUCCESS;
	UNREFERENCED_PARAMETER(WdfDevice);

	PRINTLINE("Enter");

	//pControllerContext = WdfDeviceGetContext(WdfDevice);

	if (PreviousState == WdfPowerDeviceD3Final) {

		//NT_ASSERT(!pControllerContext->AllowOnlyResetInterrupts);
		//pControllerContext->AllowOnlyResetInterrupts = TRUE;

		status = Usb_ReadDescriptorsAndPlugIn(WdfDevice);

		if (!NT_SUCCESS(status)) {

			goto exit;
		}
	}

exit:
	//FuncExit(TRACE_FLAG_Driver, status);
	return status;
}

#if 0
NTSTATUS
ControllerWdfEvtDeviceD0EntryPostInterruptsEnabled(
	_In_
	WDFDEVICE              WdfDevice,
	_In_
	WDF_POWER_DEVICE_STATE PreviousState
	)
{
	//FuncEntry(TRACE_FLAG_Driver);
	UNREFERENCED_PARAMETER(WdfDevice);
	UNREFERENCED_PARAMETER(PreviousState);
	DbgPrint("ControllerWdfEvtDeviceD0EntryPostInterruptsEnabled\n");
	return STATUS_SUCCESS;
}

NTSTATUS
ControllerWdfEvtDeviceD0ExitPreInterruptsDisabled(
	_In_
	WDFDEVICE              WdfDevice,
	_In_
	WDF_POWER_DEVICE_STATE TargetState
	)
{
	//FuncEntry(TRACE_FLAG_Driver);
	UNREFERENCED_PARAMETER(WdfDevice);
	UNREFERENCED_PARAMETER(TargetState);
	DbgPrint("ControllerWdfEvtDeviceD0ExitPreInterruptsDisabled\n");
	return STATUS_SUCCESS;
}
#endif

NTSTATUS
ControllerWdfEvtDeviceD0Exit(
	_In_
	WDFDEVICE              WdfDevice,
	_In_
	WDF_POWER_DEVICE_STATE TargetState
	)
{
	PRINTLINE("Enter");

	//PUSB_CONTEXT usbContext;

	if (TargetState == WdfPowerDeviceD3Final) {

		Usb_Disconnect(WdfDevice);
	}

	//FuncExit(TRACE_FLAG_Driver, 0);
	return STATUS_SUCCESS;
}

NTSTATUS
ControllerWdfEvtDeviceReleaseHardware(
	_In_
	WDFDEVICE       WdfDevice,
	_In_
	WDFCMRESLIST    WdfResourcesTranslated
	)
{
	//FuncEntry(TRACE_FLAG_Driver);
	UNREFERENCED_PARAMETER(WdfDevice);
	UNREFERENCED_PARAMETER(WdfResourcesTranslated);
	PRINTLINE("Enter");
	return STATUS_SUCCESS;
}

VOID
ControllerWdfEvtCleanupCallback(
	_In_
	WDFOBJECT   WdfDevice
	)
{
	//PIO_CONTEXT ioContext;
	PRINTLINE("Enter");

	//ioContext = WdfDeviceGetIoContext((WDFDEVICE)WdfDevice);

	PUSB_CONTEXT usbContext;

	usbContext = WdfDeviceGetUsbContext((WDFDEVICE)WdfDevice);

	
	WdfTimerStop(usbContext->isoTransferTimer,true);

	//WdfSpinLockAcquire(ioContext->InProgressLock);
	//MbbCleanupBufferQueue(&ioContext->AvailableInterrupt);
   // MbbCleanupBufferQueue(&ioContext->EncapsulatedCommandResponse);
	//WdfSpinLockRelease(ioContext->InProgressLock);

	Usb_Destroy((WDFDEVICE)WdfDevice);
	//FuncExit(TRACE_FLAG_Driver, 0);
}



NTSTATUS
ControllerEvtUdecxWdfDeviceQueryUsbCapability(
	_In_
	WDFDEVICE     UdecxWdfDevice,
	_In_
	PGUID         CapabilityType,
	_In_
	ULONG         OutputBufferLength,
	_Out_writes_to_opt_(OutputBufferLength, *ResultLength)
	PVOID         OutputBuffer,
	_Out_
	PULONG        ResultLength
	)
{
	UNREFERENCED_PARAMETER(UdecxWdfDevice);
	UNREFERENCED_PARAMETER(CapabilityType);
	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(OutputBuffer);
	UNREFERENCED_PARAMETER(ResultLength);
	PRINTLINE("Enter");

	return STATUS_UNSUCCESSFUL;
}


void ControlClearContextData(PUSB_CONTEXT usbContext)
{
	UNREFERENCED_PARAMETER(usbContext);
}


VOID ControllerEvtIoDeviceControl(
	_In_
	WDFQUEUE Queue,
	_In_
	WDFREQUEST Request,
	_In_
	size_t OutputBufferLength,
	_In_
	size_t InputBufferLength,
	_In_
	ULONG IoControlCode
	)
{
	BOOLEAN handled;
	NTSTATUS status;
	PVOID inBuffer, outBuffer;
	WDFDEVICE wdfDevice;
	PUSB_CONTEXT usbContext;

	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(InputBufferLength);

	wdfDevice = WdfIoQueueGetDevice(Queue);
	usbContext = WdfDeviceGetUsbContext(wdfDevice);

	if (IoControlCode == IOCTL_VIDEO_DATA_PUSH)
	{
		//PRINTLINE("IOCTL_VIDEO_DATA_PUSH");
		status = WdfRequestForwardToIoQueue(Request, usbContext->VideoQueue);
		if (NT_SUCCESS(status)) {
			//complete I/O later on
			return;
		}
		PRINTLINE("IOCTL_VIDEO_DATA_PUSH failed");
		WdfRequestComplete(Request, status);
		return;
	}

	handled = UdecxWdfDeviceTryHandleUserIoctl(WdfIoQueueGetDevice(Queue), Request);
	if (handled) {
		return;
	}

	status = WdfRequestRetrieveInputBuffer(Request, 1, &inBuffer, NULL);
	if (!NT_SUCCESS(status)) {
		PRINTLINE("Get input buffer failed");
		DbgPrint("WdfRequestRetrieveInputBuffer:%x\n", status);
		WdfRequestComplete(Request, status);
		return;
	}

	status = WdfRequestRetrieveOutputBuffer(Request, 1, &outBuffer, NULL);
	if (!NT_SUCCESS(status)) {
		DbgPrint("WdfRequestRetrieveInputBuffer:%x\n", status);
		PRINTLINE("Get Output buffer failed");
		WdfRequestComplete(Request, status);
		return;
	}

	switch (IoControlCode)
	{
#if 0
	case IOCTL_DEVICE_PLUGIN:
	{
		PRINTLINE("IOCTL_DEVICE_PLUGIN");
		//start timer here

		if (InputBufferLength != sizeof(INT32))
		{
			status = STATUS_INVALID_PARAMETER;
			PRINTLINE("STATUS_INVALID_PARAMETER\n");
			break;
		}

		ControlClearContextData(usbContext);

		break;
	}
	case IOCTL_DEVICE_PLUGOUT:
	{
		PRINTLINE("IOCTL_DEVICE_PLUGOUT");
		if (InputBufferLength != sizeof(INT32))
		{
			status = STATUS_INVALID_PARAMETER;
		}

		ControlClearContextData(usbContext);

		break;
	}

	case IOCTL_AUDIO_DATA_PUSH:
	{
		PRINTLINE("IOCTL_AUDIO_DATA_PUSH");
		status = WdfRequestForwardToIoQueue(Request, usbContext->AudioQueue);
		if (NT_SUCCESS(status)) {
			//complete I/O later on
			return;
		}
		break;
	}
	case IOCTL_SET_DEVICE_DESCRIPTOR:
	{
		PRINTLINE("IOCTL_SET_DEVICE_DESCRIPTOR");
		if (InputBufferLength <= sizeof(VDEV_DESCRIPTOR))
		{
			status = STATUS_INVALID_BUFFER_SIZE;
			break;
		}

		VDEV_DESCRIPTOR_HDR* p = (VDEV_DESCRIPTOR_HDR*)inBuffer;
		if (p->type == VDEV_TYPE_CAM)
		{
			usbContext->VCamDeviceDescriptor.len = p->len;
			usbContext->VCamDeviceDescriptor.data = (UCHAR*)ExAllocatePoolWithTag(NonPagedPoolNx, p->len, UDECXVDEV_POOL_TAG);
			if (usbContext->VCamDeviceDescriptor.data == NULL)
			{
				usbContext->VCamDeviceDescriptor.len = 0;
			}
			else
			{
				UCHAR* buf = (UCHAR*)p;
				buf += sizeof(VDEV_DESCRIPTOR_HDR);
				RtlCopyMemory(usbContext->VCamDeviceDescriptor.data, buf, p->len);
			}
			DbgPrint("dev des len:%d\n", usbContext->VCamDeviceDescriptor.len);
		}
		else
		{
			usbContext->VMicDeviceDescriptor.len = p->len;
			usbContext->VMicDeviceDescriptor.data = (UCHAR*)ExAllocatePoolWithTag(NonPagedPoolNx, p->len, UDECXVDEV_POOL_TAG);
			if (usbContext->VMicDeviceDescriptor.data == NULL)
			{
				usbContext->VMicDeviceDescriptor.len = 0;
			}
			else
			{
				UCHAR* buf = (UCHAR*)p;
				buf += sizeof(VDEV_DESCRIPTOR_HDR);
				RtlCopyMemory(usbContext->VMicDeviceDescriptor.data, buf, p->len);
			}
		}

		break;
	}
	case IOCTL_SET_CFG_DESCRIPTOR:
	{
		PRINTLINE("IOCTL_SET_CFG_DESCRIPTOR");
		if (InputBufferLength <= sizeof(VDEV_DESCRIPTOR))
		{
			status = STATUS_INVALID_BUFFER_SIZE;
			break;
		}

		VDEV_DESCRIPTOR_HDR* p = (VDEV_DESCRIPTOR_HDR*)inBuffer;
		if (p->type == VDEV_TYPE_CAM)
		{
			usbContext->VCamConfigDescriptorSet.len = p->len;
			usbContext->VCamConfigDescriptorSet.data = (UCHAR*)ExAllocatePoolWithTag(NonPagedPoolNx, p->len, UDECXVDEV_POOL_TAG);
			if (usbContext->VCamConfigDescriptorSet.data == NULL)
			{
				usbContext->VCamConfigDescriptorSet.len = 0;
			}
			else
			{
				UCHAR* buf = (UCHAR*)p;
				buf += sizeof(VDEV_DESCRIPTOR_HDR);
				RtlCopyMemory(usbContext->VCamConfigDescriptorSet.data, buf, p->len);
			}

			DbgPrint("cfg des len:%d\n", usbContext->VCamConfigDescriptorSet.len);
		}
		else
		{
			usbContext->VMicConfigDescriptorSet.len = p->len;
			usbContext->VMicConfigDescriptorSet.data = (UCHAR*)ExAllocatePoolWithTag(NonPagedPoolNx, p->len, UDECXVDEV_POOL_TAG);
			if (usbContext->VMicConfigDescriptorSet.data == NULL)
			{
				usbContext->VMicConfigDescriptorSet.len = 0;
			}
			else
			{
				UCHAR* buf = (UCHAR*)p;
				buf += sizeof(VDEV_DESCRIPTOR_HDR);
				RtlCopyMemory(usbContext->VMicConfigDescriptorSet.data, buf, p->len);
			}
		}

		break;
	}
#endif
	case IOCTL_GET_UVC_CMD:
	{
		//PRINTLINE("IOCTL_GET_UVC_CMD");
		if (IsListEmpty(&usbContext->ptzListHdr))
		{
			status = STATUS_NO_MORE_MATCHES;
			break;
		}

		if (OutputBufferLength != sizeof(PTZ_CTRL_Data))
		{
			//DbgPrint("IOCTL_GET_UVC_CMD:OutputBufferLength:%dd != sizeof(PTZ_CTRL_Data):%d\n", OutputBufferLength , sizeof(PTZ_CTRL_Data));
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		PLIST_ENTRY pEntry = RemoveTailList(&usbContext->ptzListHdr);
		PTZ_CTRL_LIST_ITEM* pData = CONTAINING_RECORD(pEntry, PTZ_CTRL_LIST_ITEM, ListEntry);
		RtlCopyMemory(outBuffer, &pData->ctrl, sizeof(PTZ_CTRL_Data));

		WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, OutputBufferLength);
		//PRINTLINE("Notify app done");

		ExFreePool(pData);

		return;
	}
	case IOCTL_SET_VIDEO_PROP:
	{
		PRINTLINE("IOCTL_SET_VIDEO_PROP");
		if (InputBufferLength != sizeof(VideoProp))
		{
			//DbgPrint("IOCTL_SET_VIDEO_PROP failed:buf len:%d, %d required\n", InputBufferLength, sizeof(VideoProp));
			status = STATUS_INVALID_BUFFER_SIZE;
			break;
		}

		VideoProp* p = (VideoProp*)inBuffer;
		if (p->width > 4096 || p->height > 2160)
		{
			PRINTLINE("Only support res up to 4K");
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		
		if (p->frameRate > 120 || p->frameRate < 0)
		{
			p->frameRate = 30;
		}

		usbContext->frameData.width = p->width;
		usbContext->frameData.height = p->height;
		usbContext->frameData.bitsPerPixel = p->bitsPerPixel;
		usbContext->frameData.frame_rate = p->frameRate;
		usbContext->frameData.bFormatIndex = p->bFormatIndex;
		usbContext->frameData.bFrameIndex = p->bFrameIndex;
		usbContext->frameData.frame_flip = 0;
		usbContext->frameData.frame_length = usbContext->frameData.width * usbContext->frameData.height * (p->bitsPerPixel / 8); ;
		usbContext->frameData.frame_pos = 0;

		usbContext->frameData.needChangeFormat = 1;
		
		//A time period, in system time units (100-nanosecond intervals)
		//If the value is negative, the time period is relative to the current system time
		usbContext->frameData.iso_timer_interval = (usbContext->frameData.frame_length * usbContext->frameData.frame_rate) / MAX_LEN_OF_FULL_SPEED_ISO_TRANSFER_PER_FRAME;
		usbContext->frameData.iso_timer_interval /= 100 * (-1); //A time period, in system time units (100-nanosecond intervals)

		DbgPrint("Video Prop:x:%d,y:%d,bitsPerPixel:%d,len:%d, bFrameIndex:%d,frame_rate:%d,timer_interval:%d\n", 
			usbContext->frameData.width,
			usbContext->frameData.height,
			usbContext->frameData.bitsPerPixel,
			usbContext->frameData.frame_length,
			usbContext->frameData.bFrameIndex,
			usbContext->frameData.frame_rate,
			usbContext->frameData.iso_timer_interval);

		break;
	}
	default:
	{
		//NT_ASSERTMSG("Unexpected I/O", FALSE);
		break;
	}
	}

	//DbgPrint("ControllerEvtIoDeviceControl:status:%x\n", status);
	WdfRequestComplete(Request, status);
	return;
}



VOID Control_EVT_WDF_FILE_CLEANUP(
	_In_
	WDFFILEOBJECT FileObject
)
{
	PRINTLINE("Enter");

	WDFDEVICE wdfDevice;
	PUSB_CONTEXT usbContext;

	wdfDevice = WdfFileObjectGetDevice(FileObject);
	usbContext = WdfDeviceGetUsbContext(wdfDevice);

}