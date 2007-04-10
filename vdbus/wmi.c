//******************************************************************************

#include "vdbus.h"

#define tst_BUG_CHECK_FILEID_d tst_BUG_CHECK_FILEID_WMI_d

//============================================================================//
NTSTATUS
Bus_WmiRegistration(WDFDEVICE Device)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    WDF_WMI_PROVIDER_CONFIG providerConfig;
    WDF_WMI_INSTANCE_CONFIG instanceConfig;
    PFDO_DEVICE_DATA deviceData;

    DECLARE_CONST_UNICODE_STRING(szuBusRName, tst_WSZ_BUSRESOURCENAME_d);

    tst_FUNCIN_m();

    tst_TRY_m(ntStatus)
    {
        deviceData = BusGetFDODevExtData(Device);

        // Register WMI classes.
        // First specify the resource name which contain the binary mof resource.
        ntStatus = WdfDeviceAssignMofResourceName(Device, &szuBusRName);
        if (!NT_SUCCESS(ntStatus)) 
        {
            tst_ERR_m((tst_ERR_PARAMS_d, tst_XY_ERRMSG_d,
            //  012345678901234567890123456789012345678901234567890123456        
            //  Failed in WdfDeviceAssignMofResourceName with ntSts=0x%x. 
                "%x", 
                ntStatus));
            goto cleanup;
        };

        WDF_WMI_PROVIDER_CONFIG_INIT(&providerConfig, &VDBUS_WMI_STD_DATA_GUID);
        providerConfig.MinInstanceBufferSize = sizeof(VDBUS_WMI_STD_DATA);

        // You would want to create a WDFWMIPROVIDER handle separately if you are
        // going to dynamically create instances on the provider.  Since we are
        // statically creating one instance, there is no need to create the provider
        // handle.
        WDF_WMI_INSTANCE_CONFIG_INIT_PROVIDER_CONFIG(&instanceConfig, &providerConfig);

        // By setting Regsiter to TRUE, we tell the framework to create a provider
        // as part of the Instance creation call. This eliminates the need to
        // call WdfWmiProviderRegister.
        instanceConfig.Register = TRUE;
        instanceConfig.EvtWmiInstanceQueryInstance = Bus_EvtStdDataQueryInstance;
        instanceConfig.EvtWmiInstanceSetInstance = Bus_EvtStdDataSetInstance;
        instanceConfig.EvtWmiInstanceSetItem = Bus_EvtStdDataSetItem;

        ntStatus = WdfWmiInstanceCreate(Device, &instanceConfig, 
            WDF_NO_OBJECT_ATTRIBUTES, WDF_NO_HANDLE);
        if (NT_SUCCESS(ntStatus)) 
        {
            deviceData->StdVDBusData.ErrorCount = 0;
        };
    }
    tst_CATCH_m(ntStatus);

cleanup:
    tst_FUNCOUT_m();

    return ntStatus;
}
//============================================================================//

//============================================================================//
NTSTATUS
Bus_EvtStdDataSetItem(IN WDFWMIINSTANCE WmiInstance, IN ULONG DataItemId,
                      IN ULONG InBufferSize, IN PVOID InBuffer)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PFDO_DEVICE_DATA fdoData;

    PAGED_CODE();

    tst_FUNCIN_m();

    tst_TRY_m(ntStatus)
    {
        fdoData = BusGetFDODevExtData(WdfWmiInstanceGetDevice(WmiInstance));

        // TODO: Use generated header's #defines for constants and sizes
        // (for the remainder of the file)
        if (DataItemId == 2) 
        {
            if (InBufferSize < sizeof(ULONG)) 
            {
                ntStatus = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                // VDBusDebugLevel = fdoData->StdVDBusData.DebugPrintLevel = *((PULONG) InBuffer);
            };
        }
        else
        {
            // All other fields are read only
            ntStatus = STATUS_WMI_READ_ONLY;
        };
    }
    tst_CATCH_m(ntStatus);

    tst_FUNCOUT_m();

    return ntStatus;
}
//============================================================================//

//============================================================================//
NTSTATUS
Bus_EvtStdDataSetInstance(IN WDFWMIINSTANCE WmiInstance, IN ULONG InBufferSize,
                          IN PVOID InBuffer)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PFDO_DEVICE_DATA fdoData;

    UNREFERENCED_PARAMETER(InBufferSize);

    PAGED_CODE();

    tst_FUNCIN_m();

    tst_TRY_m(ntStatus)
    {
        fdoData = BusGetFDODevExtData(WdfWmiInstanceGetDevice(WmiInstance));

        // We will update only writable elements.
        // VDBusDebugLevel = fdoData->StdVDBusData.DebugPrintLevel =
        //    ((PVDBUS_WMI_STD_DATA) InBuffer)->DebugPrintLevel;
    }
    tst_CATCH_m(ntStatus);

    tst_FUNCOUT_m();

    return ntStatus;
}
//============================================================================//

//============================================================================//
NTSTATUS
Bus_EvtStdDataQueryInstance(IN WDFWMIINSTANCE WmiInstance, IN ULONG OutBufferSize,
                            IN PVOID OutBuffer, OUT PULONG BufferUsed)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PFDO_DEVICE_DATA fdoData;

    UNREFERENCED_PARAMETER(OutBufferSize);

    PAGED_CODE();

    tst_FUNCIN_m();

    tst_TRY_m(ntStatus)
    {
        fdoData = BusGetFDODevExtData(WdfWmiInstanceGetDevice(WmiInstance));

        *BufferUsed = sizeof(VDBUS_WMI_STD_DATA);
        *(PVDBUS_WMI_STD_DATA) OutBuffer = fdoData->StdVDBusData;
    }
    tst_CATCH_m(ntStatus);

    tst_FUNCOUT_m();

    return ntStatus;
}
//============================================================================//

// End Of File
