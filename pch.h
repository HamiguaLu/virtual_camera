/*++
Copyright (c) Microsoft Corporation

Module Name:

    pch.h

Abstract:


--*/

#ifndef __PCH_H__
#define __PCH_H__

//
// Device is using simple endpoints
//

//
// Device described as 3.0 device
//

#define USB20_COMPOSITE


#ifdef __cplusplus
extern "C" {
#endif

#include <initguid.h>

#include <ntddk.h>
#include <wdf.h>

#include <usb.h>
#include <Wdfusb.h>

#include <ntstrsafe.h>
#include <ntintsafe.h>
#include <driverspecs.h>
#include <limits.h>

//#include <wdfcx.h>
//#include <devpkey.h>
#include <acpiioct.h>
#include <wdmguid.h>
#include <WppRecorder.h>

#include <usb.h>

#pragma warning(push)
#include <wdfusb.h>
#pragma warning(pop)
//#include "tracing.h"
#include "ucx\1.5\ucxclass.h"

#include <windef.h>

#ifdef __cplusplus
} // extern "C"
#endif


const UCHAR g_LanguageDescriptor[] = { 4,3,9,4 };
const USHORT AMERICAN_ENGLISH = 0x409;

const UCHAR g_ManufacturerIndex = 1;
DECLARE_CONST_UNICODE_STRING(g_ManufacturerStringEnUs, L"PTZoptics");

const UCHAR g_ProductIndex = 2;
DECLARE_CONST_UNICODE_STRING(g_ProductStringEnUs, L"PTZOpticsNDIVirtualWebcam");


#include "ude/1.0/udecx.h"

#include "usbdevice.h"
#include "controller.h"
#include "driver.h"
#include "io.h"


#define  PRINTLINE(s)  DbgPrint("%s:%d %s\n",__func__,__LINE__,s)

#endif