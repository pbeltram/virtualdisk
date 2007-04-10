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

#ifndef _PUBLIC_H_
#define _PUBLIC_H_

//============================================================================//
// Define an Interface Guid for bus enumerator class.
// This GUID is used to register (IoRegisterDeviceInterface)
// an instance of an interface so that enumerator application
// can send an ioctl to the bus driver.

// {D43EED52-993F-494b-9383-1012218F21E4}
DEFINE_GUID(tst_GUID_DEVINTERFACE_VDBUS_d, 
    0xd43eed52, 0x993f, 0x494b, 0x93, 0x83, 0x10, 0x12, 0x21, 0x8f, 0x21, 0xe4);

//============================================================================//

//============================================================================//
// GUID definition are required to be outside of header inclusion pragma to avoid
// error during precompiled headers.
#define tst_SZW_BUS_HARDWARE_IDS_d L"VDBUS\\Disk\0"
#define tst_BUS_HARDWARE_IDS_LENGTH_d sizeof(tst_SZW_BUS_HARDWARE_IDS_d)
//============================================================================//

//============================================================================//
#define tst_VDBUS_IOCTL_m(_index_) CTL_CODE(FILE_DEVICE_BUS_EXTENDER, _index_, METHOD_BUFFERED, FILE_READ_DATA)

#define tst_IOCTL_VDBUS_PLUGIN_HARDWARE_d tst_VDBUS_IOCTL_m(0x0)
#define tst_IOCTL_VDBUS_UNPLUG_HARDWARE_d tst_VDBUS_IOCTL_m(0x1)
#define tst_IOCTL_VDBUS_EJECT_HARDWARE_d  tst_VDBUS_IOCTL_m(0x2)
//============================================================================//

//============================================================================//
typedef struct _VDBUS_PLUGIN_HARDWARE
{
    // sizeof (struct _VDBUS_PLUGIN_HARDWARE)
    IN ULONG Size;

    // Unique serial number of the device to be enumerated.
    IN ULONG SerialNo;
    // An array of (zero terminated wide character strings).
    // The array itself also null terminated (ie, MULTI_SZ)
    #pragma warning(disable:4200)  // nonstandard extension used
    IN WCHAR HardwareIDs[];
    #pragma warning(default:4200)

} VDBUS_PLUGIN_HARDWARE, *PVDBUS_PLUGIN_HARDWARE;
//============================================================================//

//============================================================================//
typedef struct _VDBUS_UNPLUG_HARDWARE
{
    // sizeof (struct _VDBUS_REMOVE_HARDWARE)
    IN ULONG Size;

    // Serial number of the device to be plugged out
    ULONG SerialNo;
    ULONG Reserved[2];

} VDBUS_UNPLUG_HARDWARE, *PVDBUS_UNPLUG_HARDWARE;
//============================================================================//

//============================================================================//
typedef struct _VDBUS_EJECT_HARDWARE
{
    // sizeof (struct _VDBUS_EJECT_HARDWARE)
    IN ULONG Size;
    // Serial number of the device to be ejected
    ULONG SerialNo;
    ULONG Reserved[2];

} VDBUS_EJECT_HARDWARE, *PVDBUS_EJECT_HARDWARE;
//============================================================================//

#endif // _VDBUS_H_

// End Of File