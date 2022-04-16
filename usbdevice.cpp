
#include "pch.h"
#include "usbdevice.h"




VOID
DEVICE_ENDPOINTS_CONFIGURE(
	_In_
	UDECXUSBDEVICE                    UdecxUsbDevice,
	_In_
	WDFREQUEST                      Request,
	_In_
	PUDECX_ENDPOINTS_CONFIGURE_PARAMS Params
)
{
	UNREFERENCED_PARAMETER(UdecxUsbDevice);
	UNREFERENCED_PARAMETER(Request);
	UNREFERENCED_PARAMETER(Params);

	//NTSTATUS                    status;
	//status = STATUS_SUCCESS;

	DbgPrint("DEVICE_ENDPOINTS_CONFIGURE  ConfigureType:%d,EndpointsToConfigureCount:%d,InterfaceNumber:%d\n", Params->ConfigureType, Params->EndpointsToConfigureCount, Params->InterfaceNumber);
	
	WdfRequestComplete(Request, STATUS_SUCCESS);
}


NTSTATUS
DEVICE_DATA_ENDPOINT_ADD(
	_In_
	UDECXUSBDEVICE                                    UdecxUsbDevice,
	_In_
	PUDECX_USB_ENDPOINT_INIT_AND_METADATA             EndpointToCreate
)
{
	UNREFERENCED_PARAMETER(UdecxUsbDevice);
	UNREFERENCED_PARAMETER(EndpointToCreate);

	NTSTATUS                    status;
	PUDECX_USBDEVICE_CONTEXT    deviceContext;
	//WDFQUEUE                    queue;
	WDF_IO_QUEUE_CONFIG         queueConfig;
	UDECX_USB_ENDPOINT_CALLBACKS  callbacks;
	PUSB_CONTEXT                pUsbContext;

	deviceContext = UdecxDeviceGetContext(UdecxUsbDevice);
	

	pUsbContext = WdfDeviceGetUsbContext(deviceContext->WdfDevice);

    status = 0;

    UCHAR epAddr = EndpointToCreate->EndpointDescriptor->bEndpointAddress;
    DbgPrint("Endpoint addr:%d", epAddr);

    if (epAddr == 0x82)
    {
        PRINTLINE("add iso endpoint ");
        //video stream iso transfer
        WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchManual);
        //Sequential must specify this callback

        status = WdfIoQueueCreate(deviceContext->WdfDevice,
            &queueConfig,
            WDF_NO_OBJECT_ATTRIBUTES,
            &pUsbContext->IsoVideoInQueue);

        if (!NT_SUCCESS(status)) {
            DbgPrint("WdfIoQueueCreate failed for control queue %d", status);
            goto exit;
        }

        /*status = WdfIoQueueReadyNotify(pUsbContext->IsoInQueue, EvtIoQueueReadyNotification, (WDFCONTEXT)deviceContext);
        if (!NT_SUCCESS(status)) {
            DbgPrint("Io_RetrieveBulkInQueue:WdfIoQueueReadyNotify failed for control queue %d\n", status);
            goto exit;
        }*/

        //UdecxUsbEndpointInitSetEndpointAddress(EndpointToCreate->UdecxUsbEndpointInit, EP_ADDR);

        UDECX_USB_ENDPOINT_CALLBACKS_INIT(&callbacks, UsbEndpointReset);
        UdecxUsbEndpointInitSetCallbacks(EndpointToCreate->UdecxUsbEndpointInit, &callbacks);

        status = UdecxUsbEndpointCreate(&EndpointToCreate->UdecxUsbEndpointInit,
            WDF_NO_OBJECT_ATTRIBUTES,
            &pUsbContext->IsoTransferVideoEndpoint);

        if (!NT_SUCCESS(status)) {
            DbgPrint("UdecxUsbEndpointCreate failed\n");
            goto exit;
        }

        UdecxUsbEndpointSetWdfIoQueue(pUsbContext->IsoTransferVideoEndpoint, pUsbContext->IsoVideoInQueue);
    }
    else if (epAddr == 0x83)
    {
        PRINTLINE("add Interrupt endpoint ");
        //video control Interrupt transfer 
        WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchManual);
        //Sequential must specify this callback

        status = WdfIoQueueCreate(deviceContext->WdfDevice,
            &queueConfig,
            WDF_NO_OBJECT_ATTRIBUTES,
            &pUsbContext->InterruptInQueue);

        if (!NT_SUCCESS(status)) {
            DbgPrint("WdfIoQueueCreate failed for control queue %d", status);
            goto exit;
        }

        /*status = WdfIoQueueReadyNotify(pUsbContext->IsoInQueue, EvtIoQueueReadyNotification, (WDFCONTEXT)deviceContext);
        if (!NT_SUCCESS(status)) {
            DbgPrint("Io_RetrieveBulkInQueue:WdfIoQueueReadyNotify failed for control queue %d\n", status);
            goto exit;
        }*/

        //UdecxUsbEndpointInitSetEndpointAddress(EndpointToCreate->UdecxUsbEndpointInit, EP_ADDR);

        UDECX_USB_ENDPOINT_CALLBACKS_INIT(&callbacks, UsbEndpointReset);
        UdecxUsbEndpointInitSetCallbacks(EndpointToCreate->UdecxUsbEndpointInit, &callbacks);

        status = UdecxUsbEndpointCreate(&EndpointToCreate->UdecxUsbEndpointInit,
            WDF_NO_OBJECT_ATTRIBUTES,
            &pUsbContext->InterruptEndpoint);

        if (!NT_SUCCESS(status)) {
            DbgPrint("UdecxUsbEndpointCreate failed\n");
            goto exit;
        }

        UdecxUsbEndpointSetWdfIoQueue(pUsbContext->InterruptEndpoint, pUsbContext->InterruptInQueue);

        PRINTLINE("Exit");
    }

    

	
exit:

	return status;
}



NTSTATUS
EvtUdecxUsbDeviceControlEndpointAdd(
	_In_
	UDECXUSBDEVICE            UdecxUsbDevice,
	_In_
	PUDECXUSBENDPOINT_INIT    UdecxUsbEndpointInit
)
{
	NTSTATUS                    status;
	PUDECX_USBDEVICE_CONTEXT    deviceContext;
	WDFQUEUE                    controlQueue;
	WDF_IO_QUEUE_CONFIG         queueConfig;
	UDECX_USB_ENDPOINT_CALLBACKS  callbacks;
	PUSB_CONTEXT                pUsbContext;

	deviceContext = UdecxDeviceGetContext(UdecxUsbDevice);
	pUsbContext = WdfDeviceGetUsbContext(deviceContext->WdfDevice);

	WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchSequential);

	queueConfig.EvtIoInternalDeviceControl = IoEvtControlUrb;

	status = WdfIoQueueCreate(deviceContext->WdfDevice,
		&queueConfig,
		WDF_NO_OBJECT_ATTRIBUTES,
		&controlQueue);

	if (!NT_SUCCESS(status)) {
		DbgPrint("EvtUdecxUsbDeviceControlEndpointAdd failed\n");
		goto exit;
	}

	UdecxUsbEndpointInitSetEndpointAddress(UdecxUsbEndpointInit, USB_DEFAULT_DEVICE_ADDRESS);

	UDECX_USB_ENDPOINT_CALLBACKS_INIT(&callbacks, UsbEndpointReset);
	UdecxUsbEndpointInitSetCallbacks(UdecxUsbEndpointInit, &callbacks);

	status = UdecxUsbEndpointCreate(&UdecxUsbEndpointInit,
		WDF_NO_OBJECT_ATTRIBUTES,
		&pUsbContext->ControlEndpoint);

	if (!NT_SUCCESS(status)) {
		DbgPrint("UdecxUsbEndpointCreate failed\n");
		goto exit;
	}

	UdecxUsbEndpointSetWdfIoQueue(pUsbContext->ControlEndpoint, controlQueue);
	DbgPrint("EvtUdecxUsbDeviceControlEndpointAdd done\n");

exit:

	return status;
}


NTSTATUS
Usb_Initialize(
	_In_
	WDFDEVICE WdfDevice
)
{
	NTSTATUS                                status;
	PUSB_CONTEXT                            usbContext;
	UDECX_USB_DEVICE_STATE_CHANGE_CALLBACKS   callbacks;

	DbgPrint("Usb_Initialize\n");

	usbContext = WdfDeviceGetUsbContext(WdfDevice);

	UsbValidateConstants();

	usbContext->UdecxCamInit = UdecxUsbDeviceInitAllocate(WdfDevice);

	if (usbContext->UdecxCamInit == NULL) {

		status = STATUS_INSUFFICIENT_RESOURCES;
		DbgPrint("Failed to allocate UDECXUSBDEVICE_INIT %d", status);
		goto exit;
	}

	//
	// State changed callbacks
	//
	UDECX_USB_DEVICE_CALLBACKS_INIT(&callbacks);

	callbacks.EvtUsbDeviceLinkPowerEntry = UsbDevice_EvtUsbDeviceLinkPowerEntry;
	callbacks.EvtUsbDeviceLinkPowerExit = UsbDevice_EvtUsbDeviceLinkPowerExit;
	callbacks.EvtUsbDeviceSetFunctionSuspendAndWake = UsbDevice_EvtUsbDeviceSetFunctionSuspendAndWake;
	callbacks.EvtUsbDeviceDefaultEndpointAdd = EvtUdecxUsbDeviceControlEndpointAdd;
	callbacks.EvtUsbDeviceEndpointAdd = DEVICE_DATA_ENDPOINT_ADD;
	callbacks.EvtUsbDeviceEndpointsConfigure = DEVICE_ENDPOINTS_CONFIGURE;

	UdecxUsbDeviceInitSetStateChangeCallbacks(usbContext->UdecxCamInit, &callbacks);

	//
	// Set required attributes.
	//
	UdecxUsbDeviceInitSetSpeed(usbContext->UdecxCamInit, UdecxUsbFullSpeed);

	UdecxUsbDeviceInitSetEndpointsType(usbContext->UdecxCamInit, UdecxEndpointTypeDynamic);
	
	//
	// Device descriptor
	//
	status = UdecxUsbDeviceInitAddDescriptor(usbContext->UdecxCamInit,
		usbContext->VCamDeviceDescriptor.data, (USHORT)usbContext->VCamDeviceDescriptor.len);
	if (!NT_SUCCESS(status)) {

		DbgPrint("UdecxCamInitAddDescriptor failed\n");
		goto exit;
	}


	//
	// String descriptors
	//
	status = UdecxUsbDeviceInitAddDescriptorWithIndex(usbContext->UdecxCamInit,
		(PUCHAR)g_LanguageDescriptor,
		sizeof(g_LanguageDescriptor),
		0);

	if (!NT_SUCCESS(status)) {
		DbgPrint("UdecxCamInitAddDescriptorWithIndex failed\n");
		goto exit;
	}

	status = UdecxUsbDeviceInitAddStringDescriptor(usbContext->UdecxCamInit,
		&g_ManufacturerStringEnUs,
		g_ManufacturerIndex,
		AMERICAN_ENGLISH);

	if (!NT_SUCCESS(status)) {

		goto exit;
	}

	status = UdecxUsbDeviceInitAddStringDescriptor(usbContext->UdecxCamInit,
		&g_ProductStringEnUs,
		g_ProductIndex,
		AMERICAN_ENGLISH);

	if (!NT_SUCCESS(status)) {

		goto exit;
	}


exit:

	//
	// On failure in this function (or later but still before creating the UDECXUSBDEVICE),
	// UdecxCamInit will be freed by Usb_Destroy.
	//

	return status;
}

NTSTATUS
Usb_ReadDescriptorsAndPlugIn(
	_In_
	WDFDEVICE WdfDevice
)
{
	NTSTATUS                        status;
	PUSB_CONTEXT                    usbContext;
	//PUSB_CONFIGURATION_DESCRIPTOR   pComputedConfigDescSet;
	WDF_OBJECT_ATTRIBUTES           attributes;
	PUDECX_USBDEVICE_CONTEXT          deviceContext;
	UDECX_USB_DEVICE_PLUG_IN_OPTIONS  pluginOptions;
	WDF_TIMER_CONFIG  timerConfig;
	WDF_OBJECT_ATTRIBUTES  timerAttributes;

	usbContext = WdfDeviceGetUsbContext(WdfDevice);

	DbgPrint("Usb_ReadDescriptorsAndPlugIn enter\n");

	status = UdecxUsbDeviceInitAddDescriptor(usbContext->UdecxCamInit, usbContext->VCamConfigDescriptorSet.data, (USHORT)usbContext->VCamConfigDescriptorSet.len);
	if (!NT_SUCCESS(status)) {
		DbgPrint("UdecxCamInitAddDescriptor failed\n");
		goto exit;
	}
	
	//
	// Create emulated USB device
	//
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, UDECX_USBDEVICE_CONTEXT);

	status = UdecxUsbDeviceCreate(&usbContext->UdecxCamInit,	&attributes,	&usbContext->VCamDevice);

	if (!NT_SUCCESS(status)) {
		DbgPrint("UdecxUsbDeviceCreate failed:%x\n", status);
		goto exit;
	}

	deviceContext = UdecxDeviceGetContext(usbContext->VCamDevice);
	deviceContext->WdfDevice = WdfDevice;


	usbContext = WdfDeviceGetUsbContext(WdfDevice);

	//creaet iso transfer timer
	WDF_TIMER_CONFIG_INIT(&timerConfig, EvtIsochTimer);

	timerConfig.AutomaticSerialization = TRUE;
	timerConfig.Period = 0;

	WDF_OBJECT_ATTRIBUTES_INIT(&timerAttributes);
	timerAttributes.ParentObject = WdfDevice;

	status = WdfTimerCreate(
		&timerConfig,
		&timerAttributes,
		&usbContext->isoTransferTimer
		);

	if (!NT_SUCCESS(status)) {
		DbgPrint("Timer creation failed %d", status);
		goto exit;
	}

	WdfTimerStart(usbContext->isoTransferTimer, WDF_REL_TIMEOUT_IN_MS(1000));

	// Create a DPC to complete read requests.
	WDF_DPC_CONFIG dpcConfig;
	WDF_OBJECT_ATTRIBUTES dpcAttributes;
	WDF_DPC_CONFIG_INIT(&dpcConfig, DpcCompleteVidoeIsoUrb);
	dpcConfig.AutomaticSerialization = TRUE;
	WDF_OBJECT_ATTRIBUTES_INIT(&dpcAttributes);
	dpcAttributes.ParentObject = WdfDevice;
	status = WdfDpcCreate(&dpcConfig,	&dpcAttributes,	&usbContext->completeUrbDpc);
	if (!NT_SUCCESS(status)) {

		DbgPrint("WdfDpcCreate(CompleteUrbDpc) failed %x\n", status);
		goto exit;
	}

	//
	// This begins USB communication and prevents us from modifying descriptors and simple endpoints.
	//
	UDECX_USB_DEVICE_PLUG_IN_OPTIONS_INIT(&pluginOptions);
	pluginOptions.Usb20PortNumber = 1;
	pluginOptions.Usb30PortNumber = 0;
	status = UdecxUsbDevicePlugIn(usbContext->VCamDevice, &pluginOptions);
	if (!NT_SUCCESS(status)) {
		DbgPrint("UdecxUsbDevicePlugIn failed  %d\n", status);
		goto exit;
	}

	DbgPrint("UdecxUsbDevicePlugIn OK  %d\n", status);

exit:

	//
	// Free temporary allocation always.
	//
	

	return status;
}



NTSTATUS
Usb_Disconnect(
	_In_
	WDFDEVICE WdfDevice
)
{
	NTSTATUS status = 0;
	PUSB_CONTEXT pUsbContext;


	pUsbContext = WdfDeviceGetUsbContext(WdfDevice);

	if (pUsbContext != NULL && pUsbContext->UdecxCamInit != NULL) {

		status = UdecxUsbDevicePlugOutAndDelete(pUsbContext->VCamDevice);

		if (!NT_SUCCESS(status)) {

			goto exit;
		}
	}

exit:
	PRINTLINE("Enter");
	return status;
}


VOID
Usb_Destroy(
	_In_
	WDFDEVICE WdfDevice
)
{
	PUSB_CONTEXT pUsbContext;

	pUsbContext = WdfDeviceGetUsbContext(WdfDevice);

	//
	// Free device init in case we didn't successfully create the device.
	//
	if (pUsbContext != NULL && pUsbContext->UdecxCamInit != NULL) {

		UdecxUsbDeviceInitFree(pUsbContext->UdecxCamInit);
		pUsbContext->UdecxCamInit = NULL;
	}

	//free video buffer here
	PRINTLINE("Enter");
	return;
}

VOID
Usb_UdecxUsbEndpointEvtReset(
	_In_ UCXCONTROLLER,
	_In_ UCXENDPOINT,
	_In_ WDFREQUEST
)
{
	// TODO: endpoint reset. will require a different function prototype
	PRINTLINE("Enter");
}




VOID
UsbEndpointReset(
	_In_
	UDECXUSBENDPOINT UdecxUsbEndpoint,
	_In_
	WDFREQUEST     Request
)
{
	UNREFERENCED_PARAMETER(UdecxUsbEndpoint);
	UNREFERENCED_PARAMETER(Request);

	PRINTLINE("Enter");
}



NTSTATUS
UsbDevice_EvtUsbDeviceLinkPowerEntry(
	_In_
	WDFDEVICE       UdecxWdfDevice,
	_In_
	UDECXUSBDEVICE    UdecxUsbDevice
)
{
	UNREFERENCED_PARAMETER(UdecxWdfDevice);
	UNREFERENCED_PARAMETER(UdecxUsbDevice);
	PRINTLINE("Enter");
	return STATUS_SUCCESS;
}

NTSTATUS
UsbDevice_EvtUsbDeviceLinkPowerExit(
	_In_
	WDFDEVICE                   UdecxWdfDevice,
	_In_
	UDECXUSBDEVICE                UdecxUsbDevice,
	_In_
	UDECX_USB_DEVICE_WAKE_SETTING WakeSetting
)
{
	UNREFERENCED_PARAMETER(UdecxWdfDevice);
	UNREFERENCED_PARAMETER(UdecxUsbDevice);
	UNREFERENCED_PARAMETER(WakeSetting);
	PRINTLINE("Enter");
	return STATUS_SUCCESS;
}

NTSTATUS
UsbDevice_EvtUsbDeviceSetFunctionSuspendAndWake(
	_In_
	WDFDEVICE                      UdecxWdfDevice,
	_In_
	UDECXUSBDEVICE                   UdecxUsbDevice,
	_In_
	ULONG                          Interface,
	_In_
	UDECX_USB_DEVICE_FUNCTION_POWER  FunctionPower
)
{
	UNREFERENCED_PARAMETER(UdecxWdfDevice);
	UNREFERENCED_PARAMETER(UdecxUsbDevice);
	UNREFERENCED_PARAMETER(Interface);
	UNREFERENCED_PARAMETER(FunctionPower);
	PRINTLINE("Enter");
	return STATUS_SUCCESS;
}

UCHAR g_camCfgDescriptorBuffer[] =
{
  0x09,
   0x02,
   0xa7,
   0x01,
   0x02,
   0x01,
   0x00,
   0x80,
   0x40,
   0x08,
   0x0b,
   0x00,
   0x02,
   0x0e,
   0x03,
   0x00,
   0x02,
   0x09,
   0x04,
   0x00,
   0x00,
   0x01,
   0x0e,
   0x01,
   0x00,
   0x02,
   0x0d,
   0x24,
   0x01,
   0x10,
   0x01,
   0x4f,
   0x00,
   0x80,
   0xc3,
   0xc9,
   0x01,
   0x01,
   0x01,
   0x12,
   0x24,
   0x02,
   0x01,
   0x01,
   0x02,
   0x00,
   0x00,
   0x01,
   0x00,
   0x01,
   0x00,
   0x00,
   0x00,
   0x03,
   0x60,
   0x1e,
   0x02,
   0x0c,
   0x24,
   0x05,
   0x02,
   0x01,
   0x00,
   0x00,
   0x02,
   0x7b,
   0x17,
   0x00,
   0x00,
   0x09,
   0x24,
   0x03,
   0x03,
   0x01,
   0x01,
   0x00,
   0x02,
   0x00,
   0x1c,
   0x24,
   0x06,
   0x04,
   0xa6,
   0x4d,
   0xf0,
   0x88,
   0x13,
   0xf7,
   0x4a,
   0x45,
   0xb6,
   0x25,
   0x37,
   0x93,
   0xae,
   0x44,
   0x75,
   0x16,
   0x08,
   0x01,
   0x01,
   0x03,
   0x00,
   0x00,
   0x00,
   0x00,
   0x07,
   0x05,
   0x83,
   0x03,
   0x10,
   0x00,
   0x01,
   0x09,
   0x04,
   0x01,
   0x00,
   0x00,
   0x0e,
   0x02,
   0x00,
   0x00,
   0x0e,
   0x24,
   0x01,
   0x01,
   0x1d,
   0x01,
   0x82,
   0x01,
   0x03,
   0x01,
   0x01,
   0x00,
   0x01,
   0x00,
   0x1b,
   0x24,
   0x04,
   0x01,
   0x07,
   0x55,
   0x59,
   0x56,
   0x59,
   0x00,
   0x00,
   0x10,
   0x00,
   0x80,
   0x00,
   0x00,
   0xaa,
   0x00,
   0x38,
   0x9b,
   0x71,
   0x10,
   0x01,
   0x00,
   0x00,
   0x00,
   0x00,
   0x22,
   0x24,
   0x05,
   0x01,
   0x00,
   0x00,
   0x10,
   0x70,
   0x08,
   0x00,
   0x00,
   0xe0,
   0x10,
   0x00,
   0x00,
   0x80,
   0xf4,
   0x00,
   0x00,
   0x0e,
   0x01,
   0x15,
   0x16,
   0x05,
   0x00,
   0x02,
   0x15,
   0x16,
   0x05,
   0x00,
   0x2b,
   0x2c,
   0x0a,
   0x00,
   0x22,
   0x24,
   0x05,
   0x02,
   0x00,
   0x80,
   0x07,
   0x38,
   0x04,
   0x00,
   0x00,
   0xe0,
   0x10,
   0x00,
   0x00,
   0x80,
   0xf4,
   0x00,
   0x48,
   0x3f,
   0x00,
   0x15,
   0x16,
   0x05,
   0x00,
   0x02,
   0x15,
   0x16,
   0x05,
   0x00,
   0x2b,
   0x2c,
   0x0a,
   0x00,
   0x22,
   0x24,
   0x05,
   0x03,
   0x00,
   0x00,
   0x05,
   0xd0,
   0x02,
   0x00,
   0x00,
   0xe0,
   0x10,
   0x00,
   0x00,
   0x80,
   0xf4,
   0x00,
   0x20,
   0x1c,
   0x00,
   0x15,
   0x16,
   0x05,
   0x00,
   0x02,
   0x15,
   0x16,
   0x05,
   0x00,
   0x2b,
   0x2c,
   0x0a,
   0x00,
   0x22,
   0x24,
   0x05,
   0x04,
   0x00,
   0xc0,
   0x02,
   0x40,
   0x02,
   0x00,
   0x00,
   0xe0,
   0x10,
   0x00,
   0x00,
   0x80,
   0xf4,
   0x00,
   0x60,
   0x0c,
   0x00,
   0x15,
   0x16,
   0x05,
   0x00,
   0x02,
   0x15,
   0x16,
   0x05,
   0x00,
   0x2b,
   0x2c,
   0x0a,
   0x00,
   0x22,
   0x24,
   0x05,
   0x05,
   0x00,
   0x5a,
   0x03,
   0xe0,
   0x01,
   0x00,
   0x00,
   0xe0,
   0x10,
   0x00,
   0x00,
   0x80,
   0xf4,
   0x80,
   0x91,
   0x0c,
   0x00,
   0x15,
   0x16,
   0x05,
   0x00,
   0x02,
   0x15,
   0x16,
   0x05,
   0x00,
   0x2b,
   0x2c,
   0x0a,
   0x00,
   0x22,
   0x24,
   0x05,
   0x06,
   0x00,
   0xe0,
   0x01,
   0x68,
   0x01,
   0x00,
   0x00,
   0xe0,
   0x10,
   0x00,
   0x00,
   0x80,
   0xf4,
   0x00,
   0x46,
   0x05,
   0x00,
   0x15,
   0x16,
   0x05,
   0x00,
   0x02,
   0x15,
   0x16,
   0x05,
   0x00,
   0x2b,
   0x2c,
   0x0a,
   0x00,
   0x22,
   0x24,
   0x05,
   0x07,
   0x00,
   0x40,
   0x01,
   0xb4,
   0x00,
   0x00,
   0x00,
   0xe0,
   0x10,
   0x00,
   0x00,
   0x80,
   0xf4,
   0x00,
   0xc2,
   0x01,
   0x00,
   0x15,
   0x16,
   0x05,
   0x00,
   0x02,
   0x15,
   0x16,
   0x05,
   0x00,
   0x2b,
   0x2c,
   0x0a,
   0x00,
   0x06,
   0x24,
   0x0d,
   0x01,
   0x01,
   0x04,
   0x09,
   0x04,
   0x01,
   0x01,
   0x01,
   0x0e,
   0x02,
   0x00,
   0x00,
   0x07,
   0x05,
   0x82,
   0x05,
   0x00,
   0x0c,
   0x01,
};

void fill_cam_cfg_desc_video(PUSB_CONTEXT deviceContext)
{
	deviceContext->VCamConfigDescriptorSet.len = sizeof(g_camCfgDescriptorBuffer);
	deviceContext->VCamConfigDescriptorSet.data = g_camCfgDescriptorBuffer;
}

usb_device_descriptor g_camDevDescriptor;
void fill_cam_dev_desc_video(PUSB_CONTEXT deviceContext)
{
	
	g_camDevDescriptor.bLength = 0x12;  // length 18
	g_camDevDescriptor.bDescriptorType = 0x01;  // type 1
	g_camDevDescriptor.bcdUSB = 0x0200; // usb 2.0
	g_camDevDescriptor.bDeviceClass = 0xEF; // 固定为ef ，表示是misc设备
	g_camDevDescriptor.bDeviceSubClass = 0x02; // common class
	g_camDevDescriptor.bDeviceProtocol = 0x01; //Interface Association Descriptor
	g_camDevDescriptor.bMaxPacketSize0 = 0x40; // 64
	g_camDevDescriptor.idVendor = 0xbb10;
	g_camDevDescriptor.idProduct = 0xcc10;
	g_camDevDescriptor.bcdDevice = 0x00; /// 设备出厂编号
	g_camDevDescriptor.iManufacturer = 0x01; // 厂商信息序号，固定为1 
	g_camDevDescriptor.iProduct = 0x02;     //产品信息序号，固定2
	g_camDevDescriptor.iSerialNumber = 0x00; //无序列号
	g_camDevDescriptor.bNumConfigurations = 1; //一个配置描述符

	deviceContext->VCamDeviceDescriptor.data = (UCHAR*)&g_camDevDescriptor;
	deviceContext->VCamDeviceDescriptor.len = sizeof(usb_device_descriptor);
}


void init_cam_prop(PUSB_CONTEXT usbContext)
{
	fill_cam_dev_desc_video(usbContext);
	fill_cam_cfg_desc_video(usbContext);

	usbContext->frameData.width = 1920;
	usbContext->frameData.height = 1080;
	usbContext->frameData.bitsPerPixel = 16;
	usbContext->frameData.bFormatIndex = 1;
	usbContext->frameData.bFrameIndex = 1;
	usbContext->frameData.frame_flip = 0;
	usbContext->frameData.frame_length = usbContext->frameData.width * usbContext->frameData.height * (16 / 8); ;
	usbContext->frameData.frame_pos = 0; ///当前帧的传输位置
    usbContext->frameData.iso_timer_interval = (-1) * (1000/120) * 10;
	//usbContext->frameData.timeout = 0;
	//here allocat max buffer, no need to release
	usbContext->frameData.frame_buf = NULL;// (UCHAR*)ExAllocatePoolWithTag(NonPagedPoolNx, 1920 * 1080 * 4, UDECXVDEV_POOL_TAG);
	usbContext->currentVideoRequest = NULL;

}

