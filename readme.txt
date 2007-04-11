1. Introduction

This device driver implements KMDF based virtual bus driver and generic virtual disk hosted on it. 
The virtual bus driver code is derived from Microsoft KMDF 1.0 "Toaster Bus" source sample. 
The result of this driver is functionality that allows user to add any (limited by OS resources) 
number virtual disks to Windows OS. Virtual disks are plugged-in (and out) by command line utility.

Virtual disks created by virtual bus driver, are recognized by Windows OS as generic disks which 
are supported by standard claspnp.sys and disk.sys. This means that they are seen in the very 
same way by the OS, as any other (physical) disks on the system (e.g. hosted on IDE bus). Such disks 
are identified by Windows Disk Manager and can be converted to MBR, GPT or Dynamic disks, partitioned, 
formatted with any supported file system (e.g. as NTFS) and supported by any service Windows OS is 
providing for generic disks (e.g. volume snapshots). 

Functionality this project provides is something similar to iSCSI, with the difference, that this project 
virtual disk (real) storage is open to be anywhere (e.g. RAM, file, network, …).

2. Building

Code is based and developed with Microsoft KMDF version 1.0 so you will need to download and install it. 
The current KMDF version is 1.5. I did not try with it (something todo), so all steps in this doc are 
related to KMDF 1.0.

You will have to install Windows 2003 SP1 DDK and KMDF. Check on this page where and how to find them http://www.microsoft.com/whdc/resources/downloads.mspx. 

Run do_vdbuild.cmd to build vdbus.sys driver and vdenum.exe console application.


3. Installing

Copy vdbus.sys, vdbus.inf, vdenum.exe and WdfCoInstaller01000.dll (from \WINDDK\WDF\KMDF10\redist\wdf\x86) 
to some directory (e.g. C:\vdbus).

Then run:
- to install: devcon.exe install c:\vdbus\vdbus.inf root\vdbus
- to update (if rebuild):  devcon.exe update c:\vdbus\vdbus.inf root\vdbus
- to remove:  devcon.exe remove root\vdbus

devcon.exe is standard DDK utility located in \WINDDK\3790.1830\tools\devcon\i386.

4. Debugging

WinDbg kernel debugging is something you will need for debugging. WinDbg is downloadable from http://www.microsoft.com/whdc/resources/downloads.mspx.

I also suggest subscription to OSR mailing lists. 
Go to http://www.osronline.com/ and follow ListServer links.

p.s.

This project can be a starting point for Windows kernel developers that are willing to learn something and 
enhance project functionality. 

If this is something that was helpful for you, than you can do something to help others too. 
You can check on internet with various charity organizations related to undeveloped countries children education. 
It's completely up to you, what you can do and what you are willing to do.

This was my motivation for starting this project.
Primoz Beltram (primoz.beltram@siol.net)
