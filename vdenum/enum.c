
//******************************************************************************

#include <basetyps.h>
#include <stdlib.h>
#include <wtypes.h>
#include <setupapi.h>
#include <initguid.h>
#include <stdio.h>
#include <string.h>
#include <winioctl.h>

#include <include\public.h>

//============================================================================//
#define USAGE  \
"Usage: vdenum [-p SerialNo] Plugs in a device. SerialNo must be greater than zero.\n\
               [-u SerialNo or 0] Unplugs device(s) - specify 0 to unplug all \n\
               the devices enumerated so far.\n"
//============================================================================//

//============================================================================//
BOOLEAN g_bPlugIn = FALSE;
BOOLEAN g_bUnplug = FALSE;
ULONG g_SerialNo = 0;
//============================================================================//

//============================================================================//
BOOLEAN
OpenBusInterface(IN HDEVINFO HardwareDeviceInfo, 
                 IN PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData)
{
    ULONG bytes;
    VDBUS_UNPLUG_HARDWARE unplug;
    PVDBUS_PLUGIN_HARDWARE hardware = NULL;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    PSP_DEVICE_INTERFACE_DETAIL_DATA deviceInterfaceDetailData = NULL;
    ULONG predictedLength = 0;
    ULONG requiredLength = 0;
    BOOLEAN bSuccess = FALSE;

    // Allocate a function class device data structure to receive the
    // information about this particular device.
    SetupDiGetDeviceInterfaceDetail(HardwareDeviceInfo, DeviceInterfaceData,
        NULL, 0, &requiredLength, NULL);
    if (ERROR_INSUFFICIENT_BUFFER != GetLastError()) 
    {
        printf("Error in SetupDiGetDeviceInterfaceDetail. dwErr=%d.\n", GetLastError());
        goto cleanup;
    };
     
    predictedLength = requiredLength;
    deviceInterfaceDetailData = malloc(predictedLength);
    if (deviceInterfaceDetailData) 
    {
        deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
    } 
    else 
    {
        printf("Couldn't allocate %d bytes for device interface details.\n", predictedLength);
        goto cleanup;
    };

    if (!SetupDiGetDeviceInterfaceDetail(HardwareDeviceInfo, DeviceInterfaceData, 
        deviceInterfaceDetailData, predictedLength, &requiredLength, NULL)) 
    {
        printf("Error in SetupDiGetDeviceInterfaceDetail. dwErr=%d.\n", GetLastError());
        goto cleanup;
    };

    printf("Opening %s\n", deviceInterfaceDetailData->DevicePath);

    hFile = CreateFile(deviceInterfaceDetailData->DevicePath, GENERIC_READ,
        0, NULL, OPEN_EXISTING, 0, NULL);
    if (INVALID_HANDLE_VALUE == hFile) 
    {
        printf("CreateFile failed: 0x%x", GetLastError());
        return bSuccess;
    };
    printf("Bus interface opened.\n");

    // Enumerate Devices
    if (g_bPlugIn) 
    {
        printf("SerialNo. of the device to be enumerated: %d.\n", g_SerialNo);
        bytes = sizeof(VDBUS_PLUGIN_HARDWARE)+tst_BUS_HARDWARE_IDS_LENGTH_d;
        hardware = malloc(bytes);
        if (hardware) 
        {
            hardware->Size = sizeof(VDBUS_PLUGIN_HARDWARE);
            hardware->SerialNo = g_SerialNo;
        } 
        else 
        {
            printf("Couldn't allocate %d bytes for vdbus plugin hardware structure.\n", bytes);
            goto cleanup;
        };

        // Allocate storage for the Device ID
        memcpy(hardware->HardwareIDs, tst_SZW_BUS_HARDWARE_IDS_d, tst_BUS_HARDWARE_IDS_LENGTH_d);
        if (!DeviceIoControl(hFile, tst_IOCTL_VDBUS_PLUGIN_HARDWARE_d, hardware, bytes,
            NULL, 0, &bytes, NULL)) 
        {
              printf("PlugIn failed. dwErr=0x%x.\n", GetLastError());
            goto cleanup;
        };
    };

    // Removes a device if given the specific Id of the device. Otherwise this
    // ioctls removes all the devices that are enumerated so far.
    if (g_bUnplug) 
    {
        printf("Unplugging device: %d.\n", g_SerialNo);

        unplug.Size = bytes = sizeof(unplug);
        unplug.SerialNo = g_SerialNo;
        if (!DeviceIoControl(hFile, tst_IOCTL_VDBUS_UNPLUG_HARDWARE_d, &unplug, bytes,
            NULL, 0, &bytes, NULL)) 
        {
            printf("Unplug failed: dwErr=%d.\n", GetLastError());
            goto cleanup;
        };
    };

    printf("Success.\n");
    bSuccess = TRUE;

cleanup:
    if (INVALID_HANDLE_VALUE != hFile)
    {
        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;
    };

    if (NULL != deviceInterfaceDetailData)
    {
        free(deviceInterfaceDetailData);
        deviceInterfaceDetailData = NULL;
    };

    if (NULL != hardware)
    {
        free(hardware);
        hardware = NULL;
    };
    return bSuccess;
};
//============================================================================//

//============================================================================//
int _cdecl main(int argc, char *argv[])
{
    HDEVINFO hardwareDeviceInfo;
    SP_DEVICE_INTERFACE_DATA deviceInterfaceData;

    if (argc < 3) 
    {
        goto usage;
    };

    if (argv[1][0] == '-') 
    {
        if (tolower(argv[1][1]) == 'p') 
        {
            if (argv[2])
            {
                g_SerialNo = (USHORT) atol(argv[2]);
            };
            g_bPlugIn = TRUE;
        }
        else if (tolower(argv[1][1]) == 'u') 
        {
            if(argv[2])
            {
                g_SerialNo = (ULONG) atol(argv[2]); 
            };
            g_bUnplug = TRUE;
        }
        else 
        {
            goto usage;
        };
    }
    else
    {
        goto usage;
    };

    if (g_bPlugIn && 0 == g_SerialNo)
    {
        goto usage;
    };

    hardwareDeviceInfo = SetupDiGetClassDevs((LPGUID) &tst_GUID_DEVINTERFACE_VDBUS_d,
        NULL, NULL, (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));
    if (INVALID_HANDLE_VALUE == hardwareDeviceInfo)
    {
        printf("SetupDiGetClassDevs failed: %x\n", GetLastError());
        return 0;
    };

    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    if (SetupDiEnumDeviceInterfaces(hardwareDeviceInfo, 0, (LPGUID) &tst_GUID_DEVINTERFACE_VDBUS_d,
        0, &deviceInterfaceData)) 
    {
        OpenBusInterface(hardwareDeviceInfo, &deviceInterfaceData);
    } 
    else if (ERROR_NO_MORE_ITEMS == GetLastError()) 
    {
    
        printf("Error:Interface GUID_DEVINTERFACE_VDBUS is not registered.\n");
    }
    else
    {
        printf("Error:%d.\n", GetLastError());
    };

    SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);
    return 0;

usage: 
    printf(USAGE);
    exit(0);
};
//============================================================================//

// End Of File


