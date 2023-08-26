1\.8.2023: Add this build instruction.

1) git clone --recurse-submodules  [git@github.com-FreeRTOS:notwendig/FreeRTOS-eZ80f91.git](mailto:git@github.com-FreeRTOS)
1) cd FreeRTOS-eZ80f91
1) cd kernel; patch -p1 < patches/0001-ZiLOG-Acclaim-fixes.patch

cd network; patch -p1 < patches/0001-ZiLOG-Acclaim-fixes.patch

cd uzlib; patch -p1 <  patches/0001-ZiLOG-Acclaim-fixes.patch

4) Run virtual windows-guest on your Linux-host. If you are using an USB-Debugge’s then redirect ther USB to the guest

Install the ZDSII v5.3.5 on your guest if not already done. And on the host connect a terminal (Putty) on the serial-port ( for example /dev/ttyS4 ) and setup the connection to 115200 8N1 RTS/CTS.

5) Now you can build the libraries.

Load the project uzlib,network and kernel one after the other into the ZDSII and compile them.

6) Connect the Zilog Debugger and the serial-port to your Build-PC
6) Load the application EZ80F910300ZCOG into the ZDSII and build and run it.
6) Open a Linux-Console (132x40) and enter “nc -u <the IP shown on putty> 4060”. Trigger the target by enter RETURN-Key.

FreeRTOS eZ80F91 Acclaim! Port - Copyright (C) 2016-2023 by NadiSoft All rights reserved

This file is part of the FreeRTOS port for ZiLOG's EZ80F91 Module. Copyright (C) 2016 by Juergen Sievers <[JSievers@NadiSoft.de>](mailto:JSievers@NadiSoft.de)

![](doc/Aspose.Words.645ddf8f-9ae5-40c3-ad4b-2cf353cd3778.001.png)

The Port was made and rudimentary tested on ZiLOG's EZ80F910300ZCOG Developer Kit using ZDSII Acclaim 5.3.5 Developer

Environment and comes WITHOUT ANY WARRANTY to you! Developer:

SIE Juergen Sievers <JSievers@NadiSoft.de> Repository directories:

- doc  some documentation.
- kernel  sub-module the real time kernel source code.
- network   sub-module FreeRTOS-Plus-TCP/source the real TCP/IP source code.
- EZ80F910300ZCOC The Demo’s header and source files.
- uzlib  sub-module The compress/un-compress library

.

See http://www.freertos.org.html for full details of the FreeRTOS directory structure and information on locating the files you require.

My Build- and Test-environment:

I use a Fedora Linux Workstation as host system. On this host a Windows 10 is running as guest under QEMU. On Windows I mount a samba share as drive Z: with at least the following directories.

1. **Z:\ZDSII\_eZ80Acclaim!\_5.3.5**

ZiLOG Developer Studio II—eZ80Acclaim!® installation.

Free download from [www.zilog.com.](https://www.zilog.com/)

2. [**Z:\workspace\FreeRTOS-eZ80f91](../../../../../../Z:/workspace) **Windows Project-root an SMB Mount from host ~/ZiLOG**

The Target is connected over its Serial and Ethernet port on the Linux-PC

The Debug-output uses target’s serial-port 0 115200,8,1,n,RTS/CTS. I use PuTTY on the Linux-PC to display such information.

On the Linux-PC also runs a DHCP-Server with will answer to the target’s DHCP Requests.![](doc/Aspose.Words.645ddf8f-9ae5-40c3-ad4b-2cf353cd3778.002.png)![](doc/Aspose.Words.645ddf8f-9ae5-40c3-ad4b-2cf353cd3778.003.png)

At last the ZiLOG USBSmartCable is connect between the target and Lunux-PC. This USB -Device is routed to the Windows guest. It will be used by ZiLOG’s Developer Studio for download, flashing and debugging.

Changing to directory Z:\workspace (or on the linux to the shared) and clone the project [https://github.com/notwendig/FreeRTOS-eZ80f91.git including submodules.](https://github.com/notwendig/FreeRTOS-eZ80f91.git)

Apply the patch to zulib.

On the Windows-guest run the ZiLOG Developer Studio. Open the Project-file Z:\workspace\FreeRTOS-eZ8091\uzib.zdsproj and build the uzlib[d].lib.

On a linux terminal change to directory uzlib and type make to build the packer/unpacker. Change to CPM directory and type make to build the CP/M 2.2 boot-tracks and the Disk-Image as packed C-Array.

Now you are ready to open and build the z80FreeROS.zdsproj, and build it.

![](doc/Aspose.Words.645ddf8f-9ae5-40c3-ad4b-2cf353cd3778.004.png)

Before you download and run the Demo to the Target, a terminal (PuTTY) should be running on the target's serial-port connection.

After the target has gotten it’s IP you may start several other connection to the target.

![](doc/Aspose.Words.645ddf8f-9ae5-40c3-ad4b-2cf353cd3778.005.png)

` `![](doc/Aspose.Words.645ddf8f-9ae5-40c3-ad4b-2cf353cd3778.006.png)![](doc/Aspose.Words.645ddf8f-9ae5-40c3-ad4b-2cf353cd3778.007.png)

Regards Jürgen
