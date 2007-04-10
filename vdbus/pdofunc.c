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

#define tst_BUG_CHECK_FILEID_d tst_BUG_CHECK_FILEID_PDOFUNC_d

VOID 
BusPDO_StorageQueryProperty(IN WDFQUEUE hQueue, IN WDFREQUEST hRequest)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PFDO_DEVICE_DATA pFDODevExt;
    PPDO_DEVICE_DATA pPDODevExt;
    PSTORAGE_PROPERTY_QUERY pPropQuery;
    PVOID pOutBuff;
    ULONG ulInpLen;
    ULONG ulOutLen;
    
    PAGED_CODE();

    tst_INP_ASSERT_m(NULL != hQueue, __LINE__, 0, 0, 0);
    tst_INP_ASSERT_m(NULL != hRequest, __LINE__, 0, 0, 0);

    ntStatus = WdfRequestRetrieveInputBuffer(hRequest, sizeof(STORAGE_PROPERTY_QUERY), 
        &pPropQuery, &ulInpLen);
    if (!NT_SUCCESS(ntStatus)) 
    {
        tst_ERR_m((tst_ERR_PARAMS_d, tst_XY_ERRMSG_d,
        //  012345678901234567890123456789012345678901234567890123456        
        //  WdfRequestRetrieveInputBuffer failed with ntSts=0x%x.
            "%x",
            ntStatus));
        
        WdfRequestComplete(hRequest, ntStatus);
        return;
    };
    tst_MUST_ASSERT_m((ulInpLen >= sizeof(STORAGE_PROPERTY_QUERY)), __LINE__, ulInpLen, 0, 0);

    ntStatus = WdfRequestRetrieveOutputBuffer(hRequest, sizeof(STORAGE_DESCRIPTOR_HEADER), 
        &pOutBuff, &ulOutLen);
    if (!NT_SUCCESS(ntStatus)) 
    {
        tst_ERR_m((tst_ERR_PARAMS_d, tst_XY_ERRMSG_d,
        //  012345678901234567890123456789012345678901234567890123456        
        //  WdfRequestRetrieveOutputBuffer failed with ntSts=0x%x.
            "%x",
            ntStatus));

        WdfRequestComplete(hRequest, ntStatus);
        return;
    };
    tst_MUST_ASSERT_m((ulOutLen >= sizeof(STORAGE_DESCRIPTOR_HEADER)), __LINE__, ulOutLen, 0, 0);


    tst_TRC_m(tst_DBG_MAIN_d, (tst_TRC_PARAMS_d, 
        "IOCTL_STORAGE_QUERY_PROPERTY for PropQuery(%d,%d). InpLen=%d OutLen=%d.", 
        pPropQuery->QueryType, pPropQuery->PropertyId, ulInpLen, ulOutLen));

    if (PropertyStandardQuery == pPropQuery->QueryType)
    {
        if (StorageAdapterProperty == pPropQuery->PropertyId)
        {
            pFDODevExt = BusGetFDODevExtData(WdfPdoGetParent(WdfIoQueueGetDevice(hQueue)));

            ulOutLen = tst_Min_m(ulOutLen, pFDODevExt->sBusDesc.Size);
            RtlCopyMemory(pOutBuff, &pFDODevExt->sBusDesc, ulOutLen);

            WdfRequestCompleteWithInformation(hRequest, STATUS_SUCCESS, ulOutLen);
        }
        else if (StorageDeviceProperty == pPropQuery->PropertyId)
        {
            pPDODevExt = BusGetPDODevExtData(WdfIoQueueGetDevice(hQueue));

            ulOutLen = tst_Min_m(ulOutLen, pPDODevExt->sDiskDesc.Size);
            RtlCopyMemory(pOutBuff, &pPDODevExt->sDiskDesc, ulOutLen);

            WdfRequestCompleteWithInformation(hRequest, STATUS_SUCCESS, ulOutLen);
        }
        else
        {
            tst_ERR_m((tst_ERR_PARAMS_d, tst_XY_ERRMSG_d,
            //  012345678901234567890123456789012345678901234567890123456        
            //  Invalid PropertyId=0x%x.
                "%x",
                pPropQuery->PropertyId));

            WdfRequestComplete(hRequest, STATUS_INVALID_PARAMETER);
        };
    }
    else if (PropertyExistsQuery == pPropQuery->QueryType)
    {
        // No documentation what to return.
        RtlZeroMemory(pOutBuff, ulOutLen);

        WdfRequestCompleteWithInformation(hRequest, STATUS_SUCCESS, ulOutLen);
    }                          
    else
    {
        tst_ERR_m((tst_ERR_PARAMS_d, tst_XY_ERRMSG_d,
        //  012345678901234567890123456789012345678901234567890123456        
        //  Invalid QueryType=0x%x.
            "%x",
            pPropQuery->QueryType));

        WdfRequestComplete(hRequest, STATUS_INVALID_PARAMETER);
    };
};
//============================================================================//

//============================================================================//
VOID 
BusPDO_GetDriveGeometry(IN WDFQUEUE hQueue, IN WDFREQUEST hRequest)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PPDO_DEVICE_DATA pPDODevExt;
    PVOID pOutBuff;
    ULONG ulOutLen;
    
    PAGED_CODE();

    tst_INP_ASSERT_m(NULL != hQueue, __LINE__, 0, 0, 0);
    tst_INP_ASSERT_m(NULL != hRequest, __LINE__, 0, 0, 0);

    ntStatus = WdfRequestRetrieveOutputBuffer(hRequest, sizeof(DISK_GEOMETRY), 
        &pOutBuff, &ulOutLen);
    if (!NT_SUCCESS(ntStatus)) 
    {
        tst_ERR_m((tst_ERR_PARAMS_d, tst_XY_ERRMSG_d,
        //  012345678901234567890123456789012345678901234567890123456        
        //  WdfRequestRetrieveOutputBuffer failed with ntSts=0x%x.
            "%x",
            ntStatus));

        WdfRequestComplete(hRequest, ntStatus);
        return;
    };
    tst_MUST_ASSERT_m((ulOutLen >= sizeof(DISK_GEOMETRY)), __LINE__, ulOutLen, 0, 0);

    pPDODevExt = BusGetPDODevExtData(WdfIoQueueGetDevice(hQueue));
    tst_ERR_ASSERT_m(NULL != pPDODevExt, __LINE__, 0, 0, 0);

    ulOutLen = tst_Min_m(ulOutLen, sizeof(DISK_GEOMETRY));
    RtlCopyMemory(pOutBuff, &pPDODevExt->sDiskGeometry, ulOutLen);

    tst_TRC_m(tst_DBG_MAIN_d, (tst_TRC_PARAMS_d, 
        "IOCTL_DISK_GET_DRIVE_GEOMETRY Cylinders=%I64d TracksPerCylinder=%d"
        " SectorsPerTrack=%d BytesPerSector=%d MediaType=0x%x. OutLen=%d.", 
        pPDODevExt->sDiskGeometry.Cylinders.QuadPart,
        pPDODevExt->sDiskGeometry.TracksPerCylinder, 
        pPDODevExt->sDiskGeometry.SectorsPerTrack, 
        pPDODevExt->sDiskGeometry.BytesPerSector, 
        pPDODevExt->sDiskGeometry.MediaType, 
        ulOutLen));

    WdfRequestCompleteWithInformation(hRequest, STATUS_SUCCESS, ulOutLen);

};
//============================================================================//

//============================================================================//
VOID 
BusPDO_GetAddress(IN WDFQUEUE hQueue, IN WDFREQUEST hRequest)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PPDO_DEVICE_DATA pPDODevExt;
    PVOID pOutBuff;
    ULONG ulOutLen;
    
    PAGED_CODE();

    tst_INP_ASSERT_m(NULL != hQueue, __LINE__, 0, 0, 0);
    tst_INP_ASSERT_m(NULL != hRequest, __LINE__, 0, 0, 0);

    ntStatus = WdfRequestRetrieveOutputBuffer(hRequest, sizeof(SCSI_ADDRESS), 
        &pOutBuff, &ulOutLen);
    if (!NT_SUCCESS(ntStatus)) 
    {
        tst_ERR_m((tst_ERR_PARAMS_d, tst_XY_ERRMSG_d,
        //  012345678901234567890123456789012345678901234567890123456        
        //  WdfRequestRetrieveOutputBuffer failed with ntSts=0x%x.
            "%x",
            ntStatus));

        WdfRequestComplete(hRequest, ntStatus);
        return;
    };
    tst_MUST_ASSERT_m((ulOutLen >= sizeof(SCSI_ADDRESS)), __LINE__, ulOutLen, 0, 0);

    pPDODevExt = BusGetPDODevExtData(WdfIoQueueGetDevice(hQueue));
    tst_ERR_ASSERT_m(NULL != pPDODevExt, __LINE__, 0, 0, 0);

    ulOutLen = tst_Min_m(ulOutLen, sizeof(SCSI_ADDRESS));
    RtlCopyMemory(pOutBuff, &pPDODevExt->sScsiAddress, ulOutLen);

    tst_TRC_m(tst_DBG_MAIN_d, (tst_TRC_PARAMS_d, 
        "IOCTL_SCSI_GET_ADDRESS PortNumber=%d PathId=%d TargetId=%d Lun=%d. OutLen=%d.", 
        pPDODevExt->sScsiAddress.PortNumber,
        pPDODevExt->sScsiAddress.PathId,
        pPDODevExt->sScsiAddress.TargetId,
        pPDODevExt->sScsiAddress.Lun,
        ulOutLen));

    WdfRequestCompleteWithInformation(hRequest, STATUS_SUCCESS, ulOutLen);

};
//============================================================================//

//============================================================================//
VOID
BusPDO_InternalSCSI(IN WDFQUEUE hQueue, IN WDFREQUEST hRequest)
{
    WDF_REQUEST_PARAMETERS sReqParams;
    PSCSI_REQUEST_BLOCK pSrb;
    ULONG ulIoctl;

    PAGED_CODE();

    tst_FUNCIN_m();

    tst_INP_ASSERT_m(NULL != hQueue, __LINE__, 0, 0, 0);
    tst_INP_ASSERT_m(NULL != hRequest, __LINE__, 0, 0, 0);

    WDF_REQUEST_PARAMETERS_INIT(&sReqParams);
    WdfRequestGetParameters(hRequest, &sReqParams);
    pSrb = (PSCSI_REQUEST_BLOCK) sReqParams.Parameters.Others.Arg1;
    ulIoctl = sReqParams.Parameters.Others.IoControlCode;

    //Dump all SRB members.
    tst_TRC_m(tst_DBG_SRB_d, (tst_TRC_PARAMS_d, 
        "Dump SRB. Len=%d Function=0x%x SrbStatus=0x%x ScsiStatus=0x%x PathId=%d TargetId=%d Lun=%d "
        "QueueTag=0x%x QueueAction=0x%x QueueSortKey=0x%x TimeOutValue=%d SrbFlags=0x%x" 
        "DataTransferLength=%d DataBuffer=0x%x SenseInfoBufferLength=%d SenseInfoBuffer=0x%x " 
        "OriginalRequest=0x%x SrbExtension=0x%x NextSrb=0x%x CdbLength=%d Cdb[0]=0x%x", 
        pSrb->Length, pSrb->Function, pSrb->SrbStatus, pSrb->ScsiStatus, 
        pSrb->PathId, pSrb->TargetId, pSrb->Lun, 
        pSrb->QueueTag, pSrb->QueueAction, pSrb->QueueSortKey, pSrb->TimeOutValue, pSrb->SrbFlags, 
        pSrb->DataTransferLength, pSrb->DataBuffer, pSrb->SenseInfoBufferLength, pSrb->SenseInfoBuffer, 
        pSrb->OriginalRequest, pSrb->SrbExtension, pSrb->NextSrb, pSrb->CdbLength, pSrb->Cdb[0]));

    switch (ulIoctl)
    {
        case IOCTL_SCSI_EXECUTE_NONE:

            tst_TRC_m(tst_DBG_MAIN_d, (tst_TRC_PARAMS_d, 
                "IOCTL_SCSI_EXECUTE_NONE Function(0x%x).", 
                pSrb->Function));

            switch (pSrb->Function)
            {
                case SRB_FUNCTION_CLAIM_DEVICE:

                    pSrb->SrbStatus = SRB_STATUS_SUCCESS;
                    pSrb->DataBuffer = WdfDeviceWdmGetPhysicalDevice(WdfIoQueueGetDevice(hQueue));
                    ClearFlag(pSrb->SrbFlags, SRB_FLAGS_FREE_SENSE_BUFFER);

                    WdfRequestComplete(hRequest, STATUS_SUCCESS);
                    break;

                case SRB_FUNCTION_EXECUTE_SCSI:

                    //Really?
                    BusPDO_ScsiRequest(hQueue, hRequest, pSrb);
                    break;

                default:

                    pSrb->SrbStatus = SRB_STATUS_INVALID_REQUEST;

                    WdfRequestComplete(hRequest, STATUS_INVALID_DEVICE_REQUEST);
                    break;
            };
            break;

        case IOCTL_SCSI_EXECUTE_IN:

            tst_TRC_m(tst_DBG_MAIN_d, (tst_TRC_PARAMS_d, 
                "IOCTL_SCSI_EXECUTE_IN Function(0x%x).", 
                pSrb->Function));

            pSrb->SrbStatus = SRB_STATUS_INVALID_REQUEST;

            WdfRequestComplete(hRequest, STATUS_INVALID_DEVICE_REQUEST);
            break;

        case IOCTL_SCSI_EXECUTE_OUT:

            tst_TRC_m(tst_DBG_MAIN_d, (tst_TRC_PARAMS_d, 
                "IOCTL_SCSI_EXECUTE_OUT Function(0x%x).", 
                pSrb->Function));

            pSrb->SrbStatus = SRB_STATUS_INVALID_REQUEST;

            WdfRequestComplete(hRequest, STATUS_INVALID_DEVICE_REQUEST);
            break;

        default: // It is IRP_MJ_SCSI.

            BusPDO_ScsiRequest(hQueue, hRequest, pSrb);
            break;
    };

    tst_FUNCOUT_m();
};
//============================================================================//

//============================================================================//
VOID 
BusPDO_SrbReadCapacity(IN WDFQUEUE hQueue, IN WDFREQUEST hRequest, 
                       IN PSCSI_REQUEST_BLOCK pSrb)
{
    //DEL NTSTATUS ntStatus = STATUS_SUCCESS;
    PPDO_DEVICE_DATA pPDODevExt;
    tst_SCSI_READ_CAPACITY_ptr pCdb;
    PSENSE_DATA pSenseData;
    PREAD_CAPACITY_DATA pCapData;

    PAGED_CODE();

    tst_INP_ASSERT_m(NULL != hQueue, __LINE__, 0, 0, 0);
    tst_INP_ASSERT_m(NULL != hRequest, __LINE__, 0, 0, 0);
    tst_INP_ASSERT_m(NULL != pSrb, __LINE__, 0, 0, 0);

    pSrb->ScsiStatus = SRB_STATUS_ERROR;
    pSrb->SrbStatus = SRB_STATUS_ERROR;

    pPDODevExt = BusGetPDODevExtData(WdfIoQueueGetDevice(hQueue));
    tst_ERR_ASSERT_m(NULL != pPDODevExt, __LINE__, 0, 0, 0);

    tst_ERR_ASSERT_m(pSrb->DataTransferLength >= sizeof(READ_CAPACITY_DATA), __LINE__, 0, 0, 0);

    pCdb = (tst_SCSI_READ_CAPACITY_ptr) pSrb->Cdb;
    tst_ERR_ASSERT_m(0 == pCdb->PartialMediumIndicator, __LINE__, 0, 0, 0);

    pCapData = (PREAD_CAPACITY_DATA) pSrb->DataBuffer;
    tst_ERR_ASSERT_m(NULL != pCapData, __LINE__, 0, 0, 0);

    REVERSE_BYTES(&pCapData->LogicalBlockAddress, &pPDODevExt->ulLastSector);
    REVERSE_BYTES(&pCapData->BytesPerBlock, &pPDODevExt->ulBlockSize);

    pSenseData = (PSENSE_DATA) pSrb->SenseInfoBuffer;
    tst_ERR_ASSERT_m(NULL != pSenseData, __LINE__, 0, 0, 0);
    tst_ERR_ASSERT_m(pSrb->SenseInfoBufferLength >= sizeof(SENSE_DATA), __LINE__, 0, 0, 0);

    tst_TRC_m(tst_DBG_MAIN_d, (tst_TRC_PARAMS_d, 
        "SCSIOP_READ_CAPACITY (%d,%d).", 
        pPDODevExt->ulLastSector, pPDODevExt->ulBlockSize));

    pSrb->ScsiStatus = 0; // No SCSI error.
    pSrb->SrbStatus = SRB_STATUS_SUCCESS;

    WdfRequestCompleteWithInformation(hRequest, STATUS_SUCCESS, pSrb->DataTransferLength);

};
//============================================================================//

//============================================================================//
VOID
SrbReadWorkItemRoutine(IN WDFWORKITEM hWorkItem)
{
    PSENSE_DATA pSenseData;
    tst_SCSI_READ_ptr pCdb;
    PCHAR pDataBuffer;
    ULONG ulStartingBlock;
    USHORT usNOFBlocks;
    PIRP pOriginalIrp;
    ULONG ulMdlBufLen;
    ULONG ulMdlOffset;
    WDFDEVICE hPDODevice;
    PPDO_DEVICE_DATA pPDODevExt;
    PSCSI_REQUEST_BLOCK pSrb;
    WDFREQUEST hRequest;
    PWORK_ITEM_CONTEXT pWorkItemContext;

    hPDODevice = WdfWorkItemGetParentObject(hWorkItem);

    pWorkItemContext = BusGetPDOWorkItemContext(hWorkItem);
    hRequest = pWorkItemContext->hRequest;
    pSrb = pWorkItemContext->pSrb;

    WdfObjectDelete(hWorkItem);

    pSrb->ScsiStatus = SRB_STATUS_ERROR;
    pSrb->SrbStatus = SRB_STATUS_ERROR;

    pPDODevExt = BusGetPDODevExtData(hPDODevice);
    tst_ERR_ASSERT_m(NULL != pPDODevExt, __LINE__, 0, 0, 0);

    //TODO: Really? DataTransferLength should at least match ulMdlBuffLen.
    tst_ERR_ASSERT_m(pSrb->DataTransferLength >= sizeof(READ_CAPACITY_DATA), __LINE__, 0, 0, 0);

    pCdb = (tst_SCSI_READ_ptr) pSrb->Cdb;
    pOriginalIrp = (PIRP) pSrb->OriginalRequest;

    pSenseData = (PSENSE_DATA) pSrb->SenseInfoBuffer;
    tst_ERR_ASSERT_m(NULL != pSenseData, __LINE__, 0, 0, 0);
    tst_ERR_ASSERT_m(pSrb->SenseInfoBufferLength >= sizeof(SENSE_DATA), __LINE__, 0, 0, 0);

    pDataBuffer = (PCHAR) MmGetSystemAddressForMdlSafe(pOriginalIrp->MdlAddress, NormalPagePriority);
    if (!MmIsAddressValid(pDataBuffer))
    {
        tst_TRC_m(tst_DBG_MAIN_d, (tst_TRC_PARAMS_d, 
            "SCSIOP_READ failed beacuse of invalid buffer (0x%x) address.",
            pDataBuffer));

        pSrb->DataTransferLength = 0;
        pSrb->SrbStatus = SRB_STATUS_ERROR;

        WdfRequestComplete(hRequest, STATUS_INVALID_DEVICE_REQUEST);
        return;
    }

    tst_ERR_ASSERT_m(NULL != pDataBuffer, __LINE__, 0, 0, 0);
    ulMdlBufLen = MmGetMdlByteCount(pOriginalIrp->MdlAddress);
    ulMdlOffset = MmGetMdlByteOffset(pOriginalIrp->MdlAddress);

    tst_ERR_ASSERT_m(ulMdlBufLen >= pSrb->DataTransferLength, __LINE__, 0, 0, 0);
    
    REVERSE_BYTES(&ulStartingBlock, &pCdb->LogicalBlockAddress);
    REVERSE_BYTES_SHORT(&usNOFBlocks, &pCdb->TransferBlocks);
    
    // Used to indicate that an SRB is the result of a paging operation.
    // #define SRB_CLASS_FLAGS_PAGING            0x40000000
    // Set FlagOn(pSrb->SrbFlags, 0x40000000)
    if (FlagOn(pOriginalIrp->Flags, IRP_PAGING_IO) ||
        FlagOn(pOriginalIrp->Flags, IRP_SYNCHRONOUS_PAGING_IO))
    {
        pDataBuffer += (ULONG) pSrb->DataBuffer;
    };

    // Allways transfer only pSrb->DataTransferLength bytes.

    //TEST ONLY! 
    RtlCopyMemory(pDataBuffer, pPDODevExt->bBuffer + (ulStartingBlock * tst_BYTESPERSECTOR_d), 
        pSrb->DataTransferLength);
    //TEST ONLY!

    tst_TRC_m(tst_DBG_MAIN_d, (tst_TRC_PARAMS_d, 
        "SCSIOP_READ Block(%d,%d)Buff(0x%x,%d)Mdl(0x%x,0x%x,%d,%d).", 
        ulStartingBlock, usNOFBlocks, 
        pDataBuffer, pSrb->DataTransferLength,
        pSrb->SrbFlags, pSrb->DataBuffer, ulMdlBufLen, ulMdlOffset));

    pSrb->ScsiStatus = 0; // No SCSI error.
    pSrb->SrbStatus = SRB_STATUS_SUCCESS;

    WdfRequestCompleteWithInformation(hRequest, STATUS_SUCCESS, pSrb->DataTransferLength);

};
//============================================================================//

//============================================================================//
VOID 
BusPDO_SrbRead(IN WDFQUEUE hQueue, IN WDFREQUEST hRequest, 
               IN PSCSI_REQUEST_BLOCK pSrb)
{
    NTSTATUS ntStatus;
    WDF_OBJECT_ATTRIBUTES workitem_Attributes;
    WDF_WORKITEM_CONFIG workitem_Config;
    WDFWORKITEM hWorkItem;
    PWORK_ITEM_CONTEXT pWorkItemContext;

    PAGED_CODE();

    tst_INP_ASSERT_m(NULL != hQueue, __LINE__, 0, 0, 0);
    tst_INP_ASSERT_m(NULL != hRequest, __LINE__, 0, 0, 0);
    tst_INP_ASSERT_m(NULL != pSrb, __LINE__, 0, 0, 0);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&workitem_Attributes, WORK_ITEM_CONTEXT);
    workitem_Attributes.ParentObject = WdfIoQueueGetDevice(hQueue);

    WDF_WORKITEM_CONFIG_INIT(&workitem_Config, SrbReadWorkItemRoutine);
    workitem_Config.AutomaticSerialization = FALSE;

    ntStatus = WdfWorkItemCreate(&workitem_Config, &workitem_Attributes, &hWorkItem);
    if (!NT_SUCCESS(ntStatus)) 
    {
        tst_TRC_m(tst_DBG_MAIN_d, (tst_TRC_PARAMS_d, 
            "SCSIOP_READ failed in create work item. ntSts=0x%x.",
            ntStatus));

        pSrb->DataTransferLength = 0;
        pSrb->SrbStatus = SRB_STATUS_ERROR;

        WdfRequestComplete(hRequest, ntStatus);
        return;
    }

    pWorkItemContext = BusGetPDOWorkItemContext(hWorkItem);
    pWorkItemContext->hRequest = hRequest;
    pWorkItemContext->pSrb = pSrb;

    // Execute this work item.
    WdfWorkItemEnqueue(hWorkItem);

};
//============================================================================//

//============================================================================//
VOID
SrbWriteWorkItemRoutine(IN WDFWORKITEM hWorkItem)
{
    PSENSE_DATA pSenseData;
    tst_SCSI_WRITE_ptr pCdb;
    PCHAR pDataBuffer;
    ULONG ulStartingBlock;
    USHORT usNOFBlocks;
    PIRP pOriginalIrp;
    ULONG ulMdlBufLen;
    ULONG ulMdlOffset;
    WDFDEVICE hPDODevice;
    PPDO_DEVICE_DATA pPDODevExt;
    PSCSI_REQUEST_BLOCK pSrb;
    WDFREQUEST hRequest;
    PWORK_ITEM_CONTEXT pWorkItemContext;

    hPDODevice = WdfWorkItemGetParentObject(hWorkItem);

    pWorkItemContext = BusGetPDOWorkItemContext(hWorkItem);
    hRequest = pWorkItemContext->hRequest;
    pSrb = pWorkItemContext->pSrb;

    WdfObjectDelete(hWorkItem);

    pSrb->ScsiStatus = SRB_STATUS_ERROR;
    pSrb->SrbStatus = SRB_STATUS_ERROR;

    pPDODevExt = BusGetPDODevExtData(hPDODevice);
    tst_ERR_ASSERT_m(NULL != pPDODevExt, __LINE__, 0, 0, 0);

    //TODO: Check if really? as for SCSIOP_READ.
    tst_ERR_ASSERT_m(pSrb->DataTransferLength >= sizeof(READ_CAPACITY_DATA), __LINE__, 0, 0, 0);

    pCdb = (tst_SCSI_WRITE_ptr) pSrb->Cdb;
    pOriginalIrp = (PIRP) pSrb->OriginalRequest;

    pSenseData = (PSENSE_DATA) pSrb->SenseInfoBuffer;
    tst_ERR_ASSERT_m(NULL != pSenseData, __LINE__, 0, 0, 0);
    tst_ERR_ASSERT_m(pSrb->SenseInfoBufferLength >= sizeof(SENSE_DATA), __LINE__, 0, 0, 0);

    pDataBuffer = (PCHAR) MmGetSystemAddressForMdlSafe(pOriginalIrp->MdlAddress, NormalPagePriority);
    if (!MmIsAddressValid(pDataBuffer))
    {
        tst_TRC_m(tst_DBG_MAIN_d, (tst_TRC_PARAMS_d, 
            "SCSIOP_WRITE failed beacuse of invalid buffer (0x%x) address.",
            pDataBuffer));

        pSrb->DataTransferLength = 0;
        pSrb->SrbStatus = SRB_STATUS_ERROR;

        WdfRequestComplete(hRequest, STATUS_INVALID_DEVICE_REQUEST);
        return;
    }

    tst_ERR_ASSERT_m(NULL != pDataBuffer, __LINE__, 0, 0, 0);
    ulMdlBufLen = MmGetMdlByteCount(pOriginalIrp->MdlAddress);
    ulMdlOffset = MmGetMdlByteOffset(pOriginalIrp->MdlAddress);

    tst_ERR_ASSERT_m(ulMdlBufLen >= pSrb->DataTransferLength, __LINE__, 0, 0, 0);
    
    REVERSE_BYTES(&ulStartingBlock, &pCdb->LogicalBlockAddress);
    REVERSE_BYTES_SHORT(&usNOFBlocks, &pCdb->TransferBlocks);
    
    // Used to indicate that an SRB is the result of a paging operation.
    // #define SRB_CLASS_FLAGS_PAGING            0x40000000
    // Set FlagOn(pSrb->SrbFlags, 0x40000000)
    if (FlagOn(pOriginalIrp->Flags, IRP_PAGING_IO) ||
        FlagOn(pOriginalIrp->Flags, IRP_SYNCHRONOUS_PAGING_IO))
    {
        pDataBuffer += (ULONG) pSrb->DataBuffer;
    };

    // Allways transfer only pSrb->DataTransferLength bytes.

    //TEST ONLY! 
    RtlCopyMemory(pPDODevExt->bBuffer + (ulStartingBlock * tst_BYTESPERSECTOR_d), 
        pDataBuffer, pSrb->DataTransferLength);
    //TEST ONLY! 

    tst_TRC_m(tst_DBG_MAIN_d, (tst_TRC_PARAMS_d, 
        "SCSIOP_WRITE Block(%d,%d)Buff(0x%x,%d)Mdl(0x%x,0x%x,%d,%d).", 
        ulStartingBlock, usNOFBlocks, 
        pDataBuffer, pSrb->DataTransferLength,
        pSrb->SrbFlags, pSrb->DataBuffer, ulMdlBufLen, ulMdlOffset));

    pSrb->ScsiStatus = 0; // No SCSI error.
    pSrb->SrbStatus = SRB_STATUS_SUCCESS;

    WdfRequestCompleteWithInformation(hRequest, STATUS_SUCCESS, pSrb->DataTransferLength);

};
//============================================================================//

//============================================================================//
VOID 
BusPDO_SrbWrite(IN WDFQUEUE hQueue, IN WDFREQUEST hRequest, 
                IN PSCSI_REQUEST_BLOCK pSrb)
{
    NTSTATUS ntStatus;
    WDF_OBJECT_ATTRIBUTES workitem_Attributes;
    WDF_WORKITEM_CONFIG workitem_Config;
    WDFWORKITEM hWorkItem;
    PWORK_ITEM_CONTEXT pWorkItemContext;

    PAGED_CODE();

    tst_INP_ASSERT_m(NULL != hQueue, __LINE__, 0, 0, 0);
    tst_INP_ASSERT_m(NULL != hRequest, __LINE__, 0, 0, 0);
    tst_INP_ASSERT_m(NULL != pSrb, __LINE__, 0, 0, 0);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&workitem_Attributes, WORK_ITEM_CONTEXT);
    workitem_Attributes.ParentObject = WdfIoQueueGetDevice(hQueue);

    WDF_WORKITEM_CONFIG_INIT(&workitem_Config, SrbWriteWorkItemRoutine);
    workitem_Config.AutomaticSerialization = FALSE;

    ntStatus = WdfWorkItemCreate(&workitem_Config, &workitem_Attributes, &hWorkItem);
    if (!NT_SUCCESS(ntStatus)) 
    {
        tst_TRC_m(tst_DBG_MAIN_d, (tst_TRC_PARAMS_d, 
            "SCSIOP_WRITE failed in create work item. ntSts=0x%x.",
            ntStatus));

        pSrb->DataTransferLength = 0;
        pSrb->SrbStatus = SRB_STATUS_ERROR;

        WdfRequestComplete(hRequest, ntStatus);
        return;
    }

    pWorkItemContext = BusGetPDOWorkItemContext(hWorkItem);
    pWorkItemContext->hRequest = hRequest;
    pWorkItemContext->pSrb = pSrb;

    // Execute this work item.
    WdfWorkItemEnqueue(hWorkItem);

};
//============================================================================//

//============================================================================//
VOID 
BusPDO_SrbModeSense(IN WDFQUEUE hQueue, IN WDFREQUEST hRequest, 
                    IN PSCSI_REQUEST_BLOCK pSrb)
{
    PPDO_DEVICE_DATA pPDODevExt;
    PSENSE_DATA pSenseData;
    tst_SCSI_MODE_SENSE_ptr pCdb;
    PMODE_PARAMETER_HEADER pModeData;
    PCHAR pDataBuffer;

    PAGED_CODE();

    tst_INP_ASSERT_m(NULL != hQueue, __LINE__, 0, 0, 0);
    tst_INP_ASSERT_m(NULL != hRequest, __LINE__, 0, 0, 0);
    tst_INP_ASSERT_m(NULL != pSrb, __LINE__, 0, 0, 0);

    pSrb->ScsiStatus = SRB_STATUS_ERROR;
    pSrb->SrbStatus = SRB_STATUS_ERROR;

    pPDODevExt = BusGetPDODevExtData(WdfIoQueueGetDevice(hQueue));
    tst_ERR_ASSERT_m(NULL != pPDODevExt, __LINE__, 0, 0, 0);

    pCdb = (tst_SCSI_MODE_SENSE_ptr) pSrb->Cdb;

    pDataBuffer = (PCHAR) pSrb->DataBuffer;
    tst_ERR_ASSERT_m(NULL != pDataBuffer, __LINE__, 0, 0, 0);

    pSenseData = (PSENSE_DATA) pSrb->SenseInfoBuffer;
    tst_ERR_ASSERT_m(NULL != pSenseData, __LINE__, 0, 0, 0);
    tst_ERR_ASSERT_m(pSrb->SenseInfoBufferLength >= sizeof(SENSE_DATA), __LINE__, 0, 0, 0);

    tst_TRC_m(tst_DBG_MAIN_d, (tst_TRC_PARAMS_d, 
        "SCSIOP_MODE_SENSE Pcf=0x%x PageCode=0x%x AllocLen=%d DataTransLen=%d.", 
        pCdb->Pcf, pCdb->PageCode, pCdb->AllocationLength, pSrb->DataTransferLength));

    switch (pCdb->PageCode)
    {
        case MODE_PAGE_FAULT_REPORTING:
        case MODE_SENSE_RETURN_ALL:
        case MODE_PAGE_CACHING:
        default:
            RtlZeroMemory(pDataBuffer, pSrb->DataTransferLength);
            pModeData = (PMODE_PARAMETER_HEADER) pDataBuffer;
            pModeData->ModeDataLength = sizeof(MODE_PARAMETER_HEADER) - sizeof(pModeData->ModeDataLength);
            break;
    };

    pSrb->ScsiStatus = 0; // No SCSI error.
    pSrb->SrbStatus = SRB_STATUS_SUCCESS;

    WdfRequestCompleteWithInformation(hRequest, STATUS_SUCCESS, pSrb->DataTransferLength);

};
//============================================================================//

//============================================================================//
VOID
BusPDO_SrbTestUnitReady(IN WDFQUEUE hQueue, IN WDFREQUEST hRequest, 
                    IN PSCSI_REQUEST_BLOCK pSrb)
{
    PSENSE_DATA pSenseData;
    tst_SCSI_TEST_UNIT_READY_ptr pCdb;

    PAGED_CODE();

    tst_INP_ASSERT_m(NULL != hQueue, __LINE__, 0, 0, 0);
    tst_INP_ASSERT_m(NULL != hRequest, __LINE__, 0, 0, 0);
    tst_INP_ASSERT_m(NULL != pSrb, __LINE__, 0, 0, 0);

    pSrb->ScsiStatus = SRB_STATUS_ERROR;
    pSrb->SrbStatus = SRB_STATUS_ERROR;

    pCdb = (tst_SCSI_TEST_UNIT_READY_ptr) pSrb->Cdb;

    pSenseData = (PSENSE_DATA) pSrb->SenseInfoBuffer;
    tst_ERR_ASSERT_m(NULL != pSenseData, __LINE__, 0, 0, 0);
    tst_ERR_ASSERT_m(pSrb->SenseInfoBufferLength >= sizeof(SENSE_DATA), __LINE__, 0, 0, 0);

    tst_TRC_m(tst_DBG_MAIN_d, (tst_TRC_PARAMS_d, 
        "SCSIOP_TEST_UNIT_READY."));

    pSrb->ScsiStatus = 0; // No SCSI error.
    pSrb->SrbStatus = SRB_STATUS_SUCCESS;

    WdfRequestCompleteWithInformation(hRequest, STATUS_SUCCESS, pSrb->DataTransferLength);

};
//============================================================================//

//============================================================================//
VOID 
BusPDO_SrbVerify(IN WDFQUEUE hQueue, IN WDFREQUEST hRequest, 
                 IN PSCSI_REQUEST_BLOCK pSrb)
{
    PSENSE_DATA pSenseData;
    tst_SCSI_VERIFY_ptr pCdb;
    ULONG ulStartingBlock;
    USHORT usNOFBlocks;
    PPDO_DEVICE_DATA pPDODevExt;

    PAGED_CODE();

    tst_INP_ASSERT_m(NULL != hQueue, __LINE__, 0, 0, 0);
    tst_INP_ASSERT_m(NULL != hRequest, __LINE__, 0, 0, 0);
    tst_INP_ASSERT_m(NULL != pSrb, __LINE__, 0, 0, 0);

    pSrb->ScsiStatus = SRB_STATUS_ERROR;
    pSrb->SrbStatus = SRB_STATUS_ERROR;

    pPDODevExt = BusGetPDODevExtData(WdfIoQueueGetDevice(hQueue));
    tst_ERR_ASSERT_m(NULL != pPDODevExt, __LINE__, 0, 0, 0);

    pCdb = (tst_SCSI_VERIFY_ptr) pSrb->Cdb;

    pSenseData = (PSENSE_DATA) pSrb->SenseInfoBuffer;
    tst_ERR_ASSERT_m(NULL != pSenseData, __LINE__, 0, 0, 0);
    tst_ERR_ASSERT_m(pSrb->SenseInfoBufferLength >= sizeof(SENSE_DATA), __LINE__, 0, 0, 0);

    REVERSE_BYTES(&ulStartingBlock, &pCdb->LogicalBlockAddress);
    REVERSE_BYTES_SHORT(&usNOFBlocks, &pCdb->VerifyBlocks);
    
    tst_TRC_m(tst_DBG_MAIN_d, (tst_TRC_PARAMS_d, 
        "SCSIOP_VERIFY Block(%d,%d).", 
        ulStartingBlock, usNOFBlocks));

    pSrb->ScsiStatus = 0; // No SCSI error.
    pSrb->SrbStatus = SRB_STATUS_SUCCESS;

    WdfRequestCompleteWithInformation(hRequest, STATUS_SUCCESS, pSrb->DataTransferLength);

};
//============================================================================//

//============================================================================//
VOID 
BusPDO_SrbMediumRemoval(IN WDFQUEUE hQueue, IN WDFREQUEST hRequest, 
                        IN PSCSI_REQUEST_BLOCK pSrb)
{
    PSENSE_DATA pSenseData;
    tst_SCSI_MEDIAREMOVAL_ptr pCdb;
    PPDO_DEVICE_DATA pPDODevExt;

    PAGED_CODE();

    tst_INP_ASSERT_m(NULL != hQueue, __LINE__, 0, 0, 0);
    tst_INP_ASSERT_m(NULL != hRequest, __LINE__, 0, 0, 0);
    tst_INP_ASSERT_m(NULL != pSrb, __LINE__, 0, 0, 0);

    pSrb->ScsiStatus = SRB_STATUS_ERROR;
    pSrb->SrbStatus = SRB_STATUS_ERROR;

    pPDODevExt = BusGetPDODevExtData(WdfIoQueueGetDevice(hQueue));
    tst_ERR_ASSERT_m(NULL != pPDODevExt, __LINE__, 0, 0, 0);

    pCdb = (tst_SCSI_MEDIAREMOVAL_ptr) pSrb->Cdb;

    pSenseData = (PSENSE_DATA) pSrb->SenseInfoBuffer;
    tst_ERR_ASSERT_m(NULL != pSenseData, __LINE__, 0, 0, 0);
    tst_ERR_ASSERT_m(pSrb->SenseInfoBufferLength >= sizeof(SENSE_DATA), __LINE__, 0, 0, 0);

    tst_TRC_m(tst_DBG_MAIN_d, (tst_TRC_PARAMS_d, 
        "SCSIOP_MEDIUMREMOVAL Prevent(%s).", 
        ((pCdb->Prevent != 0) ? "TRUE":"FALSE")));

    pSrb->ScsiStatus = 0; // No SCSI error.
    pSrb->SrbStatus = SRB_STATUS_SUCCESS;

    WdfRequestCompleteWithInformation(hRequest, STATUS_SUCCESS, pSrb->DataTransferLength);

};
//============================================================================//

//============================================================================//
VOID 
BusPDO_SrbLoadUnload(IN WDFQUEUE hQueue, IN WDFREQUEST hRequest, 
                     IN PSCSI_REQUEST_BLOCK pSrb)
{
    PSENSE_DATA pSenseData;
    tst_SCSI_LOADUNLOAD_ptr pCdb;
    PPDO_DEVICE_DATA pPDODevExt;

    PAGED_CODE();

    tst_INP_ASSERT_m(NULL != hQueue, __LINE__, 0, 0, 0);
    tst_INP_ASSERT_m(NULL != hRequest, __LINE__, 0, 0, 0);
    tst_INP_ASSERT_m(NULL != pSrb, __LINE__, 0, 0, 0);

    pSrb->ScsiStatus = SRB_STATUS_ERROR;
    pSrb->SrbStatus = SRB_STATUS_ERROR;

    pPDODevExt = BusGetPDODevExtData(WdfIoQueueGetDevice(hQueue));
    tst_ERR_ASSERT_m(NULL != pPDODevExt, __LINE__, 0, 0, 0);

    pCdb = (tst_SCSI_LOADUNLOAD_ptr) pSrb->Cdb;

    pSenseData = (PSENSE_DATA) pSrb->SenseInfoBuffer;
    tst_ERR_ASSERT_m(NULL != pSenseData, __LINE__, 0, 0, 0);
    tst_ERR_ASSERT_m(pSrb->SenseInfoBufferLength >= sizeof(SENSE_DATA), __LINE__, 0, 0, 0);

    tst_TRC_m(tst_DBG_MAIN_d, (tst_TRC_PARAMS_d, 
        "SCSIOP_LOADUNLOAD Immed(%s) LoEj(%s) Start(%s).", 
        ((pCdb->Immed != 0) ? "TRUE":"FALSE"),
        ((pCdb->LoEj != 0) ? "TRUE":"FALSE"),
        ((pCdb->Start != 0) ? "TRUE":"FALSE")));

    pSrb->ScsiStatus = 0; // No SCSI error.
    pSrb->SrbStatus = SRB_STATUS_SUCCESS;

    WdfRequestCompleteWithInformation(hRequest, STATUS_SUCCESS, pSrb->DataTransferLength);

};
//============================================================================//

//============================================================================//
VOID 
BusPDO_ScsiRequest(IN WDFQUEUE hQueue, IN WDFREQUEST hRequest, 
                   IN PSCSI_REQUEST_BLOCK pSrb)
{
    PCDB pCdb;
    PSRB_IO_CONTROL pSio;
    
    PAGED_CODE();

    tst_INP_ASSERT_m(NULL != hQueue, __LINE__, 0, 0, 0);
    tst_INP_ASSERT_m(NULL != hRequest, __LINE__, 0, 0, 0);
    tst_INP_ASSERT_m(NULL != pSrb, __LINE__, 0, 0, 0);

    switch (pSrb->Function)
    {
        case SRB_FUNCTION_EXECUTE_SCSI:

            pCdb = (PCDB) pSrb->Cdb;
            switch (pCdb->CDB6GENERIC.OperationCode)
            {
                case SCSIOP_READ_CAPACITY:
                    BusPDO_SrbReadCapacity(hQueue, hRequest, pSrb);
                    break;

                case SCSIOP_READ:
                    BusPDO_SrbRead(hQueue, hRequest, pSrb);
                    break;

                case SCSIOP_WRITE:
                    BusPDO_SrbWrite(hQueue, hRequest, pSrb);
                    break;

                case SCSIOP_MODE_SENSE:
                    BusPDO_SrbModeSense(hQueue, hRequest, pSrb);
                    break;

                case SCSIOP_TEST_UNIT_READY:
                    BusPDO_SrbTestUnitReady(hQueue, hRequest, pSrb);
                    break;

                case SCSIOP_VERIFY:
                    BusPDO_SrbVerify(hQueue, hRequest, pSrb);
                    break;

                case SCSIOP_MEDIUM_REMOVAL:
                    BusPDO_SrbMediumRemoval(hQueue, hRequest, pSrb);
                    break;

                case SCSIOP_LOAD_UNLOAD:
                    BusPDO_SrbLoadUnload(hQueue, hRequest, pSrb);
                    break;

                default: // Unknown SRB operation code.
                    tst_TRC_m(tst_DBG_MAIN_d, (tst_TRC_PARAMS_d, 
                        "UNIMPLEMENTED. SRB_FUNCTION_EXECUTE_SCSI CDB(%d,0x%x).", 
                        pSrb->CdbLength, pCdb->CDB6GENERIC.OperationCode));

                    pSrb->ScsiStatus = SRB_STATUS_ERROR;
                    pSrb->SrbStatus = SRB_STATUS_ERROR;

                    WdfRequestComplete(hRequest, STATUS_INVALID_DEVICE_REQUEST);
                    break;
            };
            break;

        case SRB_FUNCTION_IO_CONTROL:

            pSio = (PSRB_IO_CONTROL) pSrb->DataBuffer;

            tst_TRC_m(tst_DBG_MAIN_d, (tst_TRC_PARAMS_d, 
                "UNIMPLEMENTED. SRB_FUNCTION_IO_CONTROL SIO(%d,0x%x,%d).", 
                pSrb->DataTransferLength, pSio->ControlCode, pSio->Length));
            // This way comes in some SMART disk IOCTLs
            // e.g. IOCTL_SCSI_MINIPORT_IDENTIFY send from DiskPerformSmartCommand (disk.c).

            // Success request.
            // pSio->ReturnCode = STATUS_SUCCESS;
            // pSRB->SrbStatus = SRB_STATUS_SUCCESS;
            // *pulLen = pSio->Length;
                                                        
            //TODO IOCTL_SCSI_MINIPORT_IDENTIFY:
            //     UNIMPLEMENTED. SRB_FUNCTION_IO_CONTROL SIO(572,0x1b0501,544)

            pSrb->ScsiStatus = SRB_STATUS_ERROR;
            pSrb->SrbStatus = SRB_STATUS_INVALID_REQUEST;

            WdfRequestComplete(hRequest, STATUS_INVALID_DEVICE_REQUEST);
            break;

        case SRB_FUNCTION_FLUSH:

            tst_TRC_m(tst_DBG_MAIN_d, (tst_TRC_PARAMS_d, 
                "SRB_FUNCTION_FLUSH."));

            // Success request.
            pSrb->SrbStatus = SRB_STATUS_SUCCESS;
            WdfRequestComplete(hRequest, STATUS_SUCCESS);
            break;

        case SRB_FUNCTION_SHUTDOWN:

            tst_TRC_m(tst_DBG_MAIN_d, (tst_TRC_PARAMS_d, 
                "SRB_FUNCTION_SHUTDOWN."));

            // Success request.
            pSrb->SrbStatus = SRB_STATUS_SUCCESS;
            WdfRequestComplete(hRequest, STATUS_SUCCESS);
            break;

        default: // Unknown SRB function.

            tst_TRC_m(tst_DBG_MAIN_d, (tst_TRC_PARAMS_d, 
                "UNIMPLEMENTED. Unknown SRB(0x%x) function(0x%x).", 
                pSrb, pSrb->Function));
                                 
            pSrb->ScsiStatus = SRB_STATUS_ERROR;
            pSrb->SrbStatus = SRB_STATUS_INVALID_REQUEST;

            WdfRequestComplete(hRequest, STATUS_INVALID_DEVICE_REQUEST);
            break;
    };

};
//============================================================================//

// End Of File
