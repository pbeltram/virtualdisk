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

Code is based and developed with Microsoft KMDF version 1.0. 
All tools needed to develop and build this driver, including KMDF, are available as free downloads from 
the internet.
<todo: building scripts>.

3. Installing

<todo: all>.

4. Debugging

WinDbg kernel debugging is something you will need for debugging. WinDbg is downloadable from <todo: put link here>.
I also suggest subscription to OSR mailing lists. <todo: links>.

p.s.

This project can be a starting point for Windows kernel developers that are willing to learn something and 
enhance project functionality. 

If this is something that was helpful for you, than you can do something to help others too. 
You can check on internet with various charity organizations related to undeveloped countries children education. 
It's completely up to you, what you can do and what you are willing to do.

This was my motivation for starting this project.
Primoz Beltram (primoz.beltram@siol.net)
