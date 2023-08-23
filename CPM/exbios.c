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


	File: 	exbios.c
			extended CBIOS (CP/M 2.2 BIOS) to support CP/M 2.2 tasks
			Part of FreeRTOS Port for the eZ80F91 Development Kit eZ80F910300ZCOG
			See www.zilog.com for desciption.


	Developer:
	JSIE	 Juergen Sievers <JSievers@NadiSoft.de>

	150804:	JSIE Start this port.

*/
#ifdef CPM22
/* FreeRTOS includes. */

#include "exbios.h"
#include "task.h"
#include "queue.h"

/* FreeRTOS+CLI includes. */
// #include "FreeRTOS_CLI.h"

/* FreeRTOS+TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

#include "cpmrdsk.h"
#include "exbios.h"
#include <string.h>
#include "printf.h"
extern uint8_t loader;

extern uint8_t dst_minimon;
extern uint8_t src_minimon;
extern unsigned len_minimon;

extern uint8_t dst_bootloader;
extern uint8_t src_bootloader;
extern unsigned len_bootloader;
extern uint8_t dst_ccp;
extern uint8_t src_ccp;
extern unsigned len_ccp;
extern uint8_t dst_bdos;
extern uint8_t src_bdos;
extern unsigned len_bdos;
extern uint8_t dst_bios;
extern uint8_t src_bios;
extern unsigned len_bios;



#define	VERSION "(C) 2021 v2.0.0 "

// CP/M 2.2 console in queue
static QueueHandle_t cpminq;

static Socket_t xConnectedSocket = (Socket_t)-1;
static const char pcLoadRamdiskMessage[] = "Be patient decompressing and loading of the Ramdisk takes a while.\n\r";
static const char pcLoadMonitorMessage[] = "Starting ZSID stand alone.\n\r";
#define MINIMON 0xD000
static const char pcWelcomeMessage[] = "\n\rEZ80F91 CP/M 2.2 Console " VERSION "\n\r";

static TaskHandle_t thcpm;
static Socket_t xSocketRDisk;
static unsigned char *ramdisk = NULL;
static struct freertos_sockaddr xRDiskAddress;
static uint8_t  curdisk = -1;
static uint16_t curFDCT = 0;	// fdc-port: # of track
static uint16_t curFDCS = 0;	// fdc-port: # of sector
static uint8_t  curFDCST= 0;	// fdc-port: status;
static uint8_t  curFDOP = 0;	// fdc-port: command 0=read, 1=write
static char 	*dma= 0;
static dpb_t 	*dpb= 0;
static uint16_t sequenz = 0;

#include "CPM22_A.c"

// Default disk IBM 3740 8" 77Trk*26Sec
static const uint8_t  defxlt[26] = {
	 1, 7,13,19,	// sectors  1, 2, 3, 4
	25, 5,11,17,	// sectors  5, 6, 7, 8
	23, 3, 9,15,	// sectors  9,10,11,12
	21, 2, 8,14,	// sectors 13,14,15,16
	20,26, 6,12,	// sectors 17,18,19,20
	18,24, 4,10,	// sectors 21,22,23,24
	16,22			// sectors 25,26
};

static const dpb_t defdpb = {
	 26,	// sectors per track
	  3,	// block shift factor
	  7,	// block mask
	  0,	// extent mask
	242,	// disk size-1
	 63,	// directory max
	192,	// alloc 0
	  0,	// alloc 1
	 16,	// check size
	  2,	// track offset
};

void prvTCPCpmIOTask( void *ram );

static void* loadz80(const void *dst, const void*src,size_t len)
{
	void *dstmb = farptr(dst);
	memcpy(dstmb,src,len);
	return dstmb;
}

// Injecting and run an Z80 Mashine-Monitor
uint8_t *z80Monitor(trapframe_t *context)
{
	#if 0
	*context->pc = loadz80(&dst_minimon,&src_minimon,len_minimon);
	FreeRTOS_debug_printf(("%s at address %4.4X",pcLoadMonitorMessage,MINIMON));
	FreeRTOS_send( xConnectedSocket,  ( void * ) pcLoadMonitorMessage,  strlen( pcLoadMonitorMessage ), 0 );
	#endif
	return *context->pc;
	
}

uint8_t *ConsoleIO(trapframe_t *context, char dir, port_CONIO_t port)
{
	char c = context->af >> 8;

	if(dir)	// input
	{
		context->af &= 0xFF;
		switch(port)
		{
			case CONSTA:	// console status port
				if(xQueuePeek(cpminq,&c,0) == pdTRUE)
						context->af |= 0xFF00;
				break;
			case CONDAT:	// console data port
				if(xQueueReceive(cpminq,&c,portMAX_DELAY) == pdTRUE)
						context->af |= (c << 8);
				break;
			case PRTSTA:	// printer status port
			case PRTDAT:	// printer data port
			case AUXDAT:	// auxiliary data port
			default:
				break;
		}
	}
	else	// output
		switch(port)
		{
			case CONDAT:	// console data port
				FreeRTOS_send(xConnectedSocket,&c,1,0);
				break;
			case CONSTA:	// console status port
			case PRTSTA:	// printer status port
			case PRTDAT:	// printer data port
			case AUXDAT:	// auxiliary data port
			default:
				break;
		};
	return *context->pc;
}

static uint8_t *z80BiosConsoleIO(trapframe_t *context)
{
	char dir = *farptr((*context->pc)++);
	port_CONIO_t port = (port_CONIO_t) *farptr((*context->pc)++);
	return ConsoleIO(context, dir, port);
}

static const pdu_t *doRamDiskReq(pdu_t *req)
{
	size_t offset;
	pdu_t *rsp = 0;
	pdutype_t cmd = req->hdr.cmdid;

	
	if(!ramdisk)
	{
		switch(cmd)
		{
			case RDSK_Lifesign:
			case RDSK_MountRequest:
			case RDSK_UnmountRequest:
				rsp = rsp = FreeRTOS_GetUDPPayloadBuffer( sizeof(hdr_t), portMAX_DELAY );
				if(rsp)
				{
					memcpy(rsp, req, sizeof(hdr_t));
					rsp->hdr.pdusz = sizeof(hdr_t);
				}
				break;
			case RDSK_ReadRequest:
				rsp = FreeRTOS_GetUDPPayloadBuffer( sizeof(hdr_t) + sizeof(ioreq_t), portMAX_DELAY );

				if(rsp)
				{
					uint8_t sect = req->d.ioreq.sect - 1;
					memcpy(rsp,req,sizeof(hdr_t) + sizeof(ioreq_t) - SECTORSZ);
					if(sect < 26)
					{
						offset = (req->d.ioreq.track * 26 + sect) * SECTORSZ;
						memcpy(rsp->d.ioreq.data,ramdisk+offset,SECTORSZ);
					}
					else
						rsp->hdr.cmdid |= RDSK_ErrorFlag;
				}
				break;
			case RDSK_WriteRequest:
				rsp = FreeRTOS_GetUDPPayloadBuffer( sizeof(hdr_t) + sizeof(ioreq_t) - SECTORSZ, portMAX_DELAY );

				if(rsp)
				{
					uint8_t sect = req->d.ioreq.sect - 1;
					memcpy(rsp,req,sizeof(hdr_t) + sizeof(ioreq_t) - SECTORSZ);
					if(sect < 26)
					{
						offset = (req->d.ioreq.track * 26 + sect) * SECTORSZ;
						memcpy(ramdisk+offset, req->d.ioreq.data,SECTORSZ);
					}
					else
						rsp->hdr.cmdid |= RDSK_ErrorFlag;
				}
				break;
		}
		if(rsp)
		{
			rsp->hdr.cmdid |= RDSK_Response;
			rsp->hdr.seqnz = ~sequenz;
			sequenz++;
			FreeRTOS_ReleaseUDPPayloadBuffer( ( void * ) req );
		}
	}
	return rsp;
}

static const pdu_t *doRDiskReq(pdu_t *req)
{

	pdu_t *rsp = 0;
	int32_t io, iox = req->hdr.pdusz;
	uint24_t cmdrsp = (req->hdr.cmdid|RDSK_Response);

	req->hdr.seqnz = sequenz;

	io = FreeRTOS_sendto( xSocketRDisk, req, req->hdr.pdusz, FREERTOS_ZERO_COPY, &xRDiskAddress, sizeof(xRDiskAddress));
	if(io <= 0)
		FreeRTOS_ReleaseUDPPayloadBuffer( ( void * ) req );
	else if(io == iox)
	{
		struct freertos_sockaddr xFrom;
		socklen_t xFromLength;
		xFromLength = sizeof(xFrom);

		io = FreeRTOS_recvfrom( xSocketRDisk, &rsp, 0, FREERTOS_ZERO_COPY, &xFrom, &xFromLength );
		if(io > 0)
		{
			if(	io < sizeof(hdr_t) ||
				io != rsp->hdr.pdusz   ||
				rsp->hdr.seqnz != (uint16_t) ~sequenz ||
				(rsp->hdr.cmdid & ~RDSK_ErrorFlag) != cmdrsp)
			{
				FreeRTOS_ReleaseUDPPayloadBuffer( ( void * ) rsp);
				rsp = 0;
			}
			else if(rsp->hdr.cmdid & RDSK_ErrorFlag)
				sequenz=0;
			else
				sequenz++;
		}
		else
			rsp = 0;
	}

	return rsp;
}

static    int uzipcb(struct uzlib_uncomp *uncomp)
{
	char prompt ='#';
	FreeRTOS_send( xConnectedSocket,  ( void * ) &prompt,  1, 0 );
	return 0;
}

static uint8_t *z80BiosDiskIO(trapframe_t *context)
{
	char dir = *farptr((*context->pc)++);
	char c = context->af >> 8;
	pdu_t *rsp;

	if(dir)	// input
	{
		context->af = 0xFF;
		switch((port_FDIO_t) *farptr((*context->pc)++))
		{
			case FDCST:	// fdc-port: status
						// Returns A=0 for OK,
						//           1 for unrecoverable error,
						//           2 if disc is readonly,
						//        0FFh if media changed.
				context->af |= (curFDCST << 8);
				break;
			case FDCD:	// fdc-port: # of drive
				context->af |= (curdisk << 8);
				break;
			case FDCTBC:	// fdc-port: # of track
				context->af |= (curFDCT << 8);
				break;
			case FDCSBC:	// fdc-port: # of sector
				context->af |= (curFDCS << 8);
				break;
			case FDCOP:	// fdc-port: command
				context->af |= (curFDCST << 8);
			default:
				context->af |= 0xFF00;
				break;
		}
	}
	else	// output
		switch((port_FDIO_t) *farptr((*context->pc)++))
		{
			case FDCD:	// fdc-port: # of drive
				curdisk = -1;	// invalidate current drive
				curFDCST = 0xFF;

				// disk id in range?
				if(c < 4 || c == 8 || c == 9)
				{
					uint16_t sz;
					pdu_t *req;
					uint8_t  *xlt= 0;

					// if disk parameters given
					if(context->hl)
					{
						// then setup disk parameters
						dph_t *dph = (dph_t*) farptr((uint8_t*)context->hl);
						dpb = (dpb_t*) farptr((uint8_t*)dph->dpb);
						xlt = (uint8_t*) (dph->xlt ? farptr((uint8_t*)dph->xlt) : 0);
					}
					else {
						// else boot from IBM 8" default disk
						dpb = &defdpb;
						xlt = defxlt;
					}

					sz = sizeof(hdr_t) + sizeof(mountreq_t);
					req = FreeRTOS_GetUDPPayloadBuffer( sz, portMAX_DELAY );

					if(req)
					{
						int i;
						uint8_t *_xlt = &req->d.mountreq.xlt;

						req->hdr.pdusz = sz;					// size of this request
						req->hdr.cmdid = RDSK_MountRequest;		// request type
						req->hdr.devid = c;						// drive id 0=A, 1=B ...
						memcpy(&req->d.mountreq.dpb, dpb, sizeof(dpb_t));	// save Disk Parameter Block
						snprintf((char*)req->d.mountreq.diskid,13U,"%c",'A' + c); // default name
						req->d.mountreq.mode = 0;//LINEAR;						// No sector demapping
						req->d.mountreq.secsz = SECSIZE;					// CP/M 128 sector size

						// append sector translation table
						for( i = 0; i < dpb->spt; i++)
							_xlt[i] = xlt ? xlt[i] : i+1;

						//FreeRTOS_debug_printf(("req: cmd=%hhd, did=%hhd, sz=%hd sqz=%hd\n",req->hdr.cmdid,req->hdr.devid,req->hdr.pdusz,req->hdr.seqnz));

						if(!req->hdr.devid)
							rsp = doRamDiskReq(req);
						else
						{
							// Request from server
							uint8_t devid = req->hdr.devid;
							rsp = doRDiskReq(req);
						}

						if( rsp )
						{
							//FreeRTOS_debug_printf(("rsp: cmd=%hhd, did=%hhd, sz=%hd sqz=%hd\n",rsp->hdr.cmdid,rsp->hdr.devid,rsp->hdr.pdusz,rsp->hdr.seqnz));
							if(!(rsp->hdr.cmdid & RDSK_ErrorFlag))
							{
								// OK, set active disk and status OK
								curdisk = c;
								curFDCST = 0;
							}
							FreeRTOS_ReleaseUDPPayloadBuffer( ( void * ) rsp);
						}
						else
							context->hl = 0;
					}
				}

				// return hl=0 (dph) if no drive selected.
				if(curdisk == -1)
					context->hl = 0;
				break;
			case FDCTBC:	// fdc-port: # of track
				curFDCT = *(uint16_t*) &context->bc;
				break;
			case FDCSBC:	// fdc-port: # of sector
				curFDCS =  *(uint16_t*) &context->bc;
				break;
			case FDCOP:	// fdc-port: command
				curFDOP = (context->af >> 8) & 0xFF;
				 //Returns A=0 for OK, 1 for unrecoverable error, 0FFh if media changed.
				curFDCST = 0xFF;
				if(curdisk >= 0)
				{
					size_t pdusz = sizeof(hdr_t) + sizeof(ioreq_t);
					curFDCST = 1;
					if(c)	// write
					{
						pdu_t *req = FreeRTOS_GetUDPPayloadBuffer( pdusz, portMAX_DELAY );

						if(req)
						{
							req->hdr.pdusz = pdusz;
							req->hdr.cmdid = RDSK_WriteRequest;
							req->hdr.devid = curdisk;
							req->d.ioreq.track = curFDCT;
							req->d.ioreq.sect = curFDCS;
							memcpy(&req->d.ioreq.data, dma, SECSIZE);
							if(!curdisk)
								rsp = doRamDiskReq(req);
							else
								rsp = doRDiskReq(req);

							if(rsp)
							{
								if(!(rsp->hdr.cmdid & RDSK_ErrorFlag))
									curFDCST = 0;
								FreeRTOS_ReleaseUDPPayloadBuffer( ( void * ) rsp);
							}
						}
					}
					else	// readt
					{
						pdu_t *req = FreeRTOS_GetUDPPayloadBuffer(pdusz - SECTORSZ, portMAX_DELAY );
						if(req)
						{
							pdu_t *rsp;

							req->hdr.pdusz = pdusz - SECTORSZ;
							req->hdr.cmdid = RDSK_ReadRequest;
							req->hdr.devid = curdisk;
							req->d.ioreq.track = curFDCT;
							req->d.ioreq.sect  = curFDCS;
							if(!curdisk)
								rsp = doRamDiskReq(req);
							else
								rsp = doRDiskReq(req);
							if(rsp)
							{
								if(!(rsp->hdr.cmdid & RDSK_ErrorFlag))
								{
									memcpy(dma, &rsp->d.ioreq.data, SECSIZE);
									curFDCST = 0;
								}
								FreeRTOS_ReleaseUDPPayloadBuffer( ( void * ) rsp);
							}
						}
					}
				}
				break;
			case FDCST:	// fdc-port: status
				curFDCST = c;
			default:
				break;
		};
	return *context->pc;
}

uint8_t *z80BiosDMAIO(trapframe_t *context)
{
	char dir = *farptr((*context->pc)++);
	char c = context->af >> 8;
	char port = *farptr((*context->pc)++);
	//if(port == DMABC)
	{
		if(dir)	// input
			context->bc = (uint16_t)dma;
		else
		{
			dma = (char*) (*(uint16_t*)&context->bc | mbase());
		}
	}
	return *context->pc;
}

uint8_t *Romboot(trapframe_t* arg)
{
	asm("jp 0");
	return 0;
}

uint8_t *exbioscall(trapframe_t* arg)
{
	xebioscall_t bc = (xebioscall_t)*farptr((*arg->pc)++);
	switch(bc)
	{
		case MONITOR:
			z80Monitor(arg);
		break;
		case CONIO:
			z80BiosConsoleIO(arg);
		break;
		case FDIO:
			z80BiosDiskIO(arg);
		break;
		case DMAIO:
			z80BiosDMAIO(arg);
		break;
		case ROMBOOT:
			Romboot(arg);
		break;
		case BDOSCALL:
//			bdos(arg);
		break;
		default:
			configASSERT(0);
		break;
	}
	return *arg->pc;
}

static Socket_t prvOpenTCPServerSocket( uint16_t usPort )
{
	struct freertos_sockaddr xBindAddress;
	Socket_t xSocket;

	static const TickType_t xReceiveTimeOut = portMAX_DELAY; // pdMS_TO_TICKS(1000);
	static const TickType_t xTransmitTimeOut = portMAX_DELAY; // pdMS_TO_TICKS(1000);

	const BaseType_t xBacklog = 1;
	BaseType_t xReuseSocket = pdTRUE;

	/* Attempt to open the socket. */
	xSocket = FreeRTOS_socket( FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP );
	configASSERT( xSocket != FREERTOS_INVALID_SOCKET );

	/* Set a time out so accept() will just wait for a connection. */
	FreeRTOS_setsockopt( xSocket, 0, FREERTOS_SO_RCVTIMEO, &xReceiveTimeOut, sizeof( xReceiveTimeOut ) );
	FreeRTOS_setsockopt( xSocket, 0, FREERTOS_SO_SNDTIMEO, &xTransmitTimeOut, sizeof( xTransmitTimeOut ) );

	/* Only one connection will be used at a time, so re-use the listening
	socket as the connected socket.  See SimpleTCPEchoServer.c for an example
	that accepts multiple connections. */
	//FreeRTOS_setsockopt( xSocket, 0, FREERTOS_SO_REUSE_LISTEN_SOCKET, &xReuseSocket, sizeof( xReuseSocket ) );

	/* NOTE:  The CLI is a low bandwidth interface (typing characters is slow),
	so the TCP window properties are left at their default.  See
	SimpleTCPEchoServer.c for an example of a higher throughput TCP server that
	uses are larger RX and TX buffer. */

	/* Bind the socket to the port that the client task will send to, then
	listen for incoming connections. */
	xBindAddress.sin_port = usPort;
	xBindAddress.sin_port = FreeRTOS_htons( xBindAddress.sin_port );
	FreeRTOS_bind( xSocket, &xBindAddress, sizeof( xBindAddress ) );
	FreeRTOS_listen( xSocket, xBacklog );

	return xSocket;
}

static void prvGracefulShutdown( Socket_t xSocket )
{
	TickType_t xTimeOnShutdown;

	/* Initiate a shutdown in case it has not already been initiated. */
	FreeRTOS_shutdown( xSocket, FREERTOS_SHUT_RDWR );

	/* Wait for the shutdown to take effect, indicated by FreeRTOS_recv()
	returning an error. */
	xTimeOnShutdown = xTaskGetTickCount();
	do
	{
		char c;
		if( FreeRTOS_recv( xSocket, &c,1, 0 ) < 0 )
		{
			break;
		}
	} while( ( xTaskGetTickCount() - xTimeOnShutdown ) < pdMS_TO_TICKS(5000) );

	/* Finished with the socket and the task. */
	FreeRTOS_closesocket( xSocket );
}

void CPM22Task(uint8_t* ram)
{
	memcpy(ram+0x80,ramdisk,0x80);
	asm(" ld		a,(ix+8)");
	asm(" ld		mb,a");
	asm(" jp.s	80h");
}

void prvTCPCpmIOTask( void *ram )
{
	static const TickType_t xReceiveTimeOut =  pdMS_TO_TICKS(1000);
	static const TickType_t xTransmitTimeOut = pdMS_TO_TICKS(1000);
	static BaseType_t cpmthread = pdFALSE;
	BaseType_t iosize;

	char cRxedChar, cInputIndex = 0;
	struct freertos_sockaddr xClient;
	Socket_t xListeningSocket;
	socklen_t xSize = sizeof( xClient );
	cpminq = xQueueCreate(81, sizeof( CHAR));

	while(FreeRTOS_IsNetworkUp() == pdFALSE)
		vTaskDelay(pdMS_TO_TICKS(1000));


	/* Create the socket. */
	xSocketRDisk = FreeRTOS_socket( FREERTOS_AF_INET,
	FREERTOS_SOCK_DGRAM,
	FREERTOS_IPPROTO_UDP );


	/* Check the socket was created. */
	configASSERT( xSocketRDisk != FREERTOS_INVALID_SOCKET );
	/* Set a time out so accept() will just wait for a connection. */
	FreeRTOS_setsockopt( xSocketRDisk, 0, FREERTOS_SO_RCVTIMEO, &xReceiveTimeOut, sizeof( xReceiveTimeOut ) );
	FreeRTOS_setsockopt( xSocketRDisk, 0, FREERTOS_SO_SNDTIMEO, &xTransmitTimeOut, sizeof( xTransmitTimeOut ) );
	xListeningSocket = prvOpenTCPServerSocket( RDSK_PORT);
	/* Nothing for this task to do if the socket cannot be created. */
	if( xListeningSocket == FREERTOS_INVALID_SOCKET )
	{
		vTaskDelete( NULL );
	}


	for( ;; )
	{

		/* Wait for an incoming connection. */
		xConnectedSocket = FreeRTOS_accept( xListeningSocket, &xClient, &xSize );

		/* The FREERTOS_SO_REUSE_LISTEN_SOCKET option is set, so the
		connected and listening socket should be the same socket.
		configASSERT( xConnectedSocket == xListeningSocket ); */
		xRDiskAddress.sin_addr = xClient.sin_addr;
		xRDiskAddress.sin_port = FreeRTOS_htons( RDSK_PORT );

		if(!ramdisk)
		{
			size_t sz = cpm22img_length;
			ramdisk = pvPortMalloc(256256);
			if(ramdisk)
				memcpy(ramdisk,cpm22img,sz);
			else
				break;
		}
		
		if(cpmthread != pdPASS)
		{
			cpmthread = xTaskCreate( CPM22Task, "CPM22Task", configMINIMAL_STACK_SIZE*5, ram, 3,&thcpm);
			if(cpmthread != pdPASS)
			{
				prvGracefulShutdown( xListeningSocket );
				vTaskDelete( NULL );
			}

			/* Send the welcome message. */
			iosize = FreeRTOS_send( xConnectedSocket,  ( void * ) pcWelcomeMessage,  strlen( pcWelcomeMessage ), 0 );
			xQueueReset(cpminq);
		}

		/* Process the socket as long as it remains connected. */
		do {
			char c;
			/* Receive data on the socket. */
			iosize = FreeRTOS_recv( xConnectedSocket, &c, 1, 0 );

			if( iosize >= 0 )
			{
				xQueueSend(cpminq,&c,0);
			}
			else
			{
				/* Socket closed? */
				// vTaskDelete( thcpm );
				// clrtrap();
				break;
			}
		} while( iosize >= 0 );
		
		/* Close the socket correctly. */
		// prvGracefulShutdown( xListeningSocket );
	}
}
#endif // ifdef CPM22
