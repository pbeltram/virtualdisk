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

#include "vdbus.h"

#define tst_BUG_CHECK_FILEID_d tst_BUG_CHECK_FILEID_PDO_d

//============================================================================//

//============================================================================//
NTSTATUS
Bus_CreatePDO(IN WDFDEVICE pFDO_Device, IN PWDFDEVICE_INIT DeviceInit, 
              IN PWCHAR HardwareIds, IN ULONG SerialNo)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    WDF_PDO_EVENT_CALLBACKS pdoCallbacks;
    WDF_OBJECT_ATTRIBUTES pdo_Attributes;
    WDF_DEVICE_PNP_CAPABILITIES pnpCaps;
    WDF_DEVICE_POWER_CAPABILITIES powerCaps;
    WDF_IO_QUEUE_CONFIG queueConfig;
    WDFQUEUE queue;
    WDF_OBJECT_ATTRIBUTES queue_Attributes;
    PFDO_DEVICE_DATA pFDODevExt;

    //TODO: Handle multiple disk creation, SerialNo= 2,3,4,....
    // Device extension size is too big -> error 0xC000009A.

    DECLARE_CONST_UNICODE_STRING(deviceID, L"VDBUS\\Disk");
    DECLARE_CONST_UNICODE_STRING(hardwareID, L"GenDisk\0");
    DECLARE_CONST_UNICODE_STRING(compatibleID, L"VDBUS\\Disk\0GenDisk\0");
    DECLARE_CONST_UNICODE_STRING(deviceLocation, L"VDBUS");

    DECLARE_UNICODE_STRING_SIZE(buffer, 80);
    PPDO_DEVICE_DATA pdoData = NULL;
    WDFDEVICE hChild = NULL;

    tst_FUNCIN_m();

    tst_TRY_m(ntStatus)
    {
        tst_TRC_m(tst_DBG_MAIN_d, (tst_TRC_PARAMS_d, 
            "Create PDO for (%ws,%d) from FDO_WDFDEVICE(0x%x)PDO(0x%x)FDO(0x%x).", 
            HardwareIds, SerialNo, pFDO_Device, 
            WdfDeviceWdmGetPhysicalDevice(pFDO_Device), 
            WdfDeviceWdmGetDeviceObject(pFDO_Device)));

        // Set DeviceType
        WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_DISK);
        WdfDeviceInitSetExclusive(DeviceInit, TRUE);
        WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoDirect);
        
        WdfDeviceInitSetCharacteristics(DeviceInit, 
            FILE_DEVICE_SECURE_OPEN 
            /*| FILE_REMOVABLE_MEDIA*/
            | FILE_CHARACTERISTIC_PNP_DEVICE, TRUE);
        WdfDeviceInitSetDeviceClass(DeviceInit, &GUID_DEVCLASS_DISKDRIVE);

        // Provide DeviceID, HardwareIDs, CompatibleIDs and InstanceId
        ntStatus = WdfPdoInitAssignDeviceID(DeviceInit, &deviceID);
        if (!NT_SUCCESS(ntStatus)) 
        {
            tst_ERR_m((tst_ERR_PARAMS_d, tst_XY_ERRMSG_d,
            //  012345678901234567890123456789012345678901234567890123456        
            //  Failed in WdfPdoInitAssignDeviceID with ntSts=0x%x. 
                "%x", 
                ntStatus));
            goto cleanup;
        };

        ntStatus = WdfPdoInitAddHardwareID(DeviceInit, &hardwareID);
        if (!NT_SUCCESS(ntStatus)) 
        {
            tst_ERR_m((tst_ERR_PARAMS_d, tst_XY_ERRMSG_d,
            //  012345678901234567890123456789012345678901234567890123456        
            //  Failed in WdfPdoInitAddHardwareID with ntSts=0x%x. 
                "%x", 
                ntStatus));
            goto cleanup;
        };

        ntStatus = WdfPdoInitAddCompatibleID(DeviceInit, &compatibleID);
        if (!NT_SUCCESS(ntStatus)) 
        {
            tst_ERR_m((tst_ERR_PARAMS_d, tst_XY_ERRMSG_d,
            //  012345678901234567890123456789012345678901234567890123456        
            //  Failed in WdfPdoInitAddCompatibleID with ntSts=0x%x. 
                "%x", 
                ntStatus));
            goto cleanup;
        };

        ntStatus = RtlUnicodeStringPrintf(&buffer, L"VDBusDisk%02d", SerialNo);
        if (!NT_SUCCESS(ntStatus)) 
        {
            tst_ERR_m((tst_ERR_PARAMS_d, tst_XY_ERRMSG_d,
            //  012345678901234567890123456789012345678901234567890123456        
            //  Failed in RtlUnicodeStringPrintf with ntSts=0x%x. 
                "%x", 
                ntStatus));
            goto cleanup;
        };

        ntStatus = WdfPdoInitAssignInstanceID(DeviceInit, &buffer);
        if (!NT_SUCCESS(ntStatus)) 
        {
            tst_ERR_m((tst_ERR_PARAMS_d, tst_XY_ERRMSG_d,
            //  012345678901234567890123456789012345678901234567890123456        
            //  Failed in WdfPdoInitAssignInstanceID with ntSts=0x%x. 
                "%x", 
                ntStatus));
            goto cleanup;
        };

        // Provide a description about the device. This text is usually read from
        // the device. In the case of USB device, this text comes from the string
        // descriptor. This text is displayed momentarily by the PnP manager while
        // it's looking for a matching INF. If it finds one, it uses the Device
        // Description from the INF file or the friendly name created by 
        // coinstallers to display in the device manager. FriendlyName takes 
        // precedence over the DeviceDesc from the INF file.
        ntStatus = RtlUnicodeStringPrintf(&buffer, L"VDBUS Disk %02d", SerialNo);
        if (!NT_SUCCESS(ntStatus)) 
        {
            tst_ERR_m((tst_ERR_PARAMS_d, tst_XY_ERRMSG_d,
            //  012345678901234567890123456789012345678901234567890123456        
            //  Failed in RtlUnicodeStringPrintf with ntSts=0x%x. 
                "%x", 
                ntStatus));
            goto cleanup;
        };

        // You can call WdfPdoInitAddDeviceText multiple times, adding device
        // text for multiple locales. When the system displays the text, it
        // chooses the text that matches the current locale, if available.
        // Otherwise it will use the string for the default locale.
        // The driver can specify the driver's default locale by calling
        // WdfPdoInitSetDefaultLocale.
        ntStatus = WdfPdoInitAddDeviceText(DeviceInit, &buffer, &deviceLocation, 0x409);
        if (!NT_SUCCESS(ntStatus)) 
        {
            tst_ERR_m((tst_ERR_PARAMS_d, tst_XY_ERRMSG_d,
            //  012345678901234567890123456789012345678901234567890123456        
            //  Failed in WdfPdoInitAddDeviceText with ntSts=0x%x. 
                "%x", 
                ntStatus));
            goto cleanup;
        };
        WdfPdoInitSetDefaultLocale(DeviceInit, 0x409);

        // Initialize WDF_PDO_EVENT_CALLBACKS structure to fill the
        // dispatch table for a bus driver's event callbacks. We use
        // this callback to provide a fake I/O port resource.
        WDF_PDO_EVENT_CALLBACKS_INIT(&pdoCallbacks);
        WdfPdoInitSetEventCallbacks(DeviceInit, &pdoCallbacks);

        // Mark a PDO as raw.
        WdfPdoInitAssignRawDevice(DeviceInit, &GUID_DEVCLASS_DISKDRIVE);

        // Initialize the attributes to specify the size of PDO device extension.
        // All the state information private to the PDO will be tracked here.
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&pdo_Attributes, PDO_DEVICE_DATA);
        pdo_Attributes.SynchronizationScope = WdfSynchronizationScopeInheritFromParent;
        pdo_Attributes.ExecutionLevel = WdfExecutionLevelInheritFromParent;

        ntStatus = WdfDeviceCreate(&DeviceInit, &pdo_Attributes, &hChild);
        if (!NT_SUCCESS(ntStatus)) 
        {
            tst_ERR_m((tst_ERR_PARAMS_d, tst_XY_ERRMSG_d,
            //  012345678901234567890123456789012345678901234567890123456        
            //  Failed in WdfDeviceCreate with ntSts=0x%x. 
                "%x", 
                ntStatus));
            goto cleanup;
        };

        tst_TRC_m(tst_DBG_MAIN_d, (tst_TRC_PARAMS_d, 
            "Created PDO_WDFDEVICE(0x%x)PDO(0x%x)FDO(0x%x).", 
            hChild, WdfDeviceWdmGetPhysicalDevice(hChild), WdfDeviceWdmGetDeviceObject(hChild)));

        // Get the device context.
        pdoData = BusGetPDODevExtData(hChild);
        pFDODevExt = BusGetFDODevExtData(WdfPdoGetParent(hChild));

        pdoData->SerialNo = SerialNo;
        
        //Disk geometry.
        pdoData->sDiskGeometry.Cylinders.QuadPart = (LONGLONG) tst_NOFCYLINDERS_d;
        pdoData->sDiskGeometry.TracksPerCylinder = tst_TRACKSPERCYLINDER_d;
        pdoData->sDiskGeometry.SectorsPerTrack = tst_SECTORSPERTRACK_d;
        pdoData->sDiskGeometry.BytesPerSector = tst_BYTESPERSECTOR_d;
        //pdoData->sDiskGeometry.MediaType = RemovableMedia;
        pdoData->sDiskGeometry.MediaType = FixedMedia;

        pdoData->ulLastSector = tst_NOFCYLINDERS_d*tst_TRACKSPERCYLINDER_d*tst_SECTORSPERTRACK_d;
        pdoData->ulBlockSize = tst_BYTESPERSECTOR_d;

        // SCSI Address.
        pdoData->sScsiAddress.Length = sizeof(SCSI_ADDRESS);
        pdoData->sScsiAddress.PortNumber = pFDODevExt->ucBusNumber;
        pdoData->sScsiAddress.PathId = tst_DISKPATHID_d;
        pdoData->sScsiAddress.TargetId = tst_HIBYTE_m(pdoData->SerialNo);
        pdoData->sScsiAddress.Lun = tst_LOBYTE_m(pdoData->SerialNo);

        // Init STORAGE_DEVICE_DESCRIPTOR info.
        RtlZeroMemory(&pdoData->sDiskDesc, sizeof(pdoData->sDiskDesc));
        pdoData->sDiskDesc.Version = sizeof(STORAGE_DEVICE_DESCRIPTOR);
        pdoData->sDiskDesc.Size = sizeof(pdoData->sDiskDesc)+
            sizeof(pdoData->szRawProperties)+
            sizeof(pdoData->szVendorId)+
            sizeof(pdoData->szProductId)+
            sizeof(pdoData->szProductRevision)+
            sizeof(pdoData->szSerialNumber);
        pdoData->sDiskDesc.DeviceType = 0x00;
        pdoData->sDiskDesc.DeviceTypeModifier = 0x00;
        //pdoData->sDiskDesc.RemovableMedia = TRUE;
        pdoData->sDiskDesc.RemovableMedia = FALSE;
        pdoData->sDiskDesc.CommandQueueing = FALSE;
        pdoData->sDiskDesc.VendorIdOffset = (PCHAR) pdoData->szVendorId - (PCHAR) &pdoData->sDiskDesc;
        pdoData->sDiskDesc.ProductIdOffset = (PCHAR) pdoData->szProductId - (PCHAR) &pdoData->sDiskDesc;
        pdoData->sDiskDesc.ProductRevisionOffset = (PCHAR) pdoData->szProductRevision - (PCHAR) &pdoData->sDiskDesc;
        pdoData->sDiskDesc.SerialNumberOffset = (PCHAR) pdoData->szSerialNumber - (PCHAR) &pdoData->sDiskDesc;
        pdoData->sDiskDesc.BusType = BusTypeScsi;
        pdoData->sDiskDesc.RawPropertiesLength = sizeof(pdoData->szRawProperties);
        ntStatus = RtlStringCchPrintfA(pdoData->sDiskDesc.RawDeviceProperties, 
            sizeof(pdoData->szRawProperties)+1, 
            "Raw properties for Disk_%d", pdoData->SerialNo);
        tst_MUST_ASSERT_m(NT_SUCCESS(ntStatus), __LINE__, ntStatus, 0, 0);

        ntStatus = RtlStringCchCopyA(pdoData->szVendorId, 
            sizeof(pdoData->szVendorId), 
            "Primozb");
        tst_MUST_ASSERT_m(NT_SUCCESS(ntStatus), __LINE__, ntStatus, 0, 0);

        ntStatus = RtlStringCchCopyA(pdoData->szProductId, 
            sizeof(pdoData->szProductId), 
            "BUS_DISK");
        tst_MUST_ASSERT_m(NT_SUCCESS(ntStatus), __LINE__, ntStatus, 0, 0);

        ntStatus = RtlStringCchPrintfA(pdoData->szProductRevision, 
            sizeof(pdoData->szProductRevision), 
            "V%d.%d", tst_VERSION_MAJOR_d, tst_VERSION_MINOR_d);
        tst_MUST_ASSERT_m(NT_SUCCESS(ntStatus), __LINE__, ntStatus, 0, 0);

        ntStatus = RtlStringCchPrintfA(pdoData->szSerialNumber, 
            sizeof(pdoData->szSerialNumber), 
            "SerialNo_%d", pdoData->SerialNo);
        tst_MUST_ASSERT_m(NT_SUCCESS(ntStatus), __LINE__, ntStatus, 0, 0);

        // Configure the default queue associated with the control device object
        // to be Serial so that request passed to EvtIoDeviceControl are serialized.
        // A default queue gets all the requests that are not
        // configure-fowarded using WdfDeviceConfigureRequestDispatching.
        WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchParallel);

        queueConfig.PowerManaged = WdfFalse;
        queueConfig.AllowZeroLengthRequests = TRUE;
        queueConfig.EvtIoDeviceControl = BusPDO_EvtIoDeviceControl;
        queueConfig.EvtIoDefault = BusPDO_EvtIoDefault;
        queueConfig.EvtIoRead = BusPDO_EvtIoRead;
        queueConfig.EvtIoWrite = BusPDO_EvtIoWrite;
        //DEL queueConfig.EvtIoInternalDeviceControl = BusPDO_EvtIoInternalDeviceControl;
        queueConfig.EvtIoStop = BusPDO_EvtIoStop;
        queueConfig.EvtIoResume = BusPDO_EvtIoResume;
        queueConfig.EvtIoCanceledOnQueue = BusPDO_EvtIoCanceledOnQueue;

        WDF_OBJECT_ATTRIBUTES_INIT(&queue_Attributes);
        queue_Attributes.SynchronizationScope = WdfSynchronizationScopeInheritFromParent;
        queue_Attributes.ExecutionLevel = WdfExecutionLevelInheritFromParent;

        ntStatus = WdfIoQueueCreate(hChild, &queueConfig, &queue_Attributes, &queue);
        if (!NT_SUCCESS(ntStatus)) 
        {
            tst_ERR_m((tst_ERR_PARAMS_d, tst_XY_ERRMSG_d,
            //  012345678901234567890123456789012345678901234567890123456        
            //  Failed in WdfIoQueueCreate with ntSts=0x%x. 
                "%x", 
                ntStatus));
            goto cleanup;
        };

        tst_TRC_m(tst_DBG_MAIN_d, (tst_TRC_PARAMS_d, 
            "Created WDFQUEUE0(x%x) for PDO_WDFDEVICE(x%x).", 
            queue, WdfIoQueueGetDevice(queue)));

        // Set PNP properties for the child device.
        WDF_DEVICE_PNP_CAPABILITIES_INIT(&pnpCaps);
        pnpCaps.Removable = WdfTrue;
        pnpCaps.EjectSupported = WdfTrue;
        pnpCaps.SurpriseRemovalOK = WdfTrue;

        pnpCaps.Address  = SerialNo;
        pnpCaps.UINumber = SerialNo;

        WdfDeviceSetPnpCapabilities(hChild, &pnpCaps);

        // Set POWER properties for the child device.
        WDF_DEVICE_POWER_CAPABILITIES_INIT(&powerCaps);
        powerCaps.DeviceWake = PowerDeviceD1;

        powerCaps.DeviceState[PowerSystemWorking]   = PowerDeviceD0;
        powerCaps.DeviceState[PowerSystemSleeping1] = PowerDeviceD3;
        powerCaps.DeviceState[PowerSystemSleeping2] = PowerDeviceD3;
        powerCaps.DeviceState[PowerSystemSleeping3] = PowerDeviceD3;
        powerCaps.DeviceState[PowerSystemHibernate] = PowerDeviceD3;
        powerCaps.DeviceState[PowerSystemShutdown]  = PowerDeviceD3;

        WdfDeviceSetPowerCapabilities(hChild, &powerCaps);

        // Create device interface for this device. The interface will be
        // enabled by the framework when we return from StartDevice successfully.
        // Clients of this driver will open this interface and send ioctls.
        // No Reference String. If you provide one it will appended to the 
        // symbolic link. Some drivers register multiple interfaces for the same device
        // and use the reference string to distinguish between them
        ntStatus = WdfDeviceCreateDeviceInterface(hChild, &GUID_DEVCLASS_DISKDRIVE, NULL);   
        if (!NT_SUCCESS(ntStatus)) 
        {
            tst_ERR_m((tst_ERR_PARAMS_d, tst_XY_ERRMSG_d,
            //  012345678901234567890123456789012345678901234567890123456        
            //  WdfDeviceCreateDeviceInterface failed with ntSts=0x%x.
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
VOID
BusPDO_EvtIoDefault(IN WDFQUEUE hQueue, IN WDFREQUEST hRequest)
{
    WDF_REQUEST_PARAMETERS sReqParams;

    PAGED_CODE ();

    WDF_REQUEST_PARAMETERS_INIT(&sReqParams);
    WdfRequestGetParameters(hRequest, &sReqParams);

    switch (sReqParams.Type)
    {
        case WdfRequestTypeDeviceControlInternal: // IRP_MJ_SCSI

            // Handle IRP_MJ_SCSI request.
            BusPDO_InternalSCSI(hQueue, hRequest);
            break;

        default:

            tst_TRC_m(tst_DBG_MAIN_d, (tst_TRC_PARAMS_d, 
                "Unknown EvtIoDefault(0x%x,0x%x).", 
                sReqParams.Type, sReqParams.MinorFunction));

            WdfRequestComplete(hRequest, STATUS_INVALID_DEVICE_REQUEST);
            break;
    };

};
//============================================================================//

//============================================================================//
VOID
BusPDO_EvtIoDeviceControl(IN WDFQUEUE hQueue, IN WDFREQUEST hRequest, 
                          IN size_t ulOutputBufferLength, IN size_t ulInputBufferLength,
                          IN ULONG ulIoControlCode)
{
    UNREFERENCED_PARAMETER(ulOutputBufferLength);
    UNREFERENCED_PARAMETER(ulInputBufferLength);

    PAGED_CODE ();

    switch (ulIoControlCode) 
    {
        case IOCTL_STORAGE_QUERY_PROPERTY:

            BusPDO_StorageQueryProperty(hQueue, hRequest);
            break;

        case IOCTL_DISK_GET_DRIVE_GEOMETRY:

            BusPDO_GetDriveGeometry(hQueue, hRequest);
            break;

        case IOCTL_SCSI_GET_ADDRESS:

            BusPDO_GetAddress(hQueue, hRequest);
            break;
        
        default:

            tst_TRC_m(tst_DBG_MAIN_d, (tst_TRC_PARAMS_d, 
                "PDO unknown IOCTL(0x%x)(0x%x,0x%x,0x%x,0x%x).", 
                ulIoControlCode, 
                tst_DEVICE_FROM_CTL_CODE_m(ulIoControlCode), tst_FUNCTION_FROM_CTL_CODE_m(ulIoControlCode), 
                tst_METHOD_FROM_CTL_CODE_m(ulIoControlCode), tst_ACCESS_FROM_CTL_CODE_m(ulIoControlCode)));

            WdfRequestComplete(hRequest, STATUS_INVALID_DEVICE_REQUEST);
            break; 
    };

};
//============================================================================//

//============================================================================//
VOID
BusPDO_EvtIoRead(IN WDFQUEUE hQueue, IN WDFREQUEST hRequest, IN size_t ulLength)
{
    WDFDEVICE hDevice;

    PAGED_CODE ();

    hDevice = WdfIoQueueGetDevice(hQueue);

    tst_TRC_m(tst_DBG_MAIN_d, (tst_TRC_PARAMS_d, 
        "BusPDO_EvtIoRead.WDFDEVICE(0x%x,0x%x,0x%x)(%d).", 
        hDevice, hQueue, hRequest, ulLength));

    WdfRequestComplete(hRequest, STATUS_SUCCESS);

};
//============================================================================//

//============================================================================//
VOID
BusPDO_EvtIoWrite(IN WDFQUEUE hQueue, IN WDFREQUEST hRequest, IN size_t ulLength)
{
    WDFDEVICE hDevice;

    PAGED_CODE ();

    hDevice = WdfIoQueueGetDevice(hQueue);

    tst_TRC_m(tst_DBG_MAIN_d, (tst_TRC_PARAMS_d, 
        "BusPDO_EvtIoWrite.WDFDEVICE(0x%x,0x%x,0x%x)(%d).", 
        hDevice, hQueue, hRequest, ulLength));

    WdfRequestComplete(hRequest, STATUS_SUCCESS);

};
//============================================================================//

//============================================================================//
VOID
BusPDO_EvtIoStop(IN WDFQUEUE hQueue, IN WDFREQUEST hRequest, IN ULONG ulActionFlag)
{
    WDFDEVICE hDevice;

    PAGED_CODE ();

    hDevice = WdfIoQueueGetDevice(hQueue);

    tst_TRC_m(tst_DBG_MAIN_d, (tst_TRC_PARAMS_d, 
        "BusPDO_EvtIoStop.WDFDEVICE(0x%x,0x%x,0x%x)(0x%x).", 
        hDevice, hQueue, hRequest, ulActionFlag));

    WdfRequestComplete(hRequest, STATUS_SUCCESS);

};
//============================================================================//

//============================================================================//
VOID
BusPDO_EvtIoResume(IN WDFQUEUE hQueue, IN WDFREQUEST hRequest)
{
    WDFDEVICE hDevice;

    PAGED_CODE ();

    hDevice = WdfIoQueueGetDevice(hQueue);

    tst_TRC_m(tst_DBG_MAIN_d, (tst_TRC_PARAMS_d, 
        "BusPDO_EvtIoResume.WDFDEVICE(0x%x,0x%x,0x%x).", 
        hDevice, hQueue, hRequest));

    WdfRequestComplete(hRequest, STATUS_SUCCESS);

};
//============================================================================//

//============================================================================//
VOID
BusPDO_EvtIoCanceledOnQueue(IN WDFQUEUE hQueue, IN WDFREQUEST hRequest)
{
    WDFDEVICE hDevice;

    PAGED_CODE ();

    hDevice = WdfIoQueueGetDevice(hQueue);

    tst_TRC_m(tst_DBG_MAIN_d, (tst_TRC_PARAMS_d, 
        "BusPDO_EvtIoCanceledOnQueue.WDFDEVICE(0x%x,0x%x,0x%x).", 
        hDevice, hQueue, hRequest));

    WdfRequestComplete(hRequest, STATUS_SUCCESS);

};
//============================================================================//
// End Of File

