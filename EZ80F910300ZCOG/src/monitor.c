/*
    FreeRTOS - Copyright (C) 2016 Real Time Engineers Ltd.
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>>> AND MODIFIED BY <<<< the FreeRTOS exception.

    ***************************************************************************
    >>!   NOTE: The modification to the GPL is included to allow you to     !<<
    >>!   distribute a combined work that includes FreeRTOS without being   !<<
    >>!   obliged to provide the source code for proprietary components     !<<
    >>!   outside of the FreeRTOS kernel.                                   !<<
    ***************************************************************************

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available on the following
    link: http://www.freertos.org/a00114.html

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that is more than just the market leader, it     *
     *    is the industry's de facto standard.                               *
     *                                                                       *
     *    Help yourself get started quickly while simultaneously helping     *
     *    to support the FreeRTOS project by purchasing a FreeRTOS           *
     *    tutorial book, reference manual, or both:                          *
     *    http://www.FreeRTOS.org/Documentation                              *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org/FAQHelp.html - Having a problem?  Start by reading
    the FAQ page "My application does not run, what could be wrong?".  Have you
    defined configASSERT()?

    http://www.FreeRTOS.org/support - In return for receiving this top quality
    embedded software for free we request you assist our global community by
    participating in the support forum.

    http://www.FreeRTOS.org/training - Investing in training allows your team to
    be as productive as possible as early as possible.  Now you can receive
    FreeRTOS training directly from Richard Barry, CEO of Real Time Engineers
    Ltd, and the world's leading authority on the world's leading RTOS.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.FreeRTOS.org/labs - Where new FreeRTOS products go to incubate.
    Come and try FreeRTOS+TCP, our new open source TCP/IP stack for FreeRTOS.

    http://www.OpenRTOS.com - Real Time Engineers ltd. license FreeRTOS to High
    Integrity Systems ltd. to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and commercial middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/


/*
	FreeRTOS eZ80F91 Acclaim! Port - Copyright (C) 2016 by NadiSoft
    All rights reserved

	This file is part of the FreeRTOS port for ZiLOG's EZ80F91 Module.
    Copyright (C) 2016 by Juergen Sievers <JSievers@NadiSoft.de>
	The Port was made and rudimentary tested on ZiLOG's
	EZ80F910300ZCOG Developer Kit using ZDSII Acclaim 5.2.1 Developer
	Environmen and comes WITHOUT ANY WARRANTY to you!


	File: 	monitor.c
			plattform and module system information.
			Connect ansi-terminal to comport0 (115200,8,1,n) to see what's happen.
			Part of FreeRTOS Port for the eZ80F91 Development Kit eZ80F910300ZCOG
			See www.zilog.com for desciption.


	Developer:
	JSIE	 Juergen Sievers <JSievers@NadiSoft.de>

	150804:	JSIE Start this port.

*/

/* FreeRTOS includes. */
#include "stdint.h"
#include "FreeRTOS.h"
#include "task.h"

/* FreeRTOS+TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
//#include "FreeRTOS_CLI.h"
#include "console.h"
#include "ez80_rtc.h"
#include "ez80_leds.h"
#include "ez80_buttons.h"
#define F91_EMAC_CONF_t void*
#include "AMD79C874_phy.h"
#include "NetworkInterface.h"
#include "ez80_emac.h"
#include "eZ80F91.h"
#include "monitor.h"
#include <CTYPE.H>
#include <String.h>


#define MKQUOTE(x)	#x
#define MKVERSION(x)	MKQUOTE(x)

#define DEVKIT 		"EZ80F910300ZCOG"
#define ZIDE		"ZDS II Acclaim! 5.3.5 Build:" MKVERSION(__ZDATE__)
#define AUTOR		"Juergen Sievers"
#define AUTORMAIL	"JSievers@NadiSoft.de"
#define VERSION		MKVERSION(DEMOVERSION)

static int alarmflg = 0;
static unsigned alarms = 0;

static Socket_t xSocketTrace = (Socket_t)-1;
static struct freertos_sockaddr xClient;
static int clientok;

static int sockPrintf(const char *fmt, ...)
{
	uint8_t *pucUDPPayloadBuffer = ( uint8_t * ) FreeRTOS_GetUDPPayloadBuffer( 1000, 100);
	int res = -1;
	va_list argp;
	va_start( argp, fmt);
	if( pucUDPPayloadBuffer != NULL )
	{
		int32_t iores;
		res = vsnprintf((char*)pucUDPPayloadBuffer, 1000, fmt, argp);
        if(res < 0)
        {
            FreeRTOS_ReleaseUDPPayloadBuffer(pucUDPPayloadBuffer);
            va_end(argp);
            return res;
        }
        if(res >= 1000)
            res = 999;
		iores = FreeRTOS_sendto( xSocketTrace, pucUDPPayloadBuffer, res, FREERTOS_ZERO_COPY, &xClient, sizeof(xClient));
		if(iores <= 0)
		{
			clientok =
			res = 0;
			FreeRTOS_ReleaseUDPPayloadBuffer( ( void * ) pucUDPPayloadBuffer );
		}
	}
	va_end(argp);
	return res;
}
static void /*nested_interrupt*/ rtc_alarm(void);
static void setrelalarm(uint8_t sec)
{
	rtc_t p;
	int s,m,h,d;

	p.flg = (1 << RID_DOW) |(1 << RID_HRS) | (1 << RID_MIN) | (1 << RID_SEC);
	getRTC(&p);
	s = p.d.r.SEC;
	m = p.d.r.MIN;
	h = p.d.r.HRS;
	d = p.d.r.DOW;

	s += sec;
	m += s / 60; s %= 60;
	h += m / 60; m %= 60;
	d += h / 24; h %= 24;
	if (d > 7)
		d = 1;
	setAlarm(rtc_alarm, ASEC_EN | AMIN_EN | AHRS_EN | ADOW_EN, d,h,m,s);
}

static void /*nested_interrupt*/ rtc_alarm(void)
{
	char dummy = RTC_CTRL;

	alarmflg = 5;
	alarms++;
	setrelalarm(33);
}

/* Two 37-column panels leave a clear gutter on an 80x40 display. */
void sys_heapinfo(void)
{
    HeapStats_t stats;
    vPortGetHeapStats(&stats);
    sockPrintf(ANSI_SCUR ANSI_COFF);
    sockPrintf(ANSI_GXY(43,12) ANSI_SATT(0,34,43) "%-37.37s", " Heap");
    sockPrintf(ANSI_GXY(43,13) ANSI_SATT(0,36,40) "Free total     : " ANSI_SATT(0,32,40) "%8u", stats.xAvailableHeapSpaceInBytes);
    sockPrintf(ANSI_GXY(43,14) ANSI_SATT(0,36,40) "Largest block  : " ANSI_SATT(0,32,40) "%8u", stats.xSizeOfLargestFreeBlockInBytes);
    sockPrintf(ANSI_GXY(43,15) ANSI_SATT(0,36,40) "Smallest block : " ANSI_SATT(0,32,40) "%8u", stats.xSizeOfSmallestFreeBlockInBytes);
    sockPrintf(ANSI_GXY(43,16) ANSI_SATT(0,36,40) "Free blocks    : " ANSI_SATT(0,32,40) "%8u", stats.xNumberOfFreeBlocks);
    sockPrintf(ANSI_GXY(43,17) ANSI_SATT(0,36,40) "Alloc calls    : " ANSI_SATT(0,32,40) "%8u", stats.xNumberOfSuccessfulAllocations);
    sockPrintf(ANSI_GXY(43,18) ANSI_SATT(0,36,40) "Free calls     : " ANSI_SATT(0,32,40) "%8u", stats.xNumberOfSuccessfulFrees);
    sockPrintf(ANSI_GXY(43,19) ANSI_SATT(0,36,40) "Minimum free   : " ANSI_SATT(0,32,40) "%8u", stats.xMinimumEverFreeBytesRemaining);
    sockPrintf(ANSI_RCUR ANSI_CON);
}

void sys_netinfo(void)
{
    uint32_t ip = 0, mask = 0, gateway = 0, dns = 0;
    uint16_t phyData = 0;
    const char *att = ANSI_SATT(0,31,40);
    char mode[32] = "DOWN";
    const emacStat_t *stats = emac_stat();
    if(xIsEthernetConnected() == pdTRUE)
    {
        att = ANSI_SATT(0,32,40);
        usPHY_ReadReg(PHY_DIAG_REG, &phyData);
        snprintf(mode, sizeof(mode), "%s Mbps, %s-Duplex",
                  phyData & PHY_100_MBPS ? "100" : "10",
                  phyData & PHY_FULL_DUPLEX ? "Full" : "Half");
        FreeRTOS_GetAddressConfiguration(&ip, &mask, &gateway, &dns);
    }
    sockPrintf(ANSI_SCUR ANSI_COFF);
    sockPrintf(ANSI_GXY(2,4) ANSI_SATT(0,34,43) "%-37.37s", " Ethernet");
    sockPrintf(ANSI_GXY(2,5) ANSI_SATT(0,36,40) "Link   : %s%-28.28s", att, mode);
    sockPrintf(ANSI_GXY(2,6) ANSI_SATT(0,36,40) "IP     : %s%-28lxip", att, FreeRTOS_htonl(ip));
    sockPrintf(ANSI_GXY(2,7) ANSI_SATT(0,36,40) "Mask   : %s%-28lxip", att, FreeRTOS_htonl(mask));
    sockPrintf(ANSI_GXY(2,8) ANSI_SATT(0,36,40) "Gateway: %s%-28lxip", att, FreeRTOS_htonl(gateway));
    sockPrintf(ANSI_GXY(2,9) ANSI_SATT(0,36,40) "DNS    : %s%-28lxip", att, FreeRTOS_htonl(dns));
    sockPrintf(ANSI_GXY(2,12) ANSI_SATT(0,34,43) "%-37.37s", " EMAC counters");
    sockPrintf(ANSI_GXY(2,13) ANSI_SATT(0,36,40) "txsz    " "%s%8u", att, stats->txsz);
    sockPrintf(ANSI_GXY(22,13) ANSI_SATT(0,36,40) "rxsz    " "%s%8u", att, stats->rxsz);
    sockPrintf(ANSI_GXY(2,14) ANSI_SATT(0,36,40) "txdone  " "%s%8u", att, stats->txdone);
    sockPrintf(ANSI_GXY(22,14) ANSI_SATT(0,36,40) "rxdone  " "%s%8u", att, stats->rxdone);
    sockPrintf(ANSI_GXY(2,15) ANSI_SATT(0,36,40) "txover  " "%s%8u", att, stats->txover);
    sockPrintf(ANSI_GXY(22,15) ANSI_SATT(0,36,40) "rxover  " "%s%8u", att, stats->rxover);
    sockPrintf(ANSI_GXY(2,16) ANSI_SATT(0,36,40) "txpcf   " "%s%8u", att, stats->txpcf);
    sockPrintf(ANSI_GXY(22,16) ANSI_SATT(0,36,40) "rxpcf   " "%s%8u", att, stats->rxpcf);
    sockPrintf(ANSI_GXY(2,17) ANSI_SATT(0,36,40) "txabort " "%s%8u", att, stats->txabort);
    sockPrintf(ANSI_GXY(22,17) ANSI_SATT(0,36,40) "rxnotok " "%s%8u", att, stats->rxnotok);
    sockPrintf(ANSI_GXY(2,18) ANSI_SATT(0,36,40) "txfsmer " "%s%8u", att, stats->txfsmerr);
    sockPrintf(ANSI_GXY(22,18) ANSI_SATT(0,36,40) "mgdone  " "%s%8u", att, stats->mgdone);
    sockPrintf(ANSI_GXY(2,19) ANSI_SATT(0,36,40) "rxnospc " "%s%8u", att, stats->rxnospace);
    sockPrintf(ANSI_GXY(22,19) ANSI_SATT(0,36,40) "rxinvsz " "%s%8u", att, stats->rxinvsize);
    sockPrintf(ANSI_GXY(2,20) ANSI_SATT(0,36,40) "rxcrcer " "%s%8u", att, stats->rxcrcerr);
    sockPrintf(ANSI_GXY(22,20) ANSI_SATT(0,36,40) "rxalign " "%s%8u", att, stats->rxalignerr);
    sockPrintf(ANSI_GXY(2,21) ANSI_SATT(0,36,40) "rxlongs " "%s%8u", att, stats->rxlongevent);
    sockPrintf(ANSI_GXY(22,21) ANSI_SATT(0,36,40) "rxok    " "%s%8u", att, stats->rxok);
    sockPrintf(ANSI_GXY(2,22) ANSI_SATT(0,36,40) "rxcf    " "%s%8u", att, stats->rxcf);
    sockPrintf(ANSI_RCUR ANSI_CON);
}

void sys_rtcinfo(void)
{
    char clock[40];
    const char *att = chkRTCPowerLost() ? ANSI_SATT(1,31,40) : ANSI_SATT(0,32,40);
    sockPrintf(ANSI_SCUR ANSI_COFF);
    sockPrintf(ANSI_GXY(43,4) ANSI_SATT(0,34,43) "%-37.37s", " Real Time Clock");
    sockPrintf(ANSI_GXY(43,5) ANSI_SATT(0,36,40) "Date : %s%-30.30s", att, getsDate(clock,sizeof(clock)));
    sockPrintf(ANSI_GXY(43,6) ANSI_SATT(0,36,40) "Time : %s%-30.30s", att, getsTime(clock,sizeof(clock)));
    sockPrintf(ANSI_GXY(43,7) ANSI_SATT(0,36,40) "Alarm: %s%-30.30s", att, getsAlarm(clock,sizeof(clock)));
    if(alarmflg)
        --alarmflg;
    sockPrintf(ANSI_GXY(43,9) "%sAlarm count: %8u       ",
               alarmflg ? ANSI_SATT(7,31,40) : ANSI_SATT(0,31,40), alarms);
    sockPrintf(ANSI_RCUR ANSI_CON);
}

void sys_buttoninfo(void)
{
    static const char *const labels[3] = {"Left", "Middle", "Right"};
    static const char *const states[3] = {"?", "Open", "Close"};
    static TickType_t previous[3];
    int i;
    sockPrintf(ANSI_SCUR ANSI_COFF);
    sockPrintf(ANSI_GXY(2,27) ANSI_SATT(0,34,43) "%-78.78s", " Platform buttons");
    sockPrintf(ANSI_GXY(2,28) ANSI_SATT(0,36,40)
               "%-8s %-6s %8s %16s %16s", "Button", "State", "Changes", "Previous tick", "Last tick");
    for(i = BUTTON_LEFT; i <= BUTTON_RIGTH; ++i)
    {
        const button_t *button = get_button(i);
        const char *att = previous[i] != button->changes ? ANSI_SATT(1,31,40) : ANSI_SATT(0,32,40);
        int state = button->state + 1;
        if(state < 0 || state > 2)
            state = 0;
        sockPrintf("\x1b[%u;2f" ANSI_SATT(0,36,40) "%-8s %s%-6s %8u %16u %16u",
                   (unsigned)(29+i), labels[i], att, states[state],
                   button->changes, button->privT, button->lastT);
        previous[i] = button->changes;
    }
    sockPrintf(ANSI_RCUR ANSI_CON);
}

static Socket_t prvOpenUDPServerSocket( uint16_t usPort )
{
struct freertos_sockaddr xServer;
Socket_t xSocket = FREERTOS_INVALID_SOCKET;

	xSocket = FreeRTOS_socket( FREERTOS_AF_INET, FREERTOS_SOCK_DGRAM, FREERTOS_IPPROTO_UDP );
	if( xSocket != FREERTOS_INVALID_SOCKET)
	{
		/* Zero out the server structure. */
		memset( ( void * ) &xServer, 0x00, sizeof( xServer ) );
		/* Set family and port. */
		xServer.sin_family = FREERTOS_AF_INET;
		xServer.sin_port = FreeRTOS_htons( usPort );
		/* Bind the address to the socket. */
		if( FreeRTOS_bind( xSocket, &xServer, sizeof( xServer ) ) != 0 )
		{
			FreeRTOS_closesocket( xSocket );
			xSocket = FREERTOS_INVALID_SOCKET;
		}
	}

	return xSocket;
}

static void monitorStart(void)
{
    char title[96];
    snprintf(title, sizeof(title), " FreeRTOS %s | %s | Monitor 80x40",
              tskKERNEL_VERSION_NUMBER, DEVKIT);
    sockPrintf(ANSI_NORM ANSI_CLRS ANSI_COFF ANSI_GXY(1,1)
               ANSI_SATT(0,34,47) "%-80.80s", title);
    sockPrintf(ANSI_GXY(1,2) ANSI_SATT(0,34,47) "%-80.80s",
               " Juergen Sievers | www.NadiSoft.de | JSievers@NadiSoft.de");
    sockPrintf(ANSI_GXY(2,36) ANSI_SATT(0,36,40) "%.78s", ZIDE);
    sockPrintf(ANSI_GXY(2,38) ANSI_NORM "UDP %u | Enter: redraw | 80 columns x 40 rows", MONITOR_PORT);
    sockPrintf(ANSI_GXY(1,40) ANSI_NORM ANSI_CON);
    setAlarm(rtc_alarm,1,0,0,0,30);
    clientok = 1;
}

static void monitorReceive(void)
{
    struct freertos_sockaddr nextClient;
    socklen_t addressLength = sizeof(nextClient);
    char input;
    BaseType_t received;

    /* UDP has no disconnect notification. Every new datagram selects the
     * current viewer, including a new nc process with a different source port.
     * Do not overwrite the active address when the timed receive expires. */
    received = FreeRTOS_recvfrom(xSocketTrace, &input, 1, 0,
                                 &nextClient, &addressLength);
    if(received > 0)
    {
        xClient = nextClient;
        monitorStart();
    }
}

void sysinfo(void* param)
{
    static const TickType_t receiveTimeOut = 200;

    while(FreeRTOS_IsNetworkUp() == pdFALSE)
        vTaskDelay(3000);

    xSocketTrace = prvOpenUDPServerSocket(MONITOR_PORT);
    configASSERT(xSocketTrace != FREERTOS_INVALID_SOCKET);
    FreeRTOS_setsockopt(xSocketTrace, 0, FREERTOS_SO_RCVTIMEO,
                       &receiveTimeOut, sizeof(receiveTimeOut));
    for(;;)
    {
        if(!clientok)
        {
            monitorReceive();
            continue;
        }
        sys_netinfo();
        monitorReceive();
        sys_rtcinfo();
        monitorReceive();
        sys_heapinfo();
        monitorReceive();
        sys_buttoninfo();
        monitorReceive();
    }
}

/* ############################################################################# */
