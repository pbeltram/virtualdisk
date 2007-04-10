/*
Copyright 2007 Primoz Beltram

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

//******************************************************************************

// Standard Includes
// -----------------
#include <stdarg.h>
#include <stdio.h>

#include "vdbus.h"

#define tst_BUG_CHECK_FILEID_d tst_BUG_CHECK_FILEID_MAIN_d

//============================================================================//
#define sz_SYSTEM_PROCESS_NAME_d "System"
ULONG g_ulProcessNameOffset = 0; // Process name offset.
PEPROCESS g_pSystemProcess = NULL; // System process.

//  Holds pointer to the driver object for this driver
PDRIVER_OBJECT g_pDriverObject = NULL;

// Tock tracing buffer.
WDFSPINLOCK g_mTraceLogLock;
CHAR g_szTraceLogBuffer[512];

ULONG g_ulDebug =  // Default tracing flags 0x03003001
    (tst_DBG_LOGWARN_d | tst_DBG_LOGERR_d | 
     tst_DBG_WARN_d | tst_DBG_ERR_d |
     tst_DBG_MAIN_d);

CHAR g_szBuildDateTime[128];

ULONG g_ulFncInOut = 0; // Function IN/OUT counter.
//============================================================================//

//============================================================================//
NTSTATUS
BusGetProcessNameOffset(VOID)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG i;
   
    tst_FUNCIN_m();

    tst_TRY_m(ntStatus)
    {
        // NOTE: Assumption: Driver load function is allways called in context of System process.
        g_ulProcessNameOffset = 0;
        g_pSystemProcess = PsGetCurrentProcess();

        // Scan for 12KB, hoping the KPEB never grows that big!
        for (i = 0; i < 3 * PAGE_SIZE; i++ ) 
        {
            if (0 == _strnicmp(sz_SYSTEM_PROCESS_NAME_d, (PCHAR) g_pSystemProcess + i, sizeof(sz_SYSTEM_PROCESS_NAME_d)-1)) 
            {
                g_ulProcessNameOffset = i;
                break;
            };
        };

        if (0 == g_ulProcessNameOffset)
        {
            tst_ERR_m((tst_ERR_PARAMS_d, tst_XY_ERRMSG_d,
            //  012345678901234567890123456789012345678901234567890123456        
            //  Can't find process name offset (%s,0x%x). 
                "%s|%x", 
                sz_SYSTEM_PROCESS_NAME_d, g_pSystemProcess));

            ntStatus = STATUS_UNSUCCESSFUL;
        };
    }
    tst_CATCH_m(ntStatus);

    tst_FUNCOUT_m();

    return ntStatus;
}
//============================================================================//

//============================================================================//
NTSTATUS
DriverEntry(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pszuRegistryPath)
{
    WDF_DRIVER_CONFIG sConfig;
    NTSTATUS ntStatus;
    WDFDRIVER pDriver;
    WDF_OBJECT_ATTRIBUTES driver_Attributes;

    ntStatus = WdfSpinLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &g_mTraceLogLock);
    if (!NT_SUCCESS(ntStatus))
    {
        DbgPrint("[VDBUS] Failed in WdfSpinLockCreate(g_mTraceLogLock) with ntSts=0x%x.\n", 
            ntStatus);

        goto cleanup;
    };

    //  Save our Driver Object.
    g_pDriverObject = pDriverObject;

    RtlStringCchPrintfA(g_szBuildDateTime, sizeof(g_szBuildDateTime)-1, "Bld(%s %s)-%s", 
        __DATE__, __TIME__, (0 != DBG) ? "DEBUG":"RELEASE");

    tst_TRC_m(tst_DBG_MAIN_d, (tst_TRC_PARAMS_d, 
        "%s. LogFlgs(0x%x).RegPath(%wZ).", 
        g_szBuildDateTime, g_ulDebug, pszuRegistryPath));

    // Get global process name offset.
    ntStatus = BusGetProcessNameOffset();
    if (!NT_SUCCESS(ntStatus))
    {
        goto cleanup;
    };
    // Initialize driver config to control the attributes that
    // are global to the driver. Note that framework by default
    // provides a driver unload routine. If you create any resources
    // in the DriverEntry and want to be cleaned in driver unload,
    // you can override that by specifing one in the Config structure.
    WDF_DRIVER_CONFIG_INIT(&sConfig, Bus_EvtDriverDeviceAdd);
    sConfig.EvtDriverUnload = Bus_EvtDriverUnload;

    WDF_OBJECT_ATTRIBUTES_INIT(&driver_Attributes);
    driver_Attributes.SynchronizationScope = WdfSynchronizationScopeDevice;
    driver_Attributes.ExecutionLevel = WdfExecutionLevelPassive;
    // Create a framework driver object to represent our driver.

    ntStatus = WdfDriverCreate(pDriverObject, pszuRegistryPath, 
        &driver_Attributes, &sConfig, &pDriver);
    if (!NT_SUCCESS(ntStatus)) 
    {
        tst_ERR_m((tst_ERR_PARAMS_d, tst_XY_ERRMSG_d,
        //  012345678901234567890123456789012345678901234567890123456        
        //  WdfDriverCreate failed with ntSts=0x%x.
            "%x",
            ntStatus));

        goto cleanup;
    };

    // Log to SystemEvent log that we are running.
    tst_LOG_m((tst_LOG_PARAMS_d, tst_XY_LOGMSG_d,
    //  012345678901234567890123456789012345678901234567890123456        
    //  WdfDriver(0x%x) is running. DebugLevel(0x%x).BuildInfo(%s).
        "%x|%x|%s",
        pDriver, g_ulDebug, g_szBuildDateTime));

    tst_LOG_m((tst_LOG_PARAMS_d, tst_XY_LOGMSG_d,
    //  012345678901234567890123456789012345678901234567890123456        
    //  Driver version: (%s).
        "%s", 
        tst_VER_PRODUCTVERSION_STR_d " (" tst_VERSION_LABEL_d ")"));

cleanup:
    return ntStatus;
}
//============================================================================//

//============================================================================//
NTSTATUS
Bus_EvtDriverDeviceAdd(IN WDFDRIVER pDriver, IN PWDFDEVICE_INIT pDeviceInit)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    WDF_CHILD_LIST_CONFIG config;
    WDF_OBJECT_ATTRIBUTES fdo_Attributes;
    WDFDEVICE device;
    WDF_IO_QUEUE_CONFIG queueConfig;
    PNP_BUS_INFORMATION busInfo;
    PFDO_DEVICE_DATA deviceData;
    WDFQUEUE queue;
    WDF_OBJECT_ATTRIBUTES queue_Attributes;

    UNREFERENCED_PARAMETER(pDriver);

    PAGED_CODE();

    tst_FUNCIN_m();

    tst_TRY_m(ntStatus)
    {
        tst_TRC_m(tst_DBG_MAIN_d, (tst_TRC_PARAMS_d, 
            "PNP add FDO device for WDFDRIVER(0x%x)PWDFDEVICE_INIT(0x%x).", 
            pDriver, pDeviceInit));

        // Initialize all the properties specific to the device.
        // Framework has default values for the one that are not
        // set explicitly here. So please read the doc and make sure
        // you are okay with the defaults.
        WdfDeviceInitSetDeviceType(pDeviceInit, FILE_DEVICE_BUS_EXTENDER);
        WdfDeviceInitSetExclusive(pDeviceInit, TRUE);
        WdfDeviceInitSetIoType(pDeviceInit, WdfDeviceIoDirect);

        // Since this is pure software bus enumerator, we don't have to register for
        // any PNP/Power callbacks. Framework will take the default action for
        // all the PNP and Power IRPs.

        // WDF_ DEVICE_LIST_CONFIG describes how the framework should handle
        // dynamic child enumeration on behalf of the driver writer.
        // Since we are a bus driver, we need to specify identification description
        // for our child devices. This description will serve as the identity of our
        // child device. Since the description is opaque to the framework, we
        // have to provide bunch of callbacks to compare, copy, or free
        // any other resources associated with the description.
        WDF_CHILD_LIST_CONFIG_INIT(&config, sizeof(PDO_IDENTIFICATION_DESCRIPTION), 
            Bus_EvtChildListCreatePDO);

        // This function pointer will be called when the framework needs to copy a
        // identification description from one location to another.  An implementation
        // of this function is only necessary if the description contains description
        // relative pointer values (like  LIST_ENTRY for instance) .
        // If set to NULL, the framework will use RtlCopyMemory to copy an identification .
        // description. In this sample, it's not required to provide these callbacks. 
        // they are added just for illustration.
        config.EvtChildListIdentificationDescriptionDuplicate = Bus_EvtChildListDuplicate;

        // This function pointer will be called when the framework needs to compare 
        // two identificaiton descriptions.  If left NULL a call to RtlCompareMemory
        // will be used to compare two identificaiton descriptions.
        config.EvtChildListIdentificationDescriptionCompare = Bus_EvtChildListCompare;

        // This function pointer will be called when the framework needs to free a 
        // identification description.  An implementation of this function is only
        // necessary if the description contains dynamically allocated memory
        // (by the driver writer) that needs to be freed. The actual identification
        // description pointer itself will be freed by the framework.
        config.EvtChildListIdentificationDescriptionCleanup = Bus_EvtChildListCleanup;

        // Tell the framework to use the built-in childlist to track the state
        // of the device based on the configuration we just created.
        WdfFdoInitSetDefaultChildListConfig(pDeviceInit, &config, WDF_NO_OBJECT_ATTRIBUTES);

        // Initialize attributes structure to specify size and accessor function
        // for storing device context.
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&fdo_Attributes, FDO_DEVICE_DATA);
        fdo_Attributes.SynchronizationScope = WdfSynchronizationScopeInheritFromParent;
        fdo_Attributes.ExecutionLevel = WdfExecutionLevelInheritFromParent;

        // Create a framework device object. In response to this call, framework
        // creates a WDM deviceobject and attach to the PDO.
        ntStatus = WdfDeviceCreate(&pDeviceInit, &fdo_Attributes, &device);
        if (!NT_SUCCESS(ntStatus)) 
        {
            tst_ERR_m((tst_ERR_PARAMS_d, tst_XY_ERRMSG_d,
            //  012345678901234567890123456789012345678901234567890123456        
            //  Error creating device with ntSts=0x%x.
                "%x",
                ntStatus));

            goto cleanup;
        };
        tst_TRC_m(tst_DBG_MAIN_d, (tst_TRC_PARAMS_d, 
            "Created FDO_WDFDEVICE(0x%x)PDO(0x%x)FDO(0x%x).", 
            device, WdfDeviceWdmGetPhysicalDevice(device), WdfDeviceWdmGetDeviceObject(device)));

        // Configure a default queue so that requests that are not
        // configure-fowarded using WdfDeviceConfigureRequestDispatching to goto
        // other queues get dispatched here.
        WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchParallel);
        queueConfig.EvtIoDeviceControl = BusFDO_EvtIoDeviceControl;

        WDF_OBJECT_ATTRIBUTES_INIT(&queue_Attributes);
        queue_Attributes.SynchronizationScope = WdfSynchronizationScopeInheritFromParent;
        queue_Attributes.ExecutionLevel = WdfExecutionLevelInheritFromParent;

        ntStatus = WdfIoQueueCreate(device, &queueConfig, &queue_Attributes, &queue);
        if (!NT_SUCCESS(ntStatus)) 
        {
            tst_ERR_m((tst_ERR_PARAMS_d, tst_XY_ERRMSG_d,
            //  012345678901234567890123456789012345678901234567890123456        
            //  WdfIoQueueCreate failed with ntSts=0x%x.
                "%x",
                ntStatus));

            goto cleanup;
        };

        tst_TRC_m(tst_DBG_MAIN_d, (tst_TRC_PARAMS_d, 
            "Created WDFQUEUE0(0x%x) for FDO_WDFDEVICE(0x%x).", 
            queue, WdfIoQueueGetDevice(queue)));

        // Get the device context.
        deviceData = BusGetFDODevExtData(device);

        // Bus number.
        deviceData->ucBusNumber = tst_BUSNUMBER_d;

        // Init STORAGE_ADAPTER_DESCRIPTOR info.
        RtlZeroMemory(&deviceData->sBusDesc, sizeof(deviceData->sBusDesc));
        deviceData->sBusDesc.Version = sizeof(STORAGE_ADAPTER_DESCRIPTOR);
        deviceData->sBusDesc.Size = sizeof(deviceData->sBusDesc);
        deviceData->sBusDesc.MaximumTransferLength = 0x100000;
        deviceData->sBusDesc.MaximumPhysicalPages= 0x101;
        deviceData->sBusDesc.AlignmentMask = 0x3;
        deviceData->sBusDesc.AdapterUsesPio = FALSE;
        deviceData->sBusDesc.AdapterScansDown = FALSE;
        deviceData->sBusDesc.CommandQueueing= FALSE;
        deviceData->sBusDesc.AcceleratedTransfer= TRUE;
        deviceData->sBusDesc.BusType = BusTypeScsi;
        deviceData->sBusDesc.BusMajorVersion= tst_VERSION_MAJOR_d;
        deviceData->sBusDesc.BusMinorVersion = tst_VERSION_MINOR_d;

        // Create device interface for this device. The interface will be
        // enabled by the framework when we return from StartDevice successfully.
        // Clients of this driver will open this interface and send ioctls.
        // No Reference String. If you provide one it will appended to the 
        // symbolic link. Some drivers register multiple interfaces for the same device
        // and use the reference string to distinguish between them
        ntStatus = WdfDeviceCreateDeviceInterface(device, &tst_GUID_DEVINTERFACE_VDBUS_d, NULL);   
        if (!NT_SUCCESS(ntStatus)) 
        {
            tst_ERR_m((tst_ERR_PARAMS_d, tst_XY_ERRMSG_d,
            //  012345678901234567890123456789012345678901234567890123456        
            //  WdfDeviceCreateDeviceInterface failed with ntSts=0x%x.
                "%x",
                ntStatus));

            goto cleanup;
        };

        // This value is used in responding to the IRP_MN_QUERY_BUS_INFORMATION 
        // for the child devices. This is an optional information provided to
        // uniquely idenitfy the bus the device is connected. 
        busInfo.BusTypeGuid = GUID_DEVCLASS_VDBUS;
        busInfo.LegacyBusType = PNPBus;
        busInfo.BusNumber = 0;
        WdfDeviceSetBusInformationForChildren(device, &busInfo);

        ntStatus = Bus_WmiRegistration(device);
        if (!NT_SUCCESS(ntStatus)) 
        {
            tst_ERR_m((tst_ERR_PARAMS_d, tst_XY_ERRMSG_d,
            //  012345678901234567890123456789012345678901234567890123456        
            //  Bus_WmiRegistration failed with ntSts=0x%x.
                "%x",
                ntStatus));

            goto cleanup;
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
Bus_EvtDriverUnload(IN WDFDRIVER pDriver)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(pDriver);

    PAGED_CODE();

    tst_FUNCIN_m();

    tst_TRY_m(ntStatus)
    {
        tst_TRC_m(tst_DBG_MAIN_d, (tst_TRC_PARAMS_d, 
            "WDFDRIVER(0x%x) unload.", 
            pDriver));
    }
    tst_CATCH_m(ntStatus);

    tst_FUNCOUT_m();

    return ntStatus;
}
//============================================================================//

//============================================================================//
VOID
BusFDO_EvtIoDeviceControl(IN WDFQUEUE Queue, IN WDFREQUEST Request, 
                          IN size_t OutputBufferLength, IN size_t InputBufferLength,
                          IN ULONG IoControlCode)
{
    NTSTATUS ntStatus = STATUS_INVALID_PARAMETER;
    WDFDEVICE hDevice;
    PVDBUS_PLUGIN_HARDWARE plugIn = NULL;
    PVDBUS_UNPLUG_HARDWARE unPlug = NULL;
    PVDBUS_EJECT_HARDWARE eject = NULL;
    size_t length = 0;

    UNREFERENCED_PARAMETER(OutputBufferLength);

    PAGED_CODE ();

    tst_FUNCIN_m();

    tst_TRY_m(ntStatus)
    {
        hDevice = WdfIoQueueGetDevice(Queue);

        tst_TRC_m(tst_DBG_MAIN_d, (tst_TRC_PARAMS_d, 
            "EvtIoDeviceControl for FDO_WDFDEVICE(0x%x).WDFQUEUE(0x%x).IOCTL(0x%x,%d,%d)(0x%x,0x%x,0x%x,0x%x).", 
            hDevice, Queue, 
            IoControlCode, OutputBufferLength, InputBufferLength, 
            tst_DEVICE_FROM_CTL_CODE_m(IoControlCode), tst_FUNCTION_FROM_CTL_CODE_m(IoControlCode), 
            tst_METHOD_FROM_CTL_CODE_m(IoControlCode), tst_ACCESS_FROM_CTL_CODE_m(IoControlCode)));

        switch (IoControlCode) 
        {
            case tst_IOCTL_VDBUS_PLUGIN_HARDWARE_d:

                ntStatus = WdfRequestRetrieveInputBuffer(Request, 
                    sizeof(VDBUS_PLUGIN_HARDWARE)+(sizeof(UNICODE_NULL)*2), // 2 for double NULL termination(MULTI_SZ)
                    &plugIn, &length);
                if (!NT_SUCCESS(ntStatus)) 
                {
                    tst_ERR_m((tst_ERR_PARAMS_d, tst_XY_ERRMSG_d,
                    //  012345678901234567890123456789012345678901234567890123456        
                    //  WdfRequestRetrieveInputBuffer failed with ntSts=0x%x.
                        "%x",
                        ntStatus));
                    break;
                };
                tst_MUST_ASSERT_m((length == InputBufferLength), __LINE__, length, InputBufferLength, 0);

                if (sizeof(VDBUS_PLUGIN_HARDWARE) == plugIn->Size)
                {
                    length = (InputBufferLength - sizeof(VDBUS_PLUGIN_HARDWARE))/sizeof(WCHAR);
                    // Make sure the IDs is two NULL terminated.
                    if ((UNICODE_NULL != plugIn->HardwareIDs[length - 1]) ||
                        (UNICODE_NULL != plugIn->HardwareIDs[length - 2])) 
                    {
                        ntStatus = STATUS_INVALID_PARAMETER;
                        tst_ERR_m((tst_ERR_PARAMS_d, tst_XY_ERRMSG_d,
                        //  012345678901234567890123456789012345678901234567890123456        
                        //  Invalid MULTI_SZ HardwareID string termination. ntSts=0x%x.
                            "%x",
                            ntStatus));
                        break;
                    };

                    ntStatus = Bus_IoctlPlugInDevice(hDevice, plugIn->HardwareIDs, length, plugIn->SerialNo);
                };
                break;

            case tst_IOCTL_VDBUS_UNPLUG_HARDWARE_d:

                ntStatus = WdfRequestRetrieveInputBuffer(Request, sizeof(VDBUS_UNPLUG_HARDWARE), 
                    &unPlug, &length);
                if (!NT_SUCCESS(ntStatus)) 
                {
                    tst_ERR_m((tst_ERR_PARAMS_d, tst_XY_ERRMSG_d,
                    //  012345678901234567890123456789012345678901234567890123456        
                    //  WdfRequestRetrieveInputBuffer failed with ntSts=0x%x.
                        "%x",
                        ntStatus));
                    break;
                };

                if (unPlug->Size == InputBufferLength)
                {
                    ntStatus= Bus_IoctlUnPlugDevice(hDevice, unPlug->SerialNo );
                };
                break;

            case tst_IOCTL_VDBUS_EJECT_HARDWARE_d:

                ntStatus = WdfRequestRetrieveInputBuffer(Request, sizeof(VDBUS_EJECT_HARDWARE),
                    &eject, &length);
                if (!NT_SUCCESS(ntStatus)) 
                {
                    tst_ERR_m((tst_ERR_PARAMS_d, tst_XY_ERRMSG_d,
                    //  012345678901234567890123456789012345678901234567890123456        
                    //  WdfRequestRetrieveInputBuffer failed with ntSts=0x%x.
                        "%x",
                        ntStatus));
                    break;
                };

                if (eject->Size == InputBufferLength)
                {
                    ntStatus= Bus_IoctlEjectDevice(hDevice, eject->SerialNo);
                };
                break;

            default:
                tst_TRC_m(tst_DBG_MAIN_d, (tst_TRC_PARAMS_d, 
                    "Unknown IOCTL(0x%x)(0x%x,0x%x,0x%x,0x%x).", 
                    IoControlCode, tst_DEVICE_FROM_CTL_CODE_m(IoControlCode), tst_FUNCTION_FROM_CTL_CODE_m(IoControlCode), 
                    tst_METHOD_FROM_CTL_CODE_m(IoControlCode), tst_ACCESS_FROM_CTL_CODE_m(IoControlCode)));

                ntStatus = STATUS_INVALID_PARAMETER;
                break; 
        };
    }
    tst_CATCH_m(ntStatus);

    WdfRequestCompleteWithInformation(Request, ntStatus, length);

    tst_FUNCOUT_m();
}
//============================================================================//

//============================================================================//
NTSTATUS
Bus_IoctlPlugInDevice(IN WDFDEVICE Device, IN PWCHAR HardwareIds, 
                      IN size_t CchHardwareIds, IN ULONG SerialNo)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDO_IDENTIFICATION_DESCRIPTION description;

    PAGED_CODE ();

    tst_FUNCIN_m();

    tst_TRY_m(ntStatus)
    {
        // Initialize the description with the information about the newly
        // plugged in device.
        WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(&description.Header, sizeof(description));

        description.SerialNo = SerialNo;
        description.CchHardwareIds = CchHardwareIds;
        description.HardwareIds = HardwareIds;

        // Call the framework to add this child to the childlist. This call
        // will internaly call our DescriptionCompare callback to check
        // whether this device is a new device or existing device. If
        // it's a new device, the framework will call DescriptionDuplicate to create
        // a copy of this description in nonpaged pool.
        // The actual creation of the child device will happen when the framework
        // receives QUERY_DEVICE_RELATION request from the PNP manager in
        // response to InvalidateDeviceRelations call made as part of adding
        // a new child.
        ntStatus = WdfChildListAddOrUpdateChildDescriptionAsPresent(WdfFdoGetDefaultChildList(Device), 
            &description.Header, NULL); // AddressDescription 
        if (ntStatus == STATUS_OBJECT_NAME_EXISTS) 
        {
            // The description is already present in the list, the serial number is
            // not unique, return error.
            ntStatus = STATUS_INVALID_PARAMETER;
        };
    }
    tst_CATCH_m(ntStatus);

    tst_FUNCOUT_m();

    return ntStatus;
}
//============================================================================//

//============================================================================//
NTSTATUS
Bus_IoctlUnPlugDevice(WDFDEVICE Device, ULONG SerialNo)
{
    NTSTATUS  ntStatus = STATUS_SUCCESS;
    WDFCHILDLIST list;

    PAGED_CODE ();

    tst_FUNCIN_m();

    tst_TRY_m(ntStatus)
    {
        list = WdfFdoGetDefaultChildList(Device);

        if (0 == SerialNo) 
        {
            // Unplug everybody.  We do this by starting a scan and then not reporting
            // any children upon its completion
            WdfChildListBeginScan(list);
            //
            // A call to WdfChildListBeginScan indicates to the framework that the
            // driver is about to scan for dynamic children. After this call has
            // returned, all previously reported children associated with this will be
            // marked as potentially missing.  A call to either
            // WdfChildListUpdateChildDescriptionAsPresent  or
            // WdfChildListMarkAllChildDescriptionsPresent will mark all previuosly
            // reported missing children as present.  If any children currently
            // present are not reported present by calling 
            // WdfChildListUpdateChildDescriptionAsPresent at the time of
            // WdfChildListEndScan, they will be reported as missing to the PnP subsystem
            // After WdfChildListEndScan call has returned, the framework will 
            // invalidate the device relations for the FDO associated with the list
            // and report the changes
            WdfChildListEndScan(list);
        }
        else 
        {
            PDO_IDENTIFICATION_DESCRIPTION description;
            WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(&description.Header, sizeof(description));

            description.SerialNo = SerialNo;
            // WdfFdoUpdateChildDescriptionAsMissing indicates to the framework that a
            // child device that was previuosly detected is no longe present on the bus.
            // This API can be called by itself or after a call to WdfChildListBeginScan.
            // After this call has returned, the framework will invalidate the device
            // relations for the FDO associated with the list and report the changes.
            ntStatus = WdfChildListUpdateChildDescriptionAsMissing(list, &description.Header);
            if (ntStatus == STATUS_NO_SUCH_DEVICE) 
            {
                // serial number didn't exist. Remap it to a status that user
                // application can understand when it gets translated to win32
                // error code.
                ntStatus = STATUS_INVALID_PARAMETER;
            };
        };
    }
    tst_CATCH_m(ntStatus);

    tst_FUNCOUT_m();

    return ntStatus;
}
//============================================================================//

//============================================================================//
NTSTATUS Bus_IoctlEjectDevice(WDFDEVICE Device, ULONG SerialNo)
{
    NTSTATUS ntStatus = STATUS_INVALID_PARAMETER;
    WDFDEVICE hChild;
    WDFCHILDLIST list;
    BOOLEAN bRet;

    PAGED_CODE ();

    tst_FUNCIN_m();

    tst_TRY_m(ntStatus)
    {
        list = WdfFdoGetDefaultChildList(Device);

        // A zero serial number means eject all children
        if (0 == SerialNo) 
        {
            WDF_CHILD_LIST_ITERATOR iterator;
            WDF_CHILD_LIST_ITERATOR_INIT(&iterator, WdfRetrievePresentChildren);

            WdfChildListBeginIteration(list, &iterator);

            for ( ; ; ) 
            {
                WDF_CHILD_RETRIEVE_INFO childInfo;
                PDO_IDENTIFICATION_DESCRIPTION description;

                // Init the structures.
                WDF_CHILD_RETRIEVE_INFO_INIT(&childInfo, &description.Header);
                WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(&description.Header, sizeof(description));

                // Get the device identification description
                ntStatus = WdfChildListRetrieveNextDevice(list, &iterator, &hChild, &childInfo);

                if (!NT_SUCCESS(ntStatus) || (ntStatus == STATUS_NO_MORE_ENTRIES)) 
                {
                    break;
                };
                tst_MUST_ASSERT_m((childInfo.Status == WdfChildListRetrieveDeviceSuccess), __LINE__, 0, 0, 0);

                // Use that description to request an eject.
                bRet = WdfChildListRequestChildEject(list, &description.Header);
                // Assert check only if FlagOn(WdfDriverGlobals->DriverFlags, WdfVerifyOn).
                tst_MUST_ASSERT_m(!FlagOn(WdfDriverGlobals->DriverFlags, WdfVerifyOn) || bRet, __LINE__, 0, 0, 0);
            };

            WdfChildListEndIteration(list, &iterator);

            if (ntStatus == STATUS_NO_MORE_ENTRIES) 
            {
                ntStatus = STATUS_SUCCESS;
            };
        }
        else 
        {
            PDO_IDENTIFICATION_DESCRIPTION description;
            WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(&description.Header, sizeof(description));

            description.SerialNo = SerialNo;

            bRet = WdfChildListRequestChildEject(list, &description.Header);
            // Assert check only if FlagOn(WdfDriverGlobals->DriverFlags, WdfVerifyOn).
            tst_MUST_ASSERT_m(!FlagOn(WdfDriverGlobals->DriverFlags, WdfVerifyOn) || bRet, __LINE__, 0, 0, 0);
            
            ntStatus = STATUS_SUCCESS;
        };
    }
    tst_CATCH_m(ntStatus);

    tst_FUNCOUT_m();

    return ntStatus;
}
//============================================================================//

//============================================================================//
NTSTATUS
Bus_EvtChildListCreatePDO(WDFCHILDLIST pDeviceList, 
                          PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER pDescr,
                          PWDFDEVICE_INIT pChildInit)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PPDO_IDENTIFICATION_DESCRIPTION pPDODesc;

    tst_FUNCIN_m();

    tst_TRY_m(ntStatus)
    {
        pPDODesc = CONTAINING_RECORD(pDescr, PDO_IDENTIFICATION_DESCRIPTION, Header);
        ntStatus = Bus_CreatePDO(WdfChildListGetDevice(pDeviceList), 
            pChildInit, pPDODesc->HardwareIds, pPDODesc->SerialNo);
    }
    tst_CATCH_m(ntStatus);

    tst_FUNCOUT_m();

    return ntStatus;
};
//============================================================================//

//============================================================================//
NTSTATUS
Bus_EvtChildListDuplicate(WDFCHILDLIST DeviceList, 
                          PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER pSource,   
                          PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER pDest)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PPDO_IDENTIFICATION_DESCRIPTION pPDOsrc;
    PPDO_IDENTIFICATION_DESCRIPTION pPDOdst;
    size_t safeMultResult;

    UNREFERENCED_PARAMETER(DeviceList);

    tst_FUNCIN_m();

    tst_TRY_m(ntStatus)
    {
        pPDOsrc = CONTAINING_RECORD(pSource, PDO_IDENTIFICATION_DESCRIPTION, Header);
        pPDOdst = CONTAINING_RECORD(pDest, PDO_IDENTIFICATION_DESCRIPTION, Header);

        pPDOdst->SerialNo = pPDOsrc->SerialNo;
        pPDOdst->CchHardwareIds = pPDOsrc->CchHardwareIds;
        ntStatus = RtlSizeTMult(pPDOdst->CchHardwareIds, sizeof(WCHAR), &safeMultResult);
        if (!NT_SUCCESS(ntStatus))
        {
            tst_ERR_m((tst_ERR_PARAMS_d, tst_XY_ERRMSG_d,
            //  012345678901234567890123456789012345678901234567890123456        
            //  Failed in RtlSizeTMult with ntSts=0x%x. 
                "%x", 
                ntStatus));
            goto cleanup;
        };

        pPDOdst->HardwareIds = (PWCHAR) ExAllocatePoolWithTag(NonPagedPool, 
            safeMultResult, tst_GEN_NPAGEDP_BUFF_MAGIC_e);
        if (NULL == pPDOdst->HardwareIds) 
        {
            tst_ERR_m((tst_ERR_PARAMS_d, tst_XY_ERRMSG_d,
            //  012345678901234567890123456789012345678901234567890123456        
            //  Failed in allocate bytes(%d) for Tag(0x%x). Returning with ntSts=0x%x. 
                "%d|%x", 
                safeMultResult, tst_GEN_NPAGEDP_BUFF_MAGIC_e));

            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            RtlCopyMemory(pPDOdst->HardwareIds, pPDOsrc->HardwareIds, safeMultResult);
        };
    }
    tst_CATCH_m(ntStatus);

cleanup:
    tst_FUNCOUT_m();

    return ntStatus;
};
//============================================================================//

//============================================================================//
BOOLEAN
Bus_EvtChildListCompare(WDFCHILDLIST DeviceList, 
                        PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER pFirst,   
                        PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER pSecond)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PPDO_IDENTIFICATION_DESCRIPTION pPDOlhs;
    PPDO_IDENTIFICATION_DESCRIPTION pPDOrhs;
    BOOLEAN bRet = FALSE;

    UNREFERENCED_PARAMETER(DeviceList);

    tst_FUNCIN_m();

    tst_TRY_m(ntStatus)
    {
        pPDOlhs = CONTAINING_RECORD(pFirst, PDO_IDENTIFICATION_DESCRIPTION, Header);
        pPDOrhs = CONTAINING_RECORD(pSecond, PDO_IDENTIFICATION_DESCRIPTION, Header);
        bRet = (pPDOlhs->SerialNo == pPDOrhs->SerialNo);
    }
    tst_CATCH_m(ntStatus);

    tst_FUNCOUT_m();

    return bRet;
};
//============================================================================//

//============================================================================//
VOID
Bus_EvtChildListCleanup(WDFCHILDLIST DeviceList, 
                        PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER pDescription)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PPDO_IDENTIFICATION_DESCRIPTION pPDODesc;

    UNREFERENCED_PARAMETER(DeviceList);

    tst_FUNCIN_m();

    tst_TRY_m(ntStatus)
    {
        pPDODesc = CONTAINING_RECORD(pDescription, PDO_IDENTIFICATION_DESCRIPTION,
            Header);

        if (NULL != pPDODesc->HardwareIds) 
        {
            ExFreePool(pPDODesc->HardwareIds);
            pPDODesc->HardwareIds = NULL;
        };
    }
    tst_CATCH_m(ntStatus);

    tst_FUNCOUT_m();
};
//============================================================================//

//============================================================================//
//NOTE: Tracing does not work! Do synchorinzation!
NTSTATUS 
BusDumpTrace(IN ULONG ulLine, IN ULONG ulFileId, IN PCHAR pszFormat, ... )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LARGE_INTEGER sysTime;
    ULONG nLen;
    HANDLE hPid = PsGetCurrentProcessId();                                                    
    HANDLE hTid = PsGetCurrentThreadId();                                                     
    va_list arg_ptr;
    KIRQL kIrql = KeGetCurrentIrql();

    UNREFERENCED_PARAMETER(ulLine);
    UNREFERENCED_PARAMETER(ulFileId);

    KeQuerySystemTime(&sysTime);
    //TODO: This needs to be synchronized with g_mTraceLogLock.
    // Note: Spinlock WdfSpinLockAcquire(g_mTraceLogLock) will raise IRQL to 2,
    //       which makes RtlXxx unusable.

    __try
    {
        va_start(arg_ptr, pszFormat);
        ntStatus = RtlStringCchVPrintfExA(g_szTraceLogBuffer, sizeof(g_szTraceLogBuffer)-1, NULL, &nLen,
            0, pszFormat, arg_ptr);
        tst_MUST_ASSERT_m(NT_SUCCESS(ntStatus) | (ntStatus == STATUS_BUFFER_OVERFLOW), __LINE__, 0, 0, 0);
        va_end(arg_ptr);
        nLen = sizeof(g_szTraceLogBuffer) - nLen;

        DbgPrint("[VDBUS][I][%08x.%08x][%04d:%04d][%d] %s%s\n", 
            sysTime.HighPart, sysTime.LowPart, hPid, hTid, kIrql, g_szTraceLogBuffer, 
            (ntStatus == STATUS_BUFFER_OVERFLOW) ? "(OVR)":"");
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        tst_ASSERT_m(FALSE, __LINE__, GetExceptionCode(), 0, 0);
    };

    //TODO: Unlock g_mTraceLogLock.

    return ntStatus;
};
//============================================================================//

//============================================================================//
LONG
BusExceptionFilter(IN PCHAR pszFile, IN ULONG ulLine, IN ULONG ulBugCheckFileId,
                   IN PEXCEPTION_POINTERS pExceptionPointer)
{
    NTSTATUS ntExceptionCode;

    #ifdef _DEBUG
    DbgBreakPoint(); // In debug mode, will wait for WinDbg to connect.
    #endif

    ntExceptionCode = pExceptionPointer->ExceptionRecord->ExceptionCode;

    // If the exception is STATUS_IN_PAGE_ERROR, get the I/O error code
    // from the exception record.

    if ((ntExceptionCode == STATUS_IN_PAGE_ERROR) &&
        (pExceptionPointer->ExceptionRecord->NumberParameters >= 3)) 
    {
        ntExceptionCode = (NTSTATUS) pExceptionPointer->ExceptionRecord->ExceptionInformation[2];
    };

    tst_TRC_m(tst_DBG_MAIN_d, (tst_TRC_PARAMS_d,
        "EXCEPTION: ExcpCode(0x%x) .exr(0x%x) .cxr(0x%x) bp(0x%x).",                          
        ntExceptionCode, pExceptionPointer->ExceptionRecord, pExceptionPointer->ContextRecord,                    
        pExceptionPointer->ExceptionRecord->ExceptionAddress));

    //  Bug check if this status is not supported.
    KeBugCheckEx(tst_BUGCHECK_d, ulBugCheckFileId | ulLine, 
        (ULONG_PTR) pExceptionPointer->ExceptionRecord, (ULONG_PTR) pExceptionPointer->ContextRecord,
        (ULONG_PTR) pExceptionPointer->ExceptionRecord->ExceptionAddress);

    return EXCEPTION_EXECUTE_HANDLER;
};
//============================================================================//

//============================================================================//
NTSTATUS 
BusWriteLogMessage(IN ULONG ulCode, IN ULONG ulStatus, IN PCHAR pszMsg, IN ULONG ulMsgLen, IN USHORT uhStringCount)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG ulTotalLen;
    
    PIO_ERROR_LOG_PACKET pLogEntry = NULL;

    __try
    {
        tst_INP_ASSERT_m(NULL != pszMsg, __LINE__, 0, 0, 0);
        tst_INP_ASSERT_m(0 != ulMsgLen, __LINE__, 0, 0, 0);

        tst_MEM_ASSERT_m(NULL != g_pDriverObject, __LINE__, 0, 0, 0);
        if (g_pDriverObject != NULL)
        {
            if (ulMsgLen > 
                (((ERROR_LOG_MAXIMUM_SIZE - FIELD_OFFSET(IO_ERROR_LOG_PACKET, DumpData))/sizeof(WCHAR))-1))
            {
                ulMsgLen = (ERROR_LOG_MAXIMUM_SIZE - FIELD_OFFSET(IO_ERROR_LOG_PACKET, DumpData))/sizeof(WCHAR)-1;
                pszMsg[ulMsgLen-1] = '*';
                pszMsg[ulMsgLen] = tst_EOS_d;
            };
            
            ulTotalLen = FIELD_OFFSET(IO_ERROR_LOG_PACKET, DumpData) + (ulMsgLen + 1)*sizeof(WCHAR);
            ulTotalLen = tst_Max_m(ulTotalLen, sizeof(IO_ERROR_LOG_PACKET));

            pLogEntry = IoAllocateErrorLogEntry(g_pDriverObject, (UCHAR) ulTotalLen);
            if (NULL != pLogEntry)
            {
                pLogEntry->ErrorCode = ulCode;
                pLogEntry->FinalStatus = ulStatus;
                pLogEntry->UniqueErrorValue = ulStatus;
                pLogEntry->NumberOfStrings = uhStringCount;
                pLogEntry->StringOffset = FIELD_OFFSET(IO_ERROR_LOG_PACKET, DumpData);

                ntStatus = RtlMultiByteToUnicodeN((LPWSTR) pLogEntry->DumpData, ulMsgLen * sizeof(WCHAR),
                    &ulMsgLen, pszMsg, ulMsgLen);
                tst_MUST_ASSERT_m(NT_SUCCESS(ntStatus), __LINE__, 0, 0, 0);

                IoWriteErrorLogEntry(pLogEntry);
                pLogEntry = NULL;
            }
            else
            {
                DbgPrint("[VDBUS][E][00000000.00000000][0000:0000] Can't Log message(0x%x,0x%x,%d,%s)(TotalLen=%d).\n", 
                    ulCode, ulStatus, ulMsgLen, pszMsg, ulTotalLen);
            };
        };
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        tst_MUST_ASSERT_m(FALSE, __LINE__, GetExceptionCode(), 0, 0);
    };

    return ntStatus;
};
//============================================================================//

//============================================================================//
//NOTE: Tracing does not work! Do synchorinzation!
NTSTATUS 
BusDumpStrings(IN ULONG ulLine, IN ULONG ulFileId, IN CHAR cDelimiter, 
               IN ULONG ulLogEventId, IN PCHAR pszFormat, ... )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LARGE_INTEGER sysTime;
    ULONG nLen;
    ULONG ulEvtId;
    USHORT uhStringCount;
    CHAR ucMsgType;

    HANDLE hPid = PsGetCurrentProcessId();                                                    
    HANDLE hTid = PsGetCurrentThreadId();                                                     
    va_list arg_ptr;
    PCHAR pDelimiter;

    KeQuerySystemTime(&sysTime);
    //This raise IRQL to 2: WdfSpinLockAcquire(g_mTraceLogLock);

    __try                                
    {
        va_start(arg_ptr, pszFormat);
        ntStatus = RtlStringCchVPrintfExA(g_szTraceLogBuffer, sizeof(g_szTraceLogBuffer)-1, NULL, &nLen,
            0, pszFormat, arg_ptr);
        tst_MUST_ASSERT_m(NT_SUCCESS(ntStatus) | (ntStatus == STATUS_BUFFER_OVERFLOW), __LINE__, 0, 0, 0);
        va_end(arg_ptr);
        nLen = sizeof(g_szTraceLogBuffer) - nLen;

        ucMsgType = 'X';
        if (NT_ERROR(ulLogEventId))
        {
            ucMsgType = 'E';
        }
        else if (NT_WARNING(ulLogEventId))
        {
            ucMsgType = 'W';
        }
        else if (NT_INFORMATION(ulLogEventId))
        {
            ucMsgType = 'N';
        };

        ulEvtId = (ulLogEventId & 0x0000FFFF);
        
        DbgPrint("[VDBUS][%c][%08x.%08x][%04d:%04d] MsgId=0x%08x EvtId=%d %s%s\n", 
            ucMsgType, sysTime.HighPart, sysTime.LowPart, hPid, hTid, 
            tst_MESSAGEID_m(ulFileId, ulLine), ulEvtId, 
            g_szTraceLogBuffer, (ntStatus == STATUS_BUFFER_OVERFLOW) ? "(OVR)":"");

        uhStringCount = 1;
        if (cDelimiter != tst_EOS_d) 
        {
            // replace all occurences of cDelimiter by ivdfs_EOS_d
            for (pDelimiter = g_szTraceLogBuffer; *pDelimiter != tst_EOS_d; pDelimiter++) 
            {
                if (cDelimiter == *pDelimiter) 
                {
                    uhStringCount++;
                    *pDelimiter = tst_EOS_d;
                };
            };
        };

        ntStatus = BusWriteLogMessage(ulLogEventId, tst_MESSAGEID_m(ulFileId, ulLine),
            g_szTraceLogBuffer, nLen, uhStringCount);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        tst_MUST_ASSERT_m(FALSE, __LINE__, GetExceptionCode(), 0, 0);
    };

    //This raise IRQL to 2: WdfSpinLockRelease(g_mTraceLogLock);

    return ntStatus;
};
//============================================================================//

// End Of File
