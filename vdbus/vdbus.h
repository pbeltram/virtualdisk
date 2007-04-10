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

#ifndef _VDBUS_H_
#define _VDBUS_H_

#include <ntddk.h>
#include <wdf.h>
#define NTSTRSAFE_LIB
#include <ntstrsafe.h>
#include <ntintsafe.h>
#include <initguid.h>
#include <devguid.h>
#include <scsi.h>
#include <srb.h>
#include <ntddscsi.h>
#include <ntdddisk.h>

#include <include\public.h>

// Common file version values
#include <include\product.h>

//============================================================================//
#define tst_WEOS_d L'\0'
#define tst_EOS_d '\0'

#define tst_ACCESS_FROM_CTL_CODE_m(ctrlCode)   (((ULONG)(ctrlCode & 0x0000C000)) >> 14)
#define tst_METHOD_FROM_CTL_CODE_m(ctrlCode)   ((ULONG)(ctrlCode & 0x00000003))
#define tst_DEVICE_FROM_CTL_CODE_m(ctrlCode)   (((ULONG)(ctrlCode & 0xffff0000)) >> 16)
#define tst_FUNCTION_FROM_CTL_CODE_m(ctrlCode) (((ULONG)(ctrlCode & 0x00003FFC)) >> 2)
//============================================================================//

//============================================================================//
// 0x0000000F
#define tst_DBG_MAIN_d               0x00000001 // main filter execution flow.
//#define tst_DBG_SOME_DETAIL_d      0x00000002 // Not used.
//#define tst_DBG_SOME_DETAIL_d      0x00000004 // Not used.
//#define tst_DBG_SOME_DETAIL_d      0x00000008 // Not used.

// 0x000000F0
#define tst_DBG_SRB_d                0x00000010 // Dump SRB members.

// 0x00000F00

// 0x0000F000
#define tst_DBG_ERR_d                0x00001000 // Debug output Errors.
#define tst_DBG_WARN_d               0x00002000 // Debug output Warnings.

// 0x000F0000

// 0x00F00000

// 0x0F000000
#define tst_DBG_LOGERR_d             0x01000000 // EventLog Errors.
#define tst_DBG_LOGWARN_d            0x02000000 // EventLog Warnings.
#define tst_DBG_LOGINFO_d            0x04000000 // EventLog Info.

// 0xF0000000
#define tst_DBG_FUNCINOUT_d          0x80000000 // tracing of all function in/out.
//============================================================================//

//============================================================================//
#define tst_XY_ERRMSG_d     ((NTSTATUS)0xE1BD0001L) // Temporary. Until message catalog defines.
#define tst_XY_WRNMSG_d     ((NTSTATUS)0xA1BD0001L) // Temporary. Until message catalog defines.
#define tst_XY_LOGMSG_d     ((NTSTATUS)0x61BD0001L) // Temporary. Until message catalog defines.

// ERROR/WARNING/INFO identification:
// xyyfllll
// x = Type of message (0xC=error) yy = Facilty (0xBD) f = File ID, llll = Line number.
#define tst_MESSAGEID_m(FILEID, LNID)  (FILEID | LNID)

#define tst_ERR_PARAMS_d __LINE__, tst_BUG_CHECK_FILEID_d, '|'
#define tst_ERR_m(Args) if (FlagOn(g_ulDebug,(tst_DBG_ERR_d | tst_DBG_LOGERR_d))) { BusDumpStrings Args; }

#define tst_WARN_PARAMS_d __LINE__, tst_BUG_CHECK_FILEID_d, '|'
#define tst_WARN_m(Args) if (FlagOn(g_ulDebug,(tst_DBG_WARN_d | tst_DBG_LOGWARN_d))) { BusDumpStrings Args; }

#define tst_INFO_PARAMS_d __LINE__, tst_BUG_CHECK_FILEID_d, '|'
#define tst_INFO_m(Args) if (FlagOn(g_ulDebug,tst_DBG_LOGINFO_d)) { BusDumpStrings Args; }

#define tst_LOG_PARAMS_d __LINE__, tst_BUG_CHECK_FILEID_d, '|'
#define tst_LOG_m(Args) BusDumpStrings Args 

#define tst_TRC_PARAMS_d __LINE__, tst_BUG_CHECK_FILEID_d
#define tst_TRC_m(Type, Args) if (FlagOn(g_ulDebug,Type)) { BusDumpTrace Args; }

#define tst_FUNCIN_m() { InterlockedIncrement(&g_ulFncInOut); tst_TRC_m(tst_DBG_FUNCINOUT_d, (tst_TRC_PARAMS_d,  "ENTER(%d) " __FUNCTION__, g_ulFncInOut)); };
#define tst_FUNCOUT_m() { InterlockedDecrement(&g_ulFncInOut); tst_TRC_m(tst_DBG_FUNCINOUT_d, (tst_TRC_PARAMS_d, "EXIT(%d) " __FUNCTION__, g_ulFncInOut)); };
//============================================================================//

//============================================================================//
#if _DEBUG       
#define tst_ASSERT_m(_expr, LN, A, B, C) ASSERT(_expr)
#else            
#define tst_ASSERT_m(_expr, LN, A, B, C)  if (!(_expr)) { tst_BugCheck_m(LN, A, B, C); } 
#endif //_DEBUG                         

#define tst_INP_ASSERT_m(_expr, LN, A, B, C) tst_ASSERT_m(_expr, LN, A, B, C)
#define tst_ERR_ASSERT_m(_expr, LN, A, B, C) tst_ASSERT_m(_expr, LN, A, B, C)
#define tst_MEM_ASSERT_m(_expr, LN, A, B, C) tst_ASSERT_m(_expr, LN, A, B, C)
#define tst_MUST_ASSERT_m(_expr, LN, A, B, C) tst_ASSERT_m(_expr, LN, A, B, C)

#define tst_BUG_CHECK_FILEID_MAIN_d              (0x00010000)
#define tst_BUG_CHECK_FILEID_PDO_d               (0x00020000)
#define tst_BUG_CHECK_FILEID_WMI_d               (0x00030000)
#define tst_BUG_CHECK_FILEID_PDOFUNC_d           (0x00040000)

#define tst_BUGCHECK_d 0xDEAD0000 // *!Vade retro Satana!*
#define tst_BugCheck_m(LN,A,B,C) { KeBugCheckEx(tst_BUGCHECK_d, tst_BUG_CHECK_FILEID_d | LN, (ULONG_PTR) (A), (ULONG_PTR) (B), (ULONG_PTR) (C)); }

#define tst_TRY_m(status)  status = STATUS_SUCCESS; __try
#define tst_CATCH_m(status) \
  __except(BusExceptionFilter(__FILE__, __LINE__, \
             tst_BUG_CHECK_FILEID_d, GetExceptionInformation())) \
  {                         \
    status = GetExceptionCode(); \
    tst_ASSERT_m(FALSE, __LINE__, status, 0, 0);          \
  };                        
//============================================================================//

//============================================================================//
typedef enum _tst_MAGIC_tag 
{
    tst_GEN_PAGEDP_BUFF_MAGIC_e   = '0suB', 
    tst_GEN_NPAGEDP_BUFF_MAGIC_e  = '1suB',
} tst_MAGIC_t;

#define tst_WSZ_BUSRESOURCENAME_d L"MofResourceName"

// {4C65A16B-5884-43fd-A30E-6CFA14BC2037}
DEFINE_GUID(GUID_DEVCLASS_VDBUS, 
    0x4c65a16b, 0x5884, 0x43fd, 0xa3, 0xe, 0x6c, 0xfa, 0x14, 0xbc, 0x20, 0x37);

// {BF905556-4082-4050-932C-D7AC29C27C88}
DEFINE_GUID(VDBUS_WMI_STD_DATA_GUID, 
    0xbf905556, 0x4082, 0x4050, 0x93, 0x2c, 0xd7, 0xac, 0x29, 0xc2, 0x7c, 0x88);

//============================================================================//

//============================================================================//
#ifndef tst_Min_m
#define tst_Min_m(_a, _b) (((_a) < (_b)) ? (_a) : (_b))
#endif

#ifndef tst_Max_m
#define tst_Max_m(_a, _b) (((_a) > (_b)) ? (_a) : (_b))
#endif

#ifndef FlagOn
#define FlagOn(_F,_SF) ((_F) & (_SF))
#endif

#ifndef SetFlag
#define SetFlag(_F,_SF) ((_F) |= (_SF))
#endif

#ifndef ClearFlag
#define ClearFlag(_F,_SF) ((_F) &= ~(_SF))
#endif

#define tst_LOBYTE_m(word) ((unsigned char) (word & 0xFF))
#define tst_HIBYTE_m(word) ((unsigned char) tst_LOBYTE_m(word >> 8))
//============================================================================//

//============================================================================//
// Structure for reporting data to WMI
typedef struct _VDBUS_WMI_STD_DATA 
{
    UINT32  ErrorCount; // The error Count
    UINT32  DebugPrintLevel; // Debug Print Level

} VDBUS_WMI_STD_DATA, * PVDBUS_WMI_STD_DATA;
//============================================================================//

//============================================================================//
typedef struct _PDO_IDENTIFICATION_DESCRIPTION
{
    WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER Header; // should contain this header

    // Unique serail number of the device on the bus
    ULONG SerialNo;
    size_t CchHardwareIds;
    PWCHAR HardwareIds;

} PDO_IDENTIFICATION_DESCRIPTION, *PPDO_IDENTIFICATION_DESCRIPTION;
//============================================================================//

//============================================================================//
#define tst_BUSNUMBER_d  0
//============================================================================//

//============================================================================//
// The device extension of the bus itself.  From whence the PDO's are born.
typedef struct _FDO_DEVICE_DATA
{
    VDBUS_WMI_STD_DATA StdVDBusData;

    UCHAR ucBusNumber;
    STORAGE_ADAPTER_DESCRIPTOR sBusDesc;

} FDO_DEVICE_DATA, *PFDO_DEVICE_DATA;
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FDO_DEVICE_DATA, BusGetFDODevExtData)
//============================================================================//

//============================================================================//
// Prototypes of functions
DRIVER_INITIALIZE DriverEntry;
NTSTATUS Bus_EvtDriverDeviceAdd(IN WDFDRIVER , IN PWDFDEVICE_INIT );
NTSTATUS Bus_EvtDriverUnload(IN WDFDRIVER );

NTSTATUS Bus_EvtChildListCreatePDO(IN WDFCHILDLIST , IN PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER , IN PWDFDEVICE_INIT );
NTSTATUS Bus_EvtChildListDuplicate(IN WDFCHILDLIST , IN PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER , IN PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER );
BOOLEAN Bus_EvtChildListCompare(IN WDFCHILDLIST , IN PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER , IN PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER );
VOID Bus_EvtChildListCleanup(IN WDFCHILDLIST , IN PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER );

VOID BusFDO_EvtIoDeviceControl(IN WDFQUEUE , IN WDFREQUEST , IN size_t , IN size_t , IN ULONG );
NTSTATUS Bus_IoctlPlugInDevice(IN WDFDEVICE , IN PWCHAR , IN size_t , IN ULONG );
NTSTATUS Bus_IoctlUnPlugDevice(IN WDFDEVICE ,  IN ULONG );
NTSTATUS Bus_IoctlEjectDevice(IN WDFDEVICE , IN ULONG );
//============================================================================//

//============================================================================//
NTSTATUS Bus_CreatePDO(IN WDFDEVICE , IN PWDFDEVICE_INIT ,IN PWCHAR , IN ULONG );
VOID BusPDO_EvtIoDeviceControl(IN WDFQUEUE , IN WDFREQUEST , IN size_t , IN size_t , IN ULONG );
VOID BusPDO_EvtIoDefault(IN WDFQUEUE , IN WDFREQUEST );
VOID BusPDO_EvtIoRead(IN WDFQUEUE , IN WDFREQUEST , IN size_t );
VOID BusPDO_EvtIoWrite(IN WDFQUEUE , IN WDFREQUEST , IN size_t );
VOID BusPDO_EvtIoStop(IN WDFQUEUE , IN WDFREQUEST , IN ULONG );
VOID BusPDO_EvtIoResume(IN WDFQUEUE , IN WDFREQUEST );
VOID BusPDO_EvtIoCanceledOnQueue(IN WDFQUEUE , IN WDFREQUEST );
//============================================================================//

//============================================================================//
// Prototypes of functions
NTSTATUS Bus_WmiRegistration(IN WDFDEVICE );
NTSTATUS Bus_EvtStdDataSetItem(IN WDFWMIINSTANCE , IN ULONG , IN ULONG , IN PVOID );
NTSTATUS Bus_EvtStdDataSetInstance(IN WDFWMIINSTANCE , IN ULONG , IN PVOID );
NTSTATUS Bus_EvtStdDataQueryInstance(IN WDFWMIINSTANCE , IN ULONG , IN PVOID , OUT PULONG );
//============================================================================//

//============================================================================//
NTSTATUS BusDumpTrace(IN ULONG , IN ULONG , IN PCHAR , ... );
LONG BusExceptionFilter(IN PCHAR , IN ULONG , IN ULONG , IN PEXCEPTION_POINTERS );
NTSTATUS BusWriteLogMessage(IN ULONG , IN ULONG , IN PCHAR , IN ULONG , IN USHORT );
NTSTATUS BusDumpStrings(IN ULONG , IN ULONG , IN CHAR , IN ULONG , IN PCHAR , ... );
// From ntifs.h
NTSTATUS RtlMultiByteToUnicodeN(OUT PWSTR , IN ULONG , OUT PULONG OPTIONAL, IN PCHAR , IN ULONG ); 
//============================================================================//

#include "pdofunc.h"

//============================================================================//
extern ULONG g_ulDebug;
extern ULONG g_ulFncInOut;
//============================================================================//

#endif // _VDBUS_H_

// End Of File