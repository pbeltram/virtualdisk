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

#ifndef _PDOFUNC_H_
#define _PDOFUNC_H_

//============================================================================//
// Disk geometry
#define tst_NOFCYLINDERS_d       0x10
#define tst_TRACKSPERCYLINDER_d  0x10
#define tst_SECTORSPERTRACK_d    0x100
#define tst_BYTESPERSECTOR_d     0x200

// Disk on bus IDs
#define tst_DISKPATHID_d     0
//============================================================================//

//============================================================================//
// This is PDO device-extension.
typedef struct _PDO_DEVICE_DATA
{
    // Unique serail number of the device on the bus
    ULONG SerialNo;

    DISK_GEOMETRY sDiskGeometry;
    SCSI_ADDRESS sScsiAddress;

    ULONG ulLastSector;
    ULONG ulBlockSize;

    STORAGE_DEVICE_DESCRIPTOR sDiskDesc;
    CHAR szRawProperties[36];
    CHAR szVendorId[8];
    CHAR szProductId[16];
    CHAR szProductRevision[8];
    CHAR szSerialNumber[16];

    //TEST ONLY!
    //TODO: Replace with something else. E.g. handle to sparse file.
    CHAR bBuffer[((tst_NOFCYLINDERS_d * tst_TRACKSPERCYLINDER_d * tst_SECTORSPERTRACK_d) + 1) * tst_BYTESPERSECTOR_d];
    //TEST ONLY! 

} PDO_DEVICE_DATA, *PPDO_DEVICE_DATA;
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(PDO_DEVICE_DATA, BusGetPDODevExtData)


// General purpose workitem context used in dispatching work to
// system worker thread to be executed at PASSIVE_LEVEL.
typedef struct _WORK_ITEM_CONTEXT {
    WDFREQUEST hRequest;
    PSCSI_REQUEST_BLOCK pSrb;
} WORK_ITEM_CONTEXT, *PWORK_ITEM_CONTEXT;
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(WORK_ITEM_CONTEXT, BusGetPDOWorkItemContext)
//============================================================================//

//============================================================================//
#pragma pack(push, _tst_SCSI_READ_CAPACITY_tag, 1)
typedef struct _tst_SCSI_READ_CAPACITY_tag 
{
    UCHAR OperationCode;
    UCHAR RelativeAddress : 1;
    UCHAR Reserved1 : 4;
    UCHAR LogicalUnitNumber : 3;
    ULONG LogicalBlockAddress;
    UCHAR Reserved2;
    UCHAR Reserved3;
    UCHAR PartialMediumIndicator : 1;
    UCHAR Reserved4 : 7;
    UCHAR Link : 1;
    UCHAR Flag : 1;
    UCHAR Reserved5 : 4;
    UCHAR VendorUnique : 2;
} tst_SCSI_READ_CAPACITY_t, *tst_SCSI_READ_CAPACITY_ptr;
#pragma pack(pop, _tst_SCSI_READ_CAPACITY_tag)

#pragma pack(push, _tst_SCSI_READ_tag, 1)
typedef struct _tst_SCSI_READ_tag 
{
    UCHAR OperationCode;
    UCHAR RelativeAddress : 1;
    UCHAR Reserved1 : 2;
    UCHAR ForceUnitAccess : 1;
    UCHAR DisablePageOut : 1;
    UCHAR LogicalUnitNumber : 3;
    ULONG LogicalBlockAddress;
    UCHAR Reserved2;
    USHORT TransferBlocks;
    UCHAR Link : 1;
    UCHAR Flag : 1;
    UCHAR Reserved5 : 4;
    UCHAR VendorUnique : 2;
} tst_SCSI_READ_t, *tst_SCSI_READ_ptr;
#pragma pack(pop, _tst_SCSI_READ_tag)

#pragma pack(push, _tst_SCSI_WRITE_tag, 1)
typedef struct _tst_SCSI_WRITE_tag 
{
    UCHAR OperationCode;
    UCHAR RelativeAddress : 1;
    UCHAR Reserved1 : 2;
    UCHAR ForceUnitAccess : 1;
    UCHAR DisablePageOut : 1;
    UCHAR LogicalUnitNumber : 3;
    ULONG LogicalBlockAddress;
    UCHAR Reserved2;
    USHORT TransferBlocks;
    UCHAR Link : 1;
    UCHAR Flag : 1;
    UCHAR Reserved5 : 4;
    UCHAR VendorUnique : 2;
} tst_SCSI_WRITE_t, *tst_SCSI_WRITE_ptr;
#pragma pack(pop, _tst_SCSI_WRITE_tag)

#pragma pack(push, _tst_SCSI_MODE_SENSE_tag, 1)
typedef struct _tst_SCSI_MODE_SENSE_tag 
{
    UCHAR OperationCode;
    UCHAR Reserved1 : 3;
    UCHAR Dbd : 1;
    UCHAR Reserved2 : 1;
    UCHAR LogicalUnitNumber : 3;
    UCHAR PageCode : 6;
    UCHAR Pcf : 2;
    UCHAR Reserved3;
    UCHAR AllocationLength;
    UCHAR Link : 1;
    UCHAR Flag : 1;
    UCHAR Reserved5 : 4;
    UCHAR VendorUnique : 2;
} tst_SCSI_MODE_SENSE_t, *tst_SCSI_MODE_SENSE_ptr;
#pragma pack(pop, _tst_SCSI_MODE_SENSE_tag)

#pragma pack(push, _tst_SCSI_TEST_UNIT_READY_tag, 1)
typedef struct _tst_SCSI_TEST_UNIT_READY_tag 
{
    UCHAR OperationCode;
    UCHAR Reserved1 : 3;
    UCHAR Dbd : 1;
    UCHAR Reserved2 : 1;
    UCHAR LogicalUnitNumber : 3;
    UCHAR PageCode : 6;
    UCHAR Pcf : 2;
    UCHAR Reserved3;
    UCHAR AllocationLength;
    UCHAR Link : 1;
    UCHAR Flag : 1;
    UCHAR Reserved5 : 4;
    UCHAR VendorUnique : 2;
} tst_SCSI_TEST_UNIT_READY_t, *tst_SCSI_TEST_UNIT_READY_ptr;
#pragma pack(pop, _tst_SCSI_TEST_UNIT_READY_tag)

#pragma pack(push, _tst_SCSI_VERIFY_tag, 1)
typedef struct _tst_SCSI_VERIFY_tag 
{
    UCHAR OperationCode;
    UCHAR Reserved0 : 1;
    UCHAR ByteCheck : 1;
    UCHAR Reserved1 : 3;
    UCHAR LogicalUnitNumber : 3;
    ULONG LogicalBlockAddress;
    UCHAR Reserved2;
    USHORT VerifyBlocks;
    UCHAR Link : 1;
    UCHAR Flag : 1;
    UCHAR Reserved5 : 4;
    UCHAR VendorUnique : 2;
} tst_SCSI_VERIFY_t, *tst_SCSI_VERIFY_ptr;
#pragma pack(pop, _tst_SCSI_VERIFY_tag)

#pragma pack(push, _tst_SCSI_MEDIAREMOVAL_tag, 1)
typedef struct _tst_SCSI_MEDIAREMOVAL_tag 
{
    UCHAR OperationCode;
    UCHAR Reserved1 : 5;
    UCHAR LogicalUnitNumber : 3;
    UCHAR Reserved2;
    UCHAR Reserved3;
    UCHAR Prevent : 1;
    UCHAR Reserved4 : 7;
    UCHAR Link : 1;
    UCHAR Flag : 1;
    UCHAR Reserved5 : 4;
    UCHAR VendorUnique : 2;
} _tst_SCSI_MEDIAREMOVAL_tag, *tst_SCSI_MEDIAREMOVAL_ptr;
#pragma pack(pop, _tst_SCSI_MEDIAREMOVAL_tag)

#pragma pack(push, _tst_SCSI_LOADUNLOAD_tag, 1)
typedef struct _tst_SCSI_LOADUNLOAD_tag 
{
    UCHAR OperationCode;
    UCHAR Immed : 1;
    UCHAR Reserved1 : 4;
    UCHAR LogicalUnitNumber : 3;
    UCHAR Reserved2;
    UCHAR Reserved3;
    UCHAR Start : 1;
    UCHAR LoEj : 1;
    UCHAR Reserved4 : 5;
    UCHAR Link : 1;
    UCHAR Flag : 1;
    UCHAR Reserved5 : 4;
    UCHAR VendorUnique : 2;
} _tst_SCSI_LOADUNLOAD_tag, *tst_SCSI_LOADUNLOAD_ptr;
#pragma pack(pop, _tst_SCSI_LOADUNLOAD_tag)

//============================================================================//

//============================================================================//
// Prototypes of functions
VOID BusPDO_StorageQueryProperty(IN WDFQUEUE , IN WDFREQUEST );
VOID BusPDO_GetDriveGeometry(IN WDFQUEUE , IN WDFREQUEST );
VOID BusPDO_GetAddress(IN WDFQUEUE , IN WDFREQUEST );
VOID BusPDO_InternalSCSI(IN WDFQUEUE , IN WDFREQUEST );
VOID BusPDO_ScsiRequest(IN WDFQUEUE , IN WDFREQUEST , IN PSCSI_REQUEST_BLOCK );
VOID BusPDO_SrbModeSense(IN WDFQUEUE , IN WDFREQUEST , IN PSCSI_REQUEST_BLOCK );
VOID BusPDO_SrbTestUnitReady(IN WDFQUEUE , IN WDFREQUEST , IN PSCSI_REQUEST_BLOCK );
VOID BusPDO_SrbRead(IN WDFQUEUE , IN WDFREQUEST , IN PSCSI_REQUEST_BLOCK );
VOID BusPDO_SrbReadCapacity(IN WDFQUEUE , IN WDFREQUEST , IN PSCSI_REQUEST_BLOCK );
VOID BusPDO_SrbWrite(IN WDFQUEUE , IN WDFREQUEST , IN PSCSI_REQUEST_BLOCK );
VOID BusPDO_SrbVerify(IN WDFQUEUE , IN WDFREQUEST , IN PSCSI_REQUEST_BLOCK );
VOID BusPDO_SrbMediumRemoval(IN WDFQUEUE , IN WDFREQUEST , IN PSCSI_REQUEST_BLOCK );
VOID BusPDO_SrbLoadUnload(IN WDFQUEUE , IN WDFREQUEST , IN PSCSI_REQUEST_BLOCK );
//============================================================================//

#endif // _PDOFUNC_H_

// End Of File
