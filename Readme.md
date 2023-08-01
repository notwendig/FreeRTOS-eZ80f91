FreeRTOS eZ80F91 Acclaim\! Port \- Copyright \(C\) 2016\-2021 by NadiSoft All rights reserved



This file is part of the FreeRTOS port for ZiLOG's EZ80F91 Module.   
Copyright \(C\) 2016 by Juergen Sievers &lt;[JSievers@NadiSoft.de](mailto:JSievers@NadiSoft.de)&gt;

The Port was made and rudimentary tested on ZiLOG's EZ80F910300ZCOG Developer Kit using ZDSII Acclaim 5.3.4 Developer

Environment and comes WITHOUT ANY WARRANTY to you\!

Developer:

SIE Juergen Sievers &lt;JSievers@NadiSoft.de&gt;

Repository directories: 

* CPM CP/M 2.2 source and environment.
* doc some documantation.
* FreeRTOS the modified FreeRTOS real time kernel source code.
* FreeRTOSCLI the FreeRTOS\-Plus\-CLI command line interface source code.
* FreeRTOSTCP the modified FreeRTOS\-Plus\-TCP TCP/IP source code.
* Inc The Demo’s header files.
* monitor Z80 machine monitor. \(not usable yet\)
* src The Demo’s source files.

. 

See http://www.freertos.org.html for full details of the FreeRTOS directory structure and information on locating the files you require. 

My Build\- and Test\-environment:

I use a Fedora Linux Workstation as host system for a Windows 10 as guest under QEMU. On Windows I mount a samba share as drive Z: with at least the following directories.

1. **Z:\\ZDSII\_eZ80Acclaim\!\_5.3.4**  
ZiLOG Developer Studio II—eZ80Acclaim\!® installation.  
Free download from [www.zilog.com](https://www.zilog.com/).
2. [ **Z:\\workspace**](../../../../../../../Z:/workspace)  
 

The Target is connected over a its Serial and Ethernet port on the Linux\-PC

The Debug\-output uses serial\-port 0 115200,8,1,n,RTS/CTS. I use PuTTY on the Linux\-PC to display such information. 

On the Linux\-PC also runs a DHCP\-Server with will answer to the Targets DHCP Requests.

At last the ZiLOG USBSmartCable is connect between the target and Lunux\-PC. This USB \-Device is routed to the Windows guest. It will be used by the Developer Studio for download, flashing and debugging.

Changing to directory Z:\\workspace and clone the project [https://github.com/notwendig/FreeRTOS\-eZ80f91.git](https://github.com/notwendig/FreeRTOS\-eZ80f91.git)

On the Windows\-guest run the ZiLOG Developer Studio. Open the Project\-file  
Z:\\workspace\\FreeRTOS\-eZ8091\\ez80FreeROS.zdsproj, and build it.





Before you download and run the Demo on the Target a terminal \(PuTTY\) should be running on the targets serial\-port connection.

After the target has gotten the IP you may start several other connection to the target.







Regards

Jürgen

