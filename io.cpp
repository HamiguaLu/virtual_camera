

#include "pch.h"

#include "io.h"

#if 0
VOID
IoAudioEvtControlUrb(
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
	UNREFERENCED_PARAMETER(Queue);
	UNREFERENCED_PARAMETER(Request);
	UNREFERENCED_PARAMETER(IoControlCode);

	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(InputBufferLength);

	NTSTATUS status = 0;
	PUCHAR transferBuffer;
	DbgPrint("Unhandled audio control evt!!!!!\n");
	status = UdecxUrbRetrieveBuffer(Request, &transferBuffer, NULL);
	if (NT_SUCCESS(status)) {
		if (OutputBufferLength > 0)
		{
			RtlZeroMemory(transferBuffer, OutputBufferLength);
		}
	}

	CompleteURB(Request, OutputBufferLength, 0);
	return;
}
#endif


VOID
IoEvtControlUrb(
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
	WDF_USB_CONTROL_SETUP_PACKET setupPacket;
	NTSTATUS status;
	PUCHAR transferBuffer;
	ULONG transferBufferLength, transferedLength = 0;
	WDFDEVICE wdfDevice;
	PUSB_CONTEXT usbContext;

	UNREFERENCED_PARAMETER(Queue);
	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(InputBufferLength);

	NT_VERIFY(IoControlCode == IOCTL_INTERNAL_USB_SUBMIT_URB);

	wdfDevice = WdfIoQueueGetDevice(Queue);
	usbContext = WdfDeviceGetUsbContext(wdfDevice);

	status = UdecxUrbRetrieveBuffer(Request, &transferBuffer, &transferBufferLength);
	if (!NT_SUCCESS(status)) {
		// Could mean there is no buffer on the request
		transferBuffer = NULL;
		transferBufferLength = 0;
		status = STATUS_SUCCESS;
		DbgPrint("No transfer buffer provided\n");
		goto exit;
	}

	//Initlize transferedLength and update it later to real length if it was changed.
	//transferedLength = transferBufferLength;
	status = UdecxUrbRetrieveControlSetupPacket(Request, &setupPacket);
	if (!NT_SUCCESS(status)) {
		DbgPrint("WdfRequest %p is not a control URB? UdecxUrbRetrieveControlSetupPacket %d", Request, status);
		UdecxUrbCompleteWithNtStatus(Request, status);
		goto exit;
	}

	if (setupPacket.Packet.bm.Request.Recipient != BmRequestToInterface ||
		setupPacket.Packet.bm.Request.Type != BmRequestClass || transferBufferLength < setupPacket.Packet.wLength)
	{
		status = STATUS_INVALID_DEVICE_REQUEST;
		DbgPrint("Unrecognized control request \n");
		goto exit;
	}

#if 0
	int selector = setupPacket.Packet.wValue.Bytes.HiByte;
	int req = setupPacket.Packet.bRequest;
	DbgPrint("selector:%d,req:%d\n", selector,req);
#endif

	if (setupPacket.Packet.wIndex.Value == 1)
	{
		status = videoControl(usbContext, setupPacket, transferBuffer, &transferedLength);
	}
	else
	{
		status = ptzControl(usbContext, setupPacket, transferBuffer, &transferedLength);
	}


exit:
	//DbgPrint("CompleteURB:%d,%d\n", transferedLength, status);
	CompleteURB(Request, transferedLength, status);
	return;
}





void init_ptz_ctrl_data(PUSB_CONTEXT usbContext)
{
	usbContext->ptzData.focus_rel_data[0] = 0;//bFocusRelative
	usbContext->ptzData.focus_rel_data[1] = FOCUS_REL_DEFAULT;//bSpeed

	usbContext->ptzData.zoom_abs_data[0] = ZOOM_ABS_DEFAULT;

	usbContext->ptzData.zoom_rel_data[0] = 0;//bZoom
	usbContext->ptzData.zoom_rel_data[1] = 0;//bDigitalZoom
	usbContext->ptzData.zoom_rel_data[2] = ZOOM_REL_DEFAULT;//bSpeed

	usbContext->ptzData.pantilt_rel_data[0] = 0; // bPanRelative 
	usbContext->ptzData.pantilt_rel_data[1] = PANTILT_REL_SPEED_DEFAULT; // bPanSpeed 
	usbContext->ptzData.pantilt_rel_data[2] = 0; // bTiltRelative 
	usbContext->ptzData.pantilt_rel_data[3] = PANTILT_REL_SPEED_DEFAULT; // bTiltSpeed 
}



void ptzNotifyApp(PUSB_CONTEXT usbContext, int controlType, int v1, int v2, int v3, int v4)
{
	PTZ_CTRL_Data cmd;
	cmd.controlType = controlType;
	cmd.v1 = v1;
	cmd.v2 = v2;
	cmd.v3 = v3;
	cmd.v4 = v4;

	PTZ_CTRL_LIST_ITEM*  pItem = (PTZ_CTRL_LIST_ITEM*)ExAllocatePool(NonPagedPool, sizeof(PTZ_CTRL_LIST_ITEM));
	if (pItem == NULL)
	{
		PRINTLINE("No mem");
		return;
	}

	RtlCopyMemory((void*)(&pItem->ctrl), (void*)&cmd, sizeof(cmd));

	InsertHeadList(&usbContext->ptzListHdr, &pItem->ListEntry);
	PRINTLINE("Insert ptz cmd to list");

}




NTSTATUS ptzControl(
	_In_ PUSB_CONTEXT usbContext,
	_In_
	WDF_USB_CONTROL_SETUP_PACKET setupPacket,
	_In_
	PUCHAR transferBuffer,
	_Out_
	PULONG transferedLen

)
{
	NTSTATUS status = 0;
	*transferedLen = setupPacket.Packet.wLength;


	int selector = setupPacket.Packet.wValue.Bytes.HiByte;
	int req = setupPacket.Packet.bRequest;
	//DbgPrint("selector:%d,req:%d\n", selector,req);
	if (selector == UVC_CT_ZOOM_ABSOLUTE_CONTROL)
	{
		/*
		A value of 1 indicates that the zoom lens is moved towards the telephoto direction. A value
		of zero indicates that the zoom lens is stopped, and a value of 0xFF indicates that the zoom lens
		is moved towards the wide-angle direction
		*/
		//INT8 zoom = (INT8)zoom_rel_data[0];
		//DbgPrint("UVC_CT_ZOOM_ABSOLUTE_CONTROL\n");
		switch (req)
		{
		case UVC_GET_INFO:
		{
			transferBuffer[0] = 0x02; /// GET / SET
			break;
		}
		case UVC_SET_CUR:
		{
			//INT8 val = transferBuffer[2];
			//zoom_rel_data[0] = val == 0 ? 0 : (val > 0) ? 1 : 0xff;
			memcpy(usbContext->ptzData.zoom_abs_data, transferBuffer, sizeof(UINT16));
			UINT16 wObjectiveFocalLength = 0;
			memcpy(&wObjectiveFocalLength, transferBuffer, sizeof(UINT16));

			////logMessage("wObjectiveFocalLength is %d", wObjectiveFocalLength);

			if (wObjectiveFocalLength < ZOOM_ABS_MIN) {
				wObjectiveFocalLength = ZOOM_ABS_MIN;
			}

			if (wObjectiveFocalLength > ZOOM_ABS_MAX) {
				wObjectiveFocalLength = ZOOM_ABS_MAX;
			}

			ptzNotifyApp(usbContext, NDI_PTZ_ZOOM_ABS, wObjectiveFocalLength, 0, 0, 0);
			*transferedLen = 0;
			break;
		}
		case UVC_GET_CUR:
		{
			memcpy(transferBuffer, usbContext->ptzData.zoom_abs_data, sizeof(UINT16));
			break;
		}
		case UVC_GET_MIN:
		{
			UINT16 wObjectiveFocalLength = ZOOM_ABS_MIN;
			memcpy(transferBuffer, &wObjectiveFocalLength, sizeof(UINT16));
			break;
		}
		case UVC_GET_MAX:
		{
			UINT16 wObjectiveFocalLength = ZOOM_ABS_MAX;
			memcpy(transferBuffer, &wObjectiveFocalLength, sizeof(UINT16));
			break;
		}
		case UVC_GET_RES:
		{
			UINT16 wObjectiveFocalLength = 1;
			memcpy(transferBuffer, &wObjectiveFocalLength, sizeof(UINT16));
			break;
		}
		case UVC_GET_DEF:
		{
			UINT16 wObjectiveFocalLength = ZOOM_ABS_DEFAULT;
			memcpy(transferBuffer, &wObjectiveFocalLength, sizeof(UINT16));
			break;
		}

		}

		status = 0;
		return status;
	}

	if (selector == UVC_CT_ZOOM_RELATIVE_CONTROL)
	{
		/*
		A value of 1 indicates that the zoom lens is moved towards the telephoto direction. A value
		of zero indicates that the zoom lens is stopped, and a value of 0xFF indicates that the zoom lens
		is moved towards the wide-angle direction
		*/
		//INT8 zoom = (INT8)zoom_rel_data[0];
		//DbgPrint("UVC_CT_ZOOM_RELATIVE_CONTROL\n");
		switch (req)
		{
		case UVC_GET_INFO:
		{
			transferBuffer[0] = 0x02; /// GET / SET
			break;
		}
		case UVC_SET_CUR:
		{
			//INT8 val = transferBuffer[2];
			//zoom_rel_data[0] = val == 0 ? 0 : (val > 0) ? 1 : 0xff;
			usbContext->ptzData.zoom_rel_data[0] = transferBuffer[0];
			usbContext->ptzData.zoom_rel_data[2] = transferBuffer[2];
			//logMessage("bZoom is %02x,bSpeed is %d", zoom_rel_data[0], zoom_rel_data[2]);
			UINT8 bSpeed = usbContext->ptzData.zoom_rel_data[2];
			//bSpeed -= ZOOM_REL_MIN;
			if (bSpeed < 1) {
				bSpeed = 1;
			}

			if (bSpeed > 10) {
				bSpeed = 10;
			}
			ptzNotifyApp(usbContext, NDI_PTZ_ZOOM, usbContext->ptzData.zoom_rel_data[0], bSpeed, 0, 0);
			*transferedLen = 0;

			break;
		}
		case UVC_GET_CUR:
		{
			transferBuffer[0] = usbContext->ptzData.zoom_rel_data[0];//bZoom
			transferBuffer[1] = 1;//bDigitalZoom
			transferBuffer[2] = usbContext->ptzData.zoom_rel_data[2];//bSpeed

			break;
		}
		case UVC_GET_MIN:
		{
			transferBuffer[0] = 0;//bZoom
			transferBuffer[1] = 0;//bDigitalZoom
			transferBuffer[2] = ZOOM_REL_MIN;//bSpeed
			break;
		}
		case UVC_GET_MAX:
		{
			transferBuffer[0] = 0;//bZoom
			transferBuffer[1] = 0;//bDigitalZoom
			transferBuffer[2] = ZOOM_REL_MAX;//bSpeed
			break;
		}
		case UVC_GET_RES:
		{
			transferBuffer[0] = 0;//bZoom
			transferBuffer[1] = 0;//bDigitalZoom
			transferBuffer[2] = 1;//bSpeed
			break;
		}
		case UVC_GET_DEF:
		{
			transferBuffer[0] = 0;//bZoom
			transferBuffer[1] = 1;//bDigitalZoom
			transferBuffer[2] = ZOOM_REL_DEFAULT;//bSpeed
			break;
		}

		}

		status = 0;
		return status;
	}

	if (selector == UVC_CT_PANTILT_ABSOLUTE_CONTROL)
	{
		//DbgPrint("UVC_CT_PANTILT_ABSOLUTE_CONTROL\n");
		switch (req)
		{
		case UVC_GET_INFO:
		{
			transferBuffer[0] = 0x02; /// GET / SET
			break;
		}
		case UVC_SET_CUR:
		{
			INT32 dwPanAbsolute, dwTiltAbsolute;

			memcpy(usbContext->ptzData.pantilt_abs_data, transferBuffer, sizeof(INT32) * 2);

			dwPanAbsolute = usbContext->ptzData.pantilt_abs_data[0];
			dwTiltAbsolute = usbContext->ptzData.pantilt_abs_data[1];

			//logMessage("dwPanAbsolute is %d,dwTiltAbsolute is %d", dwPanAbsolute, dwTiltAbsolute);

			//bPanSpeed -= PANTILT_REL_SPEED_MIN;
			//bTiltSpeed -= PANTILT_REL_SPEED_MIN;

			/*if (dwPanAbsolute < 1)
			{
				dwPanAbsolute = 1;
			}

			if (dwPanAbsolute > 10)
			{
				dwPanAbsolute = 10;
			}

			if (dwTiltAbsolute < 1)
			{
				dwTiltAbsolute = 1;
			}

			if (dwTiltAbsolute > 10)
			{
				dwTiltAbsolute = 10;
			}*/

			ptzNotifyApp(usbContext, NDI_PTZ_PAN_TILT_ABS, dwPanAbsolute, dwTiltAbsolute, 0, 0);
			*transferedLen = 0;
			break;
		}
		case UVC_GET_CUR:
		{
			memcpy(transferBuffer, usbContext->ptzData.pantilt_abs_data, sizeof(INT32) * 2);
			break;
		}
		case UVC_GET_MIN:
		{
			INT32 dwPanAbsolute, dwTiltAbsolute;
			dwPanAbsolute = 1;
			dwTiltAbsolute = 1;

			memcpy(transferBuffer, &dwPanAbsolute, sizeof(INT32));
			memcpy(transferBuffer + 4, &dwTiltAbsolute, sizeof(INT32));
			break;
		}
		case UVC_GET_MAX:
		{
			INT32 dwPanAbsolute, dwTiltAbsolute;
			dwPanAbsolute = 10;
			dwTiltAbsolute = 10;

			memcpy(transferBuffer, &dwPanAbsolute, sizeof(INT32));
			memcpy(transferBuffer + 4, &dwTiltAbsolute, sizeof(INT32));
			break;
		}
		case UVC_GET_RES:
		{
			INT32 dwPanAbsolute, dwTiltAbsolute;
			dwPanAbsolute = 1;
			dwTiltAbsolute = 1;

			memcpy(transferBuffer, &dwPanAbsolute, sizeof(INT32));
			memcpy(transferBuffer + 4, &dwTiltAbsolute, sizeof(INT32));
			break;
		}
		case UVC_GET_DEF:
		{
			INT32 dwPanAbsolute, dwTiltAbsolute;
			dwPanAbsolute = 5;
			dwTiltAbsolute = 5;

			memcpy(transferBuffer, &dwPanAbsolute, sizeof(INT32));
			memcpy(transferBuffer + 4, &dwTiltAbsolute, sizeof(INT32));
			break;
		}

		}

		status = 0;
		return status;
	}

	if (selector == UVC_CT_PANTILT_RELATIVE_CONTROL)
	{
		//DbgPrint("UVC_CT_PANTILT_RELATIVE_CONTROL\n");
		switch (req)
		{
		case UVC_GET_INFO:
		{
			transferBuffer[0] = 0x02; /// GET / SET
			break;
		}
		case UVC_SET_CUR:
		{
			memcpy(usbContext->ptzData.pantilt_rel_data, transferBuffer, sizeof(usbContext->ptzData.pantilt_rel_data));
			//logMessage("bPanRelative is %x,bPanSpeed is %d,bTiltRelative is %x,bTiltSpeed is %d", pantilt_rel_data[0], pantilt_rel_data[1], pantilt_rel_data[2], pantilt_rel_data[3]);

			UINT8 bPanRelative, bTiltRelative, bPanSpeed, bTiltSpeed;
			bPanRelative = usbContext->ptzData.pantilt_rel_data[0];
			bPanSpeed = usbContext->ptzData.pantilt_rel_data[1];
			bTiltRelative = usbContext->ptzData.pantilt_rel_data[2];
			bTiltSpeed = usbContext->ptzData.pantilt_rel_data[3];

			//bPanSpeed -= PANTILT_REL_SPEED_MIN;
			//bTiltSpeed -= PANTILT_REL_SPEED_MIN;

			if (bPanSpeed < 1)
			{
				bPanSpeed = 1;
			}

			if (bPanSpeed > 10)
			{
				bPanSpeed = 10;
			}

			if (bTiltSpeed < 1)
			{
				bTiltSpeed = 1;
			}

			if (bTiltSpeed > 10)
			{
				bTiltSpeed = 10;
			}

			ptzNotifyApp(usbContext, NDI_PTZ_PAN_TILT, bPanRelative, bPanSpeed, bTiltRelative, bTiltSpeed);
			*transferedLen = 0;
			break;
		}
		case UVC_GET_CUR:
		{
			/*transferBuffer[0] = 0; //bPanRelative
			transferBuffer[1] = 10; //bPanSpeed
			transferBuffer[2] = 0; // bTiltRelative
			transferBuffer[3] = 10; //bTiltSpeed */

			memcpy(transferBuffer, usbContext->ptzData.pantilt_rel_data, sizeof(usbContext->ptzData.pantilt_rel_data));

			break;
		}
		case UVC_GET_MIN:
		{
			transferBuffer[0] = 0; // bPanRelative 
			transferBuffer[1] = PANTILT_REL_SPEED_MIN; // bPanSpeed 
			transferBuffer[2] = 0; // bTiltRelative 
			transferBuffer[3] = PANTILT_REL_SPEED_MIN; // bTiltSpeed 
			break;
		}
		case UVC_GET_MAX:
		{
			transferBuffer[0] = 0; // bPanRelative 
			transferBuffer[1] = PANTILT_REL_SPEED_MAX; // bPanSpeed 
			transferBuffer[2] = 0; // bTiltRelative 
			transferBuffer[3] = PANTILT_REL_SPEED_MAX; // bTiltSpeed 
			break;
		}
		case UVC_GET_RES:
		{
			transferBuffer[0] = 0; // bPanRelative 
			transferBuffer[1] = 1; // bPanSpeed 
			transferBuffer[2] = 0; // bTiltRelative 
			transferBuffer[3] = 1; // bTiltSpeed 
			break;
		}
		case UVC_GET_DEF:
		{
			transferBuffer[0] = 0; // bPanRelative 
			transferBuffer[1] = PANTILT_REL_SPEED_DEFAULT; // bPanSpeed 
			transferBuffer[2] = 0; // bTiltRelative 
			transferBuffer[3] = PANTILT_REL_SPEED_DEFAULT; // bTiltSpeed 
			break;
		}

		}

		status = 0;
		return status;
	}

	if (selector == UVC_CT_FOCUS_ABSOLUTE_CONTROL)
	{
		//UINT8 bFocus = (UINT8)focus_rel_data[0];
		//DbgPrint("UVC_CT_FOCUS_ABSOLUTE_CONTROL\n");
		switch (req)
		{
		case UVC_GET_INFO:
		{
			transferBuffer[0] = 0x02; /// GET / SET
			break;
		}
		case UVC_SET_CUR:
		{
			memcpy(usbContext->ptzData.focus_abs_data, transferBuffer, sizeof(INT16));
			INT16 wFocusAbsolute = usbContext->ptzData.focus_abs_data[0];

			//logMessage("wFocusAbsolute is %d", wFocusAbsolute);

			if (wFocusAbsolute < 1) {
				wFocusAbsolute = 1;
			}

			if (wFocusAbsolute > 10) {
				wFocusAbsolute = 10;
			}
			ptzNotifyApp(usbContext, NDI_PTZ_FOCUS_ABS, wFocusAbsolute, 0, 0, 0);
			*transferedLen = 0;
			break;
		}
		case UVC_GET_CUR:
		{
			memcpy(transferBuffer, usbContext->ptzData.focus_abs_data, sizeof(INT16));

			break;
		}
		case UVC_GET_MIN:
		{
			INT16 wFocusAbsolute = 1;
			memcpy(transferBuffer, &wFocusAbsolute, sizeof(INT16));
			break;
		}
		case UVC_GET_MAX:
		{
			INT16 wFocusAbsolute = 10;
			memcpy(transferBuffer, &wFocusAbsolute, sizeof(INT16));
			break;
		}
		case UVC_GET_RES:
		{
			INT16 wFocusAbsolute = 1;
			memcpy(transferBuffer, &wFocusAbsolute, sizeof(INT16));
			break;
		}
		case UVC_GET_DEF:
		{
			INT16 wFocusAbsolute = 5;
			memcpy(transferBuffer, &wFocusAbsolute, sizeof(INT16));
			break;
		}

		}

		status = 0;
		return status;
	}


	if (selector == UVC_CT_FOCUS_RELATIVE_CONTROL)
	{
		//UINT8 bFocus = (UINT8)focus_rel_data[0];
		//DbgPrint("UVC_CT_FOCUS_RELATIVE_CONTROL\n");
		switch (req)
		{
		case UVC_GET_INFO:
		{
			transferBuffer[0] = 0x02; /// GET / SET
			break;
		}
		case UVC_SET_CUR:
		{
			usbContext->ptzData.focus_rel_data[0] = transferBuffer[0];
			usbContext->ptzData.focus_rel_data[1] = transferBuffer[1];
			//logMessage("bFocus is %02x,bSpeed is %d", focus_rel_data[0], focus_rel_data[1]);
			UINT8 bSpeed = usbContext->ptzData.focus_rel_data[1];
			//bSpeed -= ZOOM_REL_MIN;
			if (bSpeed < 1) {
				bSpeed = 1;
			}

			if (bSpeed > 10) {
				bSpeed = 10;
			}

			ptzNotifyApp(usbContext, NDI_PTZ_FOCUS, usbContext->ptzData.focus_rel_data[0], bSpeed, 0, 0);
			*transferedLen = 0;
			break;
		}
		case UVC_GET_CUR:
		{
			transferBuffer[0] = usbContext->ptzData.focus_rel_data[0];//bFocus
			transferBuffer[1] = usbContext->ptzData.focus_rel_data[1];//bSpeed

			break;
		}
		case UVC_GET_MIN:
		{
			transferBuffer[0] = 0;//bFocus
			transferBuffer[1] = FOCUS_REL_MIN;//bSpeed
			break;
		}
		case UVC_GET_MAX:
		{
			transferBuffer[0] = 0;//bFocus
			transferBuffer[1] = FOCUS_REL_MAX;//bSpeed
			break;
		}
		case UVC_GET_RES:
		{
			transferBuffer[0] = 0;//bFocus
			transferBuffer[1] = 1;//bSpeed
			break;
		}
		case UVC_GET_DEF:
		{
			transferBuffer[0] = 0;//bFocus
			transferBuffer[1] = FOCUS_REL_DEFAULT;//bSpeed
			break;
		}

		}

		status = 0;
		return status;
	}

	if (selector == UVC_CT_FOCUS_AUTO_CONTROL)
	{
		//UINT8 bFocus = (UINT8)focus_rel_data[0];
		//DbgPrint("UVC_CT_FOCUS_AUTO_CONTROL\n");
		switch (req)
		{
		case UVC_GET_INFO:
		{
			transferBuffer[0] = 0x02; /// GET / SET
			break;
		}
		case UVC_SET_CUR:
		{
			usbContext->ptzData.focus_auto_data[0] = transferBuffer[0];
			//logMessage("bFocusAuto is %d", transferBuffer[0]);

			ptzNotifyApp(usbContext, NDI_PTZ_FOCUS_AUTO, transferBuffer[0], 0, 0, 0);
			*transferedLen = 0;
			break;
		}
		case UVC_GET_CUR:
		{
			transferBuffer[0] = usbContext->ptzData.focus_auto_data[0];//bFocus
			break;
		}
		case UVC_GET_MIN:
		{
			transferBuffer[0] = 0;//bFocus
			break;
		}
		case UVC_GET_MAX:
		{
			transferBuffer[0] = 1;//bFocus
			break;
		}
		case UVC_GET_RES:
		{
			transferBuffer[0] = 1;//bFocus
			break;
		}
		case UVC_GET_DEF:
		{
			transferBuffer[0] = 1;//bFocus
			break;
		}

		}

		status = 0;
		return status;
	}

#if 0
	DbgPrint("Unhandled UVC selector %x\n", selector);
	DbgPrint(" bRequest:0x%02x wValue: 0x%04x wIndex: 0x%04x wLength: 0x%04x\n",
		setupPacket.Packet.bRequest, setupPacket.Packet.wValue.Bytes.HiByte,
		setupPacket.Packet.wIndex.Value, setupPacket.Packet.wLength);
#endif

	return status;
}





NTSTATUS videoControl(
	_In_ PUSB_CONTEXT usbContext,
	_In_
	WDF_USB_CONTROL_SETUP_PACKET setupPacket,
	_In_
	PUCHAR transferBuffer,
	_Out_
	PULONG transferedLen

)
{
	NTSTATUS status = 0;
	*transferedLen = 0;

	if (setupPacket.Packet.wIndex.Value != 1)
	{
		//not video interface
		return STATUS_UNSUCCESSFUL;
	}

	if (setupPacket.Packet.wLength != sizeof(uvc_streaming_control))
	{
		DbgPrint("data len wrong\n");
		goto exit;
	}

	uvc_streaming_control* sc = (uvc_streaming_control*)transferBuffer;
	if (setupPacket.Packet.bRequest == UVC_SET_CUR)
	{
		//curr_format_index = sc->bFormatIndex;
		//curr_frame_index = sc->bFrameIndex;
		*transferedLen = 0;
		status = 0;
		DbgPrint("Video UVC_SET_CUR:curr_format_index is %d,curr_frame_index is %d\n", sc->bFormatIndex, sc->bFrameIndex);
		if (sc->bFrameIndex != usbContext->frameData.bFrameIndex)
		{
			DbgPrint("Video set cur:STATUS_INVALID_PARAMETER: now supported frameIdx is %d", usbContext->frameData.bFrameIndex);
			status = STATUS_INVALID_PARAMETER;
		}
		else
		{
			usbContext->frameData.needChangeFormat = 0;
		}
		goto exit;
	}

	RtlZeroBytes(sc, sizeof(uvc_streaming_control));
	sc->bmHint = 0x01; /// supported dwFrameInterval
	sc->bFormatIndex = usbContext->frameData.bFormatIndex;
	sc->bFrameIndex = usbContext->frameData.bFrameIndex;
	sc->dwFrameInterval = 0x00051615; // (33 ms -> 30.00 fps) 
	sc->wKeyFrameRate = 0;
	sc->wPFrameRate = 0;
	sc->wCompQuality = 0;
	sc->wCompWindowSize = 0;
	sc->wDelay = 0x0A; //// 停留 毫秒 ms ???
	sc->dwMaxVideoFrameSize = usbContext->frameData.frame_length; ////
	sc->dwMaxPayloadTransferSize = 2048; //// ???? 

	status = 0;
	*transferedLen = setupPacket.Packet.wLength;
	switch (setupPacket.Packet.bRequest)
	{
	case UVC_GET_DEF:
	{
		//RtlZeroBytes(transferBuffer, setupPacket.Packet.wLength);
		//*transferedLen = setupPacket.Packet.wLength;
		//DbgPrint("video interface UVC_GET_DEF\n");
		break;
	}
	case UVC_GET_CUR:
	{
		DbgPrint("video interface UVC_GET_CUR\n");


		break;
	}
	case UVC_GET_MIN:
	{
		DbgPrint("video interface UVC_GET_MIN\n");
		sc->bFrameIndex = 7;
		break;
	}
	case UVC_GET_MAX:
	{
		DbgPrint("video interface UVC_GET_MAX\n");
		sc->bFrameIndex = 1;

		break;
	}
	default:
	{
		DbgPrint("Unhandled video interface %d\n", setupPacket.Packet.bRequest);
		break;
	}
	}

exit:

	return status;
}





VOID CompleteURB(
	_In_ WDFREQUEST request,
	_In_ ULONG transferBufferLength,
	NTSTATUS status
)
{
	if (NT_SUCCESS(status)) {
		UdecxUrbSetBytesCompleted(request, transferBufferLength);
	}

	UdecxUrbCompleteWithNtStatus(request, status);
}




#if 0
#define DELAY_ONE_MICROSECOND (-10)
#define DELAY_ONE_MILLISECOND (DELAY_ONE_MICROSECOND*1000)
VOID MySleep(LONG msec)
{
	LARGE_INTEGER my_interval;
	my_interval.QuadPart = DELAY_ONE_MILLISECOND;
	my_interval.QuadPart *= msec;
	KeDelayExecutionThread(KernelMode, 0, &my_interval);
}

#endif

#if 0
VOID DpcCompleteAudioIsoUrb(
	IN WDFDPC Dpc
)
{
	PUSB_CONTEXT usbContext;
	WDFREQUEST Request;
	NTSTATUS status = STATUS_SUCCESS;

	usbContext = WdfDeviceGetUsbContext(WdfDpcGetParentObject(Dpc));

	PUCHAR vBuf;
	ULONG vBufLen = 0;
	ULONG transfered = 0;
	WDF_REQUEST_PARAMETERS  requestParams;

	Request = usbContext->currentAudioRequest;

	WDF_REQUEST_PARAMETERS_INIT(&requestParams);
	WdfRequestGetParameters(Request, &requestParams);


	PURB urb = (PURB)requestParams.Parameters.Others.Arg1;

	if (urb->UrbHeader.Function != URB_FUNCTION_ISOCH_TRANSFER)
	{
		PRINTLINE("Incorrect URB");
		status = STATUS_INVALID_PARAMETER;
		WdfRequestComplete(Request, status);
		return;
	}

	status = UdecxUrbRetrieveBuffer(Request, &vBuf, &vBufLen);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("WdfRequest %p unable to retrieve buffer %d", Request, status);
		WdfRequestComplete(Request, status);
		return;
	}

	for (int i = 0; i < (int)urb->UrbIsochronousTransfer.NumberOfPackets; ++i)
	{
		int dtlen = 0x800;
		ULONG offset = urb->UrbIsochronousTransfer.IsoPacket[i].Offset;
		UCHAR* buf = vBuf + offset;
		int r = 0;
		
		int len = min(usbContext->audioData.frame_length - usbContext->audioData.frame_pos, dtlen); ///

#if 0
		RtlCopyMemory((UCHAR*)(buf), usbContext->audioData.frame_buf + usbContext->audioData.frame_pos, len);
#else
		static int colorToFill = 0;
		RtlFillMemory((UCHAR*)(buf), len, ++colorToFill);
#endif

		usbContext->audioData.frame_pos += len; ////
		

		r =  len;

		urb->UrbIsochronousTransfer.IsoPacket[i].Length = len;
		transfered += r;

		if (usbContext->audioData.frame_pos >= usbContext->audioData.frame_length) { ///帧结束

			break;
		}

		//DbgPrint("IsoPacket[%d].Offset:%d,Length:%d]\n", i, urb->UrbIsochronousTransfer.IsoPacket[i].Offset,  urb->UrbIsochronousTransfer.IsoPacket[i].Length);
	}

	UdecxUrbSetBytesCompleted(Request, transfered);
	UdecxUrbCompleteWithNtStatus(Request, 0);

}
#endif

VOID DpcCompleteVidoeIsoUrb(
	IN WDFDPC Dpc
)
{
	PUSB_CONTEXT usbContext;
	WDFREQUEST Request;
	NTSTATUS status = STATUS_SUCCESS;

	usbContext = WdfDeviceGetUsbContext(WdfDpcGetParentObject(Dpc));

	PUCHAR vBuf;
	ULONG vBufLen = 0;
	ULONG transfered = 0;
	WDF_REQUEST_PARAMETERS  requestParams;

	Request = usbContext->currentRequest;


	WDF_REQUEST_PARAMETERS_INIT(&requestParams);
	WdfRequestGetParameters(Request, &requestParams);


	PURB urb = (PURB)requestParams.Parameters.Others.Arg1;

	if (urb->UrbHeader.Function != URB_FUNCTION_ISOCH_TRANSFER)
	{
		DbgPrint("Incorrect URB\n");
		status = STATUS_INVALID_PARAMETER;
		WdfRequestComplete(Request, status);
		return;
	}

	if (usbContext->frameData.frame_buf == NULL)
	{
		//PRINTLINE("E");
		UdecxUrbSetBytesCompleted(Request, 0);
		UdecxUrbCompleteWithNtStatus(Request, 0);
		return;
	}

	status = UdecxUrbRetrieveBuffer(Request, &vBuf, &vBufLen);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("WdfRequest %p unable to retrieve buffer %d", Request, status);
		WdfRequestComplete(Request, status);
		return;
	}


	//urb->UrbHeader
	/*DbgPrint("URB TransferFlags:%u,urb->UrbIsochronousTransfer.NumberOfPackets:%u,TransferBufferLength:%u,packet len:%u\n",
				urb->UrbIsochronousTransfer.TransferFlags,
				urb->UrbIsochronousTransfer.NumberOfPackets,
				urb->UrbIsochronousTransfer.TransferBufferLength,
		urb->UrbIsochronousTransfer.IsoPacket[1].Offset);*/
	int DISTANCE = urb->UrbIsochronousTransfer.IsoPacket[1].Offset;
	for (int i = 0; i < (int)urb->UrbIsochronousTransfer.NumberOfPackets; ++i)
	{
		int dtlen = DISTANCE;
		ULONG offset = urb->UrbIsochronousTransfer.IsoPacket[i].Offset;
		UCHAR* buf = vBuf + offset;
		int r = 0;
		int data_len = dtlen - VID_HDR_LEN;

		UCHAR* vid_hdr = buf;

		//refer to UVC1.5 doc:  2.4.3.3 Video and Still Image Payload Headers 
		memset(vid_hdr, 0, VID_HDR_LEN); ////
		vid_hdr[0] = VID_HDR_LEN;

		vid_hdr[1] |= usbContext->frameData.frame_flip; ////
		vid_hdr[1] |= UVC_STREAM_EOH; ///

		if (usbContext->frameData.needChangeFormat)
		{
			//refer to 2.4.3.6 Device Initiated Dynamic Format Change Support
			//vid_hdr[1] |= UVC_STREAM_ERR;
			//urb->UrbIsochronousTransfer.IsoPacket[i].Length = VID_HDR_LEN;
			//transfered += VID_HDR_LEN;
			//continue;
			//break;
		}

		int len = min(usbContext->frameData.frame_length - usbContext->frameData.frame_pos, data_len); ///

#if 1
		RtlCopyMemory((UCHAR*)(vid_hdr + 2), usbContext->frameData.frame_buf + usbContext->frameData.frame_pos, len);
#else
		static int colorToFill = 0;
		RtlFillMemory((UCHAR*)(vid_hdr + 2), len, ++colorToFill);
#endif

		usbContext->frameData.frame_pos += len; ////
		//DbgPrint("frame_length:%d,frame_pos:%d,frame_flip:%d,len:%d,data_len:%d\n", usbContext->frameData.frame_length, usbContext->frameData.frame_pos, usbContext->frameData.frame_flip,len, data_len);
		if (usbContext->frameData.frame_pos >= usbContext->frameData.frame_length) { ///帧结束
			vid_hdr[1] |= UVC_STREAM_EOF;
			usbContext->frameData.frame_flip = !usbContext->frameData.frame_flip; //翻转
		}

		r = VID_HDR_LEN + len;

		urb->UrbIsochronousTransfer.IsoPacket[i].Length = r;
		transfered += r;

		if (usbContext->frameData.frame_pos >= usbContext->frameData.frame_length) { ///帧结束
			break;
		}

		//DbgPrint("IsoPacket[%d].Offset:%d,Length:%d]\n", i, urb->UrbIsochronousTransfer.IsoPacket[i].Offset,  urb->UrbIsochronousTransfer.IsoPacket[i].Length);
	}

	UdecxUrbSetBytesCompleted(Request, transfered);
	UdecxUrbCompleteWithNtStatus(Request, 0);

}



NTSTATUS processVideo(PUSB_CONTEXT usbContext)
{
	NTSTATUS status = STATUS_SUCCESS;

	WDF_IO_QUEUE_STATE state = WdfIoQueueAcceptRequests;
	ULONG   QueueRequests = 0;
	ULONG   DriverRequests = 0;

	static int timeout = 0;

	if (usbContext->frameData.frame_pos >= usbContext->frameData.frame_length || usbContext->frameData.frame_buf == NULL || ++timeout > 3000)
	{
		if (usbContext->currentVideoRequest != NULL)
		{
			usbContext->frameData.frame_buf = NULL;
			//PRINTLINE("WdfRequestComplete frame data");
			WdfRequestComplete(usbContext->currentVideoRequest, 0);
			usbContext->currentVideoRequest = 0;
			timeout = 0;
			return 0;
		}
			
		state = WdfIoQueueGetState(usbContext->VideoQueue, &QueueRequests, &DriverRequests);
		//DbgPrint("state:%d,QueueRequests:%d\n", state & WdfIoQueueNoRequests, QueueRequests);
		if (!(state & WdfIoQueueNoRequests))
		{
			//WDFREQUEST request;
			PVOID inBuf = NULL;

			status = WdfIoQueueRetrieveNextRequest(usbContext->VideoQueue, &usbContext->currentVideoRequest);
			if (!NT_SUCCESS(status)) {
				//discard
				PRINTLINE("Get video request failed");
				WdfRequestComplete(usbContext->currentVideoRequest, STATUS_INVALID_BUFFER_SIZE);
				return status;
			}

			if (timeout > 30)
			{
				timeout = 0;
				PRINTLINE("Discard frame data");
				WdfRequestComplete(usbContext->currentVideoRequest, 0);
				usbContext->currentVideoRequest = NULL;
				return 0;
			}

			status = WdfRequestRetrieveInputBuffer(usbContext->currentVideoRequest, usbContext->frameData.frame_length, &inBuf, NULL);
			if (!NT_SUCCESS(status)) {
				PRINTLINE("Get video buffer failed");
				WdfRequestComplete(usbContext->currentVideoRequest, status);
				return status;
			}

			usbContext->frameData.frame_buf = (UCHAR*)inBuf;
			usbContext->frameData.frame_pos = 0; //从新开始
		}
	}

	if (usbContext->IsoVideoInQueue == NULL )
	{
		//PRINTLINE("E");
		return 0;
	}

	status = WdfIoQueueRetrieveNextRequest(usbContext->IsoVideoInQueue, &usbContext->currentRequest);
	if (!NT_SUCCESS(status)) {
		return status;
	}

	bool ret = WdfDpcEnqueue(usbContext->completeUrbDpc);
	if (!ret)
	{
		PRINTLINE("add dpc failed");
	}

	return 0;
}


#if 0
NTSTATUS processAudio(PUSB_CONTEXT usbContext)
{
	NTSTATUS status = STATUS_SUCCESS;
	WDF_IO_QUEUE_STATE state = WdfIoQueueAcceptRequests;
	ULONG   QueueRequests = 0;
	ULONG   DriverRequests = 0;


	PRINTLINE("Enter");
	if (usbContext->audioData.frame_pos >= usbContext->audioData.frame_length)
	{
		//usbContext->frameData.timeout = 0;
		state = WdfIoQueueGetState(usbContext->AudioQueue, &QueueRequests, &DriverRequests);
		//DbgPrint("state:%d,QueueRequests:%d\n", state & WdfIoQueueNoRequests, QueueRequests);
		if (!(state & WdfIoQueueNoRequests))
		{
			WDFREQUEST request;
			PVOID inBuf = NULL;

			status = WdfIoQueueRetrieveNextRequest(usbContext->AudioQueue, &request);
			if (!NT_SUCCESS(status)) {
				//discard
				PRINTLINE("Get audio request failed");
				WdfRequestComplete(request, STATUS_INVALID_BUFFER_SIZE);
				return status;
			}

			status = WdfRequestRetrieveInputBuffer(request, usbContext->audioData.frame_length, &inBuf, NULL);
			if (!NT_SUCCESS(status)) {
				PRINTLINE("Get video buffer failed");
				WdfRequestComplete(request, status);
				return status;
			}

			RtlCopyMemory(usbContext->audioData.frame_buf, inBuf, usbContext->audioData.frame_length);
			WdfRequestComplete(request, 0);
		}
		//PRINTLINE("Enter");
		usbContext->audioData.frame_pos = 0; //从新开始

	}

	if (usbContext->IsoAudioInQueue == NULL)
	{
		return 0;
	}

	//state = WdfIoQueueGetState(usbContext->IsoVideoInQueue, &QueueRequests, &DriverRequests);

	//DbgPrint("Request coming: queue ready:%d,QueueRequests:%d,DriverRequests:%d\n", WDF_IO_QUEUE_READY(state), QueueRequests, DriverRequests);
	status = WdfIoQueueRetrieveNextRequest(usbContext->IsoAudioInQueue, &usbContext->currentAudioRequest);
	if (!NT_SUCCESS(status)) {
		return status;
	}

	bool ret = WdfDpcEnqueue(usbContext->completeAudioUrbDpc);
	if (!ret)
	{
		PRINTLINE("add dpc failed");
	}

	return 0;
}
#endif

void EvtIsochTimer(WDFTIMER Timer)
{
	PUSB_CONTEXT usbContext;
	

	usbContext = WdfDeviceGetUsbContext(WdfTimerGetParentObject(Timer));

	processVideo(usbContext);

#if 0
	NTSTATUS status = STATUS_SUCCESS;
	if (usbContext->InterruptInQueue == NULL)
	{
		//PRINTLINE("interrupt queue is empty");
		goto exit;
	}
		
	WDFREQUEST request;
	status = WdfIoQueueRetrieveNextRequest(usbContext->InterruptInQueue, &request);
	if (!NT_SUCCESS(status)) {
		//PRINTLINE("interrupt queue no request");
		goto exit;
	}

	PRINTLINE("interrupt request coming###########");

		//process here
exit:
#endif

	WdfTimerStart(usbContext->isoTransferTimer, WDF_REL_TIMEOUT_IN_MS(usbContext->frameData.iso_timer_interval));

}


VOID EVT_IO_PTZ_QUEUE_IO_CANCELED_ON_QUEUE(
	_In_
	WDFQUEUE Queue,
	_In_
	WDFREQUEST Request
)
{
	UNREFERENCED_PARAMETER(Queue);
	WdfRequestComplete(Request, STATUS_CANCELLED);
}