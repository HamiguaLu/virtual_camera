

#include "pch.h"

#include "driver.h"


//
// This variable was used by the USB3 kernel debug extension.
//
PDRIVER_OBJECT  g_UdecxCamDriverObject;

PWDFDRIVER_UDECXMBIM_CONTEXT g_WdfDriverUdecxCamCtx = NULL;

extern "C"
NTSTATUS
DriverEntry(
    struct _DRIVER_OBJECT   *DriverObject,
    PUNICODE_STRING         RegistryPath
    )
/*++

Routine Description:

    DriverEntry is the first routine called after a driver is loaded,
    and is responsible for initializing the driver.

--*/
{
    NTSTATUS                            status;
    WDFDRIVER                           wdfDriver;
    WDF_OBJECT_ATTRIBUTES               wdfAttributes;
    WDF_DRIVER_CONFIG                   wdfDriverConfig;

    PAGED_CODE();

    g_UdecxCamDriverObject = DriverObject;

    PRINTLINE("Enter");


    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&wdfAttributes, WDFDRIVER_UDECXMBIM_CONTEXT);


    wdfAttributes.EvtCleanupCallback = DriverCleanup;

    WDF_DRIVER_CONFIG_INIT(&wdfDriverConfig, &ControllerWdfEvtDeviceAdd);
    wdfDriverConfig.DriverPoolTag = UDECXVDEV_POOL_TAG;

    status = WdfDriverCreate(DriverObject,
                                RegistryPath,
                                &wdfAttributes,
                                &wdfDriverConfig,
                                &wdfDriver);

    if (!NT_SUCCESS(status)) {
        DbgPrint("DriverEntry failed");
        //
        // DriverCleanup will not be called since the WdfDriver object
        // failed to create. Clean-up any resources that were created
        // prior to WDF driver creation.
        //
        //WPP_CLEANUP(DriverObject);
        return status;
    }

    //
    // Initialize the global controller list.
    //
    g_WdfDriverUdecxCamCtx = WdfDriverGetUdecxCamCtx(wdfDriver);
    //InitializeListHead(&g_WdfDriverUdecxCamCtx->ControllerListHead);
    //KeInitializeSpinLock(&g_WdfDriverUdecxCamCtx->ControllerListLock);
    //g_WdfDriverUdecxCamCtx->ControllerListCount = 0;

    return status;
}

#if 1
VOID
DriverCleanup(
    WDFOBJECT   WdfDriver
    )
/*++

Routine Description:

    The driver's EvtCleanupCallback event callback function removes the driver's
    references on an object so that the object can be deleted.

--*/
{
    PAGED_CODE();

    PRINTLINE("Enter");
    //WPP_CLEANUP(WdfDriverWdmGetDriverObject((WDFDRIVER)WdfDriver));

    UNREFERENCED_PARAMETER(WdfDriver);

    return;
}

#endif
