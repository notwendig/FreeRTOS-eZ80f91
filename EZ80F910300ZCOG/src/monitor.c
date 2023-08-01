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
static socklen_t xClientAddressLength = 0;
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
		iores = FreeRTOS_sendto( xSocketTrace, pucUDPPayloadBuffer, res, FREERTOS_ZERO_COPY, &xClient, sizeof(xClient));
		if(iores == 0)
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

void sys_heapinfo()
{
	HeapStats_t pxHeapStats;
	vPortGetHeapStats(&pxHeapStats);
	sockPrintf( ANSI_SCUR ANSI_COFF);
	sockPrintf( ANSI_SATT(0,34,43) ANSI_GXY(85,4) " Heap                  ");
	sockPrintf( ANSI_SATT(0,37,40) ANSI_GXY(85,5) "Free total   : " ANSI_SATT(0,32,40) "%8u", pxHeapStats.xAvailableHeapSpaceInBytes);	/* The total heap size currently available - this is the sum of all the free blocks, not the largest block that can be allocated. */
	sockPrintf( ANSI_SATT(0,37,40) ANSI_GXY(85,6) "Free biggest : " ANSI_SATT(0,32,40) "%8u", pxHeapStats.xSizeOfLargestFreeBlockInBytes);	/* The maximum size, in bytes, of all the free blocks within the heap at the time vPortGetHeapStats() is called. */
	sockPrintf( ANSI_SATT(0,37,40) ANSI_GXY(85,7) "Free smallest: " ANSI_SATT(0,32,40) "%8u", pxHeapStats.xSizeOfSmallestFreeBlockInBytes);/* The minimum size, in bytes, of all the free blocks within the heap at the time vPortGetHeapStats() is called. */
	sockPrintf( ANSI_SATT(0,37,40) ANSI_GXY(85,8) "Free fracment: " ANSI_SATT(0,32,40) "%8u", pxHeapStats.xNumberOfFreeBlocks);			/* The number of free memory blocks within the heap at the time vPortGetHeapStats() is called. */
	sockPrintf( ANSI_SATT(0,37,40) ANSI_GXY(85,9) "Alloc calls  : " ANSI_SATT(0,32,40) "%8u", pxHeapStats.xNumberOfSuccessfulAllocations); /* The number of calls to pvPortMalloc() that have returned a valid memory block. */
	sockPrintf( ANSI_SATT(0,37,40) ANSI_GXY(85,10)"Free calls   : " ANSI_SATT(0,32,40) "%8u", pxHeapStats.xNumberOfSuccessfulFrees);		/* The number of calls to vPortFree() that has successfully freed a block of memory. */
	sockPrintf( ANSI_SATT(0,37,40) ANSI_GXY(85,11)"Free min     : " ANSI_SATT(0,32,40) "%8u", pxHeapStats.xMinimumEverFreeBytesRemaining);/* The minimum amount of total free memory (sum of all free blocks) there has been in the heap since the system booted. */
}

void sys_netinfo()
{
	uint32_t ulIPAddress;
	uint32_t ulNetMask;
    uint32_t ulGatewayAddress;
    uint32_t ulDNSServerAddress;
	uint16_t phyData;
	const char *att;
	char mod[40] = "NO LINK";
	emacStat_t* stats = emac_stat();

	/* The network is up and configured.  Print out the configuration,
	which may have been obtained from a DHCP server. */
	if( xIsEthernetConnected( ) == pdTRUE )
    {
		att = ANSI_SATT(0,33,40);
        usPHY_ReadReg(PHY_DIAG_REG, &phyData);
        if(phyData & PHY_100_MBPS)
        {
            strncpy(mod,"100 Mbps",sizeof(mod));
        }
        else
        {
            strncpy(mod," 10 Mbps",sizeof(mod));
        }

        if(phyData & PHY_FULL_DUPLEX)
        {
            strncat(mod, ", Full-Duplex",sizeof(mod));
        }
        else
        {
            strncat(mod, ", Half-Duplex",sizeof(mod));
        }
		FreeRTOS_GetAddressConfiguration( &ulIPAddress,
									  &ulNetMask,
									  &ulGatewayAddress,
									  &ulDNSServerAddress );

	}
	else
	{
		att = ANSI_SATT(0,31,40);
		ulIPAddress = 0UL;
		ulNetMask = 0UL;
		ulGatewayAddress = 0UL;
		ulDNSServerAddress = 0UL;
	}


	sockPrintf( ANSI_SCUR ANSI_COFF);
	sockPrintf( ANSI_SATT(0,34,43) ANSI_GXY( 5,4) " Ethernet                                      ");
	sockPrintf( ANSI_SATT(0,36,40) ANSI_GXY( 5,5) "Network: %s%s, %-23s", att, "Up  ",mod);
	sockPrintf( ANSI_SATT(0,36,40) ANSI_GXY( 5,6) "IP     : %s%-15lxip ", att, FreeRTOS_htonl(ulIPAddress));
	sockPrintf( ANSI_SATT(0,36,40) ANSI_GXY(30,6) "Mask   : %s%-15lxip ", att, FreeRTOS_htonl(ulNetMask));
	sockPrintf( ANSI_SATT(0,36,40) ANSI_GXY( 5,7) "Gateway: %s%-15lxip ", att, FreeRTOS_htonl(ulGatewayAddress));
	sockPrintf( ANSI_SATT(0,36,40) ANSI_GXY(30,7) "DNS    : %s%-15lxip ", att, FreeRTOS_htonl(ulDNSServerAddress));
	sockPrintf( ANSI_SATT(0,36,40) ANSI_GXY( 5,8) "txsz   : %s%-15u "	, att, stats->txsz);
	sockPrintf( ANSI_SATT(0,36,40) ANSI_GXY(30,8) "rxsz   : %s%-15u "	, att, stats->rxsz);
	sockPrintf( ANSI_SATT(0,36,40) ANSI_GXY( 5,9) "txdone : %s%-15u "	, att, stats->txdone);
	sockPrintf( ANSI_SATT(0,36,40) ANSI_GXY(30,9) "rxdone : %s%-15u "	, att, stats->rxdone);
	sockPrintf( ANSI_SATT(0,36,40) ANSI_GXY( 5,10)"txover : %s%-15u "	, att, stats->txover);
	sockPrintf( ANSI_SATT(0,36,40) ANSI_GXY(30,10)"rxover : %s%-15u "	, att, stats->rxover);
	sockPrintf( ANSI_SATT(0,36,40) ANSI_GXY( 5,11)"txpcf  : %s%-15u "	, att, stats->txpcf);
	sockPrintf( ANSI_SATT(0,36,40) ANSI_GXY(30,11)"rxpcf  : %s%-15u "	, att, stats->rxpcf);

	sockPrintf( ANSI_SATT(0,36,40) ANSI_GXY( 5,12)"txabort: %s%-15u "	, att, stats->txabort);
	sockPrintf( ANSI_SATT(0,36,40) ANSI_GXY(30,12)"rxnotok: %s%-15u "	, att, stats->rxnotok);
	sockPrintf( ANSI_SATT(0,36,40) ANSI_GXY( 5,13)"txfsmer: %s%-15u "	, att, stats->txfsmerr);
	sockPrintf( ANSI_SATT(0,36,40) ANSI_GXY(30,13)"mgdone : %s%-15u "	, att, stats->mgdone);
	sockPrintf( ANSI_SATT(0,36,40) ANSI_GXY( 5,14)"rxnospc: %s%-15u "	, att, stats->rxnospace);
	sockPrintf( ANSI_SATT(0,36,40) ANSI_GXY(30,14)"rxinvsz: %s%-15u "	, att, stats->rxinvsize);

	sockPrintf( ANSI_SATT(0,36,40) ANSI_GXY( 5,15)"rxcrcer: %s%-15u "	, att, stats->rxcrcerr);
	sockPrintf( ANSI_SATT(0,36,40) ANSI_GXY(30,15)"rxalign: %s%-15u "	, att, stats->rxalignerr);
	sockPrintf( ANSI_SATT(0,36,40) ANSI_GXY( 5,16)"rxlongs: %s%-15u "	, att, stats->rxlongevent);
	sockPrintf( ANSI_SATT(0,36,40) ANSI_GXY(30,16)"rxok   : %s%-15u "	, att, stats->rxok);
	sockPrintf( ANSI_SATT(0,36,40) ANSI_GXY( 5,17)"rxcf   : %s%-15u "	, att, stats->rxcf);
	sockPrintf( ANSI_RCUR ANSI_CON) ;
}


void sys_rtcinfo()
{
	char clock[21];
	const char* att = chkRTCPowerLost() ? ANSI_SATT(1,31,40):ANSI_SATT(0,32,40);

	sockPrintf( ANSI_SCUR ANSI_COFF);
	sockPrintf( ANSI_SATT(0,34,43) ANSI_GXY(55,4) " Real Time Clock           ");
	sockPrintf( ANSI_SATT(0,36,40) ANSI_GXY(55,5) "Date : %s%-22s", att, getsDate(clock,sizeof(clock)));
	sockPrintf( ANSI_SATT(0,36,40) ANSI_GXY(55,6) "Time : %s%-22s", att, getsTime(clock,sizeof(clock)));
	sockPrintf( ANSI_SATT(0,36,40) ANSI_GXY(55,7) "Alarm: %s%-22s", att, getsAlarm(clock,sizeof(clock)));
	if(!alarmflg || !--alarmflg)
		sockPrintf( ANSI_SATT(0,31,40) ANSI_GXY(62,8) ">>> ALARM %6u <<<", alarms);
	else
		sockPrintf( ANSI_SATT(7,31,40) ANSI_GXY(62,8) ">>> ALARM %6u <<<", alarms);

	sockPrintf( ANSI_RCUR ANSI_CON) ;
}

void sys_buttoninfo()
{
	static const char  *const label[3] = {"Left   ","Middle ","Right  "};
	static const char  *const state[3] = {"?????","Open ","Close"};
	static TickType_t cnt[3];
	const char* att;
	int i;

	sockPrintf( ANSI_SCUR ANSI_COFF);
	sockPrintf( ANSI_SATT(0,34,43) ANSI_GXY(55,12) " Platform buttons                        ");
	sockPrintf( ANSI_SATT(0,36,40) ANSI_GXY(62,13) "   n  ,  T-1   ,   T    , State ");

	for( i=BUTTON_LEFT; i<=BUTTON_RIGTH; i++)
	{
		char b[80];
		button_t *but = get_button(i);
		const char *s = state[but->state +1];

		snprintf(b,sizeof(b)
		, ANSI_SATT(0,36,40) "\x1b[%i;55f%s: %s%4u,%8u,%8u %s",14+i, label[i]
		,(char*) ((cnt[i] != but->changes)? ANSI_SATT(1,31,40):ANSI_SATT(0,32,40))
		,(cnt[i] = but->changes), but->privT, but->lastT, s);
		sockPrintf(b);

	}
	sockPrintf( ANSI_RCUR ANSI_CON) ;
}

static xSocket_t prvOpenUDPServerSocket( uint16_t usPort )
{
struct freertos_sockaddr xServer;
xSocket_t xSocket = FREERTOS_INVALID_SOCKET;

	xSocket = FreeRTOS_socket( FREERTOS_AF_INET, FREERTOS_SOCK_DGRAM, FREERTOS_IPPROTO_UDP );
	if( xSocket != FREERTOS_INVALID_SOCKET)
	{
		/* Zero out the server structure. */
		memset( ( void * ) &xServer, 0x00, sizeof( xServer ) );
		/* Set family and port. */
		xServer.sin_port = FreeRTOS_htons( usPort );
		/* Bind the address to the socket. */
		if( FreeRTOS_bind( xSocket, &xServer, sizeof( xServer ) ) == -1 )
		{
			FreeRTOS_closesocket( xSocket );
			xSocket = FREERTOS_INVALID_SOCKET;
		}
	}

	return xSocket;
}

void sysinfo(void* param)
{
	BaseType_t iosize;
	char cLocalBuffer;

	while(FreeRTOS_IsNetworkUp() == pdFALSE)
		vTaskDelay(3000);

	/* Create the socket. */
	
	xSocketTrace = prvOpenUDPServerSocket(MONITOR_PORT);
	//FreeRTOS_socket( FREERTOS_AF_INET, FREERTOS_SOCK_DGRAM, FREERTOS_IPPROTO_UDP );
	configASSERT( xSocketTrace != FREERTOS_INVALID_SOCKET );
	do{
		/* Wait for incoming data on the opened socket. */
		xClientAddressLength= sizeof(xClient);
		iosize = FreeRTOS_recvfrom( xSocketTrace, ( void * ) &cLocalBuffer, 1, 0, &xClient, &xClientAddressLength );
		if(iosize <= 0)
			continue;

		sockPrintf("\n\n\n\n\n\n\n\n\n\n");
		sockPrintf(ANSI_NORM ANSI_CLRS ANSI_SATT(0,34,47) ANSI_GXY(1,1)  " FreeRTOS " tskKERNEL_VERSION_NUMBER " Demo V" VERSION " on ZiLOG\'s " DEVKIT " Kit / " ZIDE ANSI_DEOL);
		sockPrintf(                    ANSI_SATT(0,34,47) ANSI_GXY(1,2)  " Autor " AUTOR " www.NadiSoft.de <" AUTORMAIL ">" ANSI_DEOL ANSI_NORM);

		setAlarm(rtc_alarm,1,0, 0, 0, 30);
		clientok = 1;
		while(clientok)
		{
			sys_netinfo();
			vTaskDelay(200);
			sys_rtcinfo();
			vTaskDelay(200);
			sys_heapinfo();
			vTaskDelay(200);
			sys_buttoninfo();
			vTaskDelay(200);
		}
	}while(1);
}


/* ############################################################################# */
