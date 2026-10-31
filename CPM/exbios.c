/*
    FreeRTOS - Copyright (C) 2016 Real Time Engineers Ltd.
    All rights reserved
    
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
#include <eZ80F91.h>
#include "cpm_disks.h"
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

#define	VERSION "(C) 2021 v2.0.1 "

// CP/M 2.2 console in queue
static QueueHandle_t cpminq;

static const char pcLoadMonitorMessage[] = "Starting ZSID stand alone.\n\r";
#define MINIMON 0xD000
static const char pcWelcomeMessage[] = "\n\rEZ80F91 CP/M 2.2 Console " VERSION "\n\r";

static TaskHandle_t thcpm;
static Socket_t xConnectedSocket;
static unsigned short cpmDevPort;
extern unsigned char cpmPortRead(unsigned short port);
extern void cpmPortWrite(unsigned short port, unsigned char value);

static unsigned char cpmFdcWait(unsigned char mask)
{
    unsigned short retry = 10000;
    unsigned char status;
    do {
        status = cpmPortRead(cpmDevPort);
        if(!(status & 1) || (mask && (status & mask)))
            return status;
    } while(--retry);
    cpmPortWrite(cpmDevPort, 0xD0);
    return 0x80;
}

static unsigned char cpmFdcSelect(unsigned char drive)
{
    cpmPortWrite(cpmDevPort+4, drive);
    return (cpmPortRead(cpmDevPort+5) & 0x80) != 0;
}

static unsigned char cpmFdcSector(unsigned char write, unsigned char drive,
    unsigned short track, unsigned short sector, unsigned char *data)
{
    unsigned char status;
    unsigned short i;
    size_t offset;
    if(!cpm_disk_sector(drive, track, sector, &offset) || !cpmFdcSelect(drive))
        return 0;
    cpmPortWrite(cpmDevPort+6, track >> 8); /* Board extension for I:/J:. */
    cpmPortWrite(cpmDevPort+3, track & 255);
    cpmPortWrite(cpmDevPort, 0x10); /* Seek, target in Data Register. */
    if(cpmFdcWait(0) & 0x98)
        return 0;
    cpmPortWrite(cpmDevPort+2, sector);
    cpmPortWrite(cpmDevPort, write ? 0xA0 : 0x80);
    for(i=0; i<128; i++)
    {
        status = cpmFdcWait(2);
        if(!(status & 2) || (status & 0xDC))
            return 0;
        if(write)
            cpmPortWrite(cpmDevPort+3, data[i]);
        else
            data[i] = cpmPortRead(cpmDevPort+3);
    }
    return !(cpmFdcWait(0) & 0xDD);
}
static uint8_t  curdisk = -1;
static uint16_t curFDCT = 0;	// fdc-port: # of track
static uint16_t curFDCS = 0;	// fdc-port: # of sector
static uint8_t  curFDCST= 0;	// fdc-port: status;
static uint8_t  curFDOP = 0;	// fdc-port: command 0=read, 1=write
static char 	*dma= 0;
static dpb_t 	*dpb= 0;
static uint16_t sequenz = 0;

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
    pdu_t *rsp;
    uint8_t valid = 0;
    size_t length = sizeof(hdr_t);
    pdutype_t cmd = req->hdr.cmdid;
    if(cmd == RDSK_ReadRequest)
        length += sizeof(ioreq_t);
    else if(cmd == RDSK_WriteRequest)
        length += sizeof(ioreq_t) - SECTORSZ;
    rsp = FreeRTOS_GetUDPPayloadBuffer(length, portMAX_DELAY);
    if(!rsp)
        return NULL;
    memcpy(rsp, req, sizeof(hdr_t));
    rsp->hdr.pdusz = length;
    switch(cmd)
    {
        case RDSK_Lifesign:
        case RDSK_MountRequest:
        case RDSK_UnmountRequest:
            valid = cpm_disk_present(req->hdr.devid) && cpmFdcSelect(req->hdr.devid);
            break;
        case RDSK_ReadRequest:
            memcpy(&rsp->d.ioreq, &req->d.ioreq, sizeof(ioreq_t) - SECTORSZ);
            valid = cpmFdcSector(0, req->hdr.devid, req->d.ioreq.track,
                req->d.ioreq.sect, rsp->d.ioreq.data);
            if(!valid)
                memset(rsp->d.ioreq.data, 0, SECTORSZ);
            break;
        case RDSK_WriteRequest:
            memcpy(&rsp->d.ioreq, &req->d.ioreq, sizeof(ioreq_t) - SECTORSZ);
            valid = cpmFdcSector(1, req->hdr.devid, req->d.ioreq.track,
                req->d.ioreq.sect, req->d.ioreq.data);
            break;
        default:
            valid = 0;
            break;
    }
    if(!valid)
        rsp->hdr.cmdid |= RDSK_ErrorFlag;
    rsp->hdr.cmdid |= RDSK_Response;
    rsp->hdr.seqnz = ~sequenz++;
    FreeRTOS_ReleaseUDPPayloadBuffer(req);
    return rsp;
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

						rsp = doRamDiskReq(req);

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
							rsp = doRamDiskReq(req);

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
							rsp = doRamDiskReq(req);
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

void CPM22Task(uint8_t* ram)
{
    unsigned char cfg[8];
    unsigned char i;
    CS3_CTL = 0; /* No memory-mapped disk or CS3 RAM is required. */
    for(i=0; i<8; i++)
        cfg[i] = cpmPortRead(0xFF00U+i);
    if(cfg[0] != 'W' || cfg[1] != 'D' || cfg[2] != '9' || cfg[3] != '7' ||
       cfg[6] != 1 || !(cfg[7] & 1))
    {
        static const char error[] = "WD2797A device missing.\r\n";
        FreeRTOS_send(xConnectedSocket, (void *)error, sizeof(error)-1, 0);
        thcpm = NULL;
        vTaskDelete(NULL);
        return;
    }
    cpmDevPort = (unsigned short)cfg[4] | ((unsigned short)cfg[5] << 8);
    if(!cpmFdcSector(0, 0, 0, 1, ram+0x80))
    {
        static const char error[] = "WD2797A boot read A: failed.\r\n";
        FreeRTOS_send(xConnectedSocket, (void *)error, sizeof(error)-1, 0);
        thcpm = NULL;
        vTaskDelete(NULL);
        return;
    }
	asm(" ld		a,(ix+8)");
	asm(" ld		mb,a");
	asm(" jp.s	80h");
}

void prvTCPCpmIOTask( void *ram )
{
struct freertos_sockaddr xClient, xBindAddress;
Socket_t xListeningSocket;
socklen_t xSize = sizeof( xClient );
static const TickType_t xReceiveTimeOut = portMAX_DELAY;
const BaseType_t xBacklog = 1;
BaseType_t iosize;
int cpmthread = pdFALSE;	

    /* Attempt to open the socket. */
    xListeningSocket = FreeRTOS_socket( FREERTOS_AF_INET, 
                                        FREERTOS_SOCK_STREAM,
                                        FREERTOS_IPPROTO_TCP );

    /* Check the socket was created. */
    configASSERT( xListeningSocket != FREERTOS_INVALID_SOCKET );

    /* If FREERTOS_SO_RCVBUF or FREERTOS_SO_SNDBUF are to be used with
    FreeRTOS_setsockopt() to change the buffer sizes from their default then do
    it here!.  (see the FreeRTOS_setsockopt() documentation. */

    /* If ipconfigUSE_TCP_WIN is set to 1 and FREERTOS_SO_WIN_PROPERTIES is to
    be used with FreeRTOS_setsockopt() to change the sliding window size from
    its default then do it here! (see the FreeRTOS_setsockopt()
    documentation. */

    /* Set a time out so accept() will just wait for a connection. */
    FreeRTOS_setsockopt( xListeningSocket,
                         0,
                         FREERTOS_SO_RCVTIMEO,
                         &xReceiveTimeOut,
                         sizeof( xReceiveTimeOut ) );

    /* Set the listening port. */
    xBindAddress.sin_port = FreeRTOS_htons( RDSK_PORT );

    /* Bind the socket to the port that the client RTOS task will send to. */
    FreeRTOS_bind( xListeningSocket, &xBindAddress, sizeof( xBindAddress ) );

    /* Set the socket into a listening state so it can accept connections.
    The maximum number of simultaneous connections is limited to 1 */
    FreeRTOS_listen( xListeningSocket, xBacklog );

	for( ;; )
	{

		/* Wait for an incoming connection. */
		xConnectedSocket = FreeRTOS_accept( xListeningSocket, &xClient, &xSize );
		configASSERT( xConnectedSocket != FREERTOS_INVALID_SOCKET );


		if(cpmthread != pdPASS || thcpm == NULL)
		{
			if(!cpminq) cpminq = xQueueCreate(81, sizeof( CHAR));
			cpmthread = xTaskCreate( CPM22Task, "CPM22Task", configMINIMAL_STACK_SIZE*5, ram, 3,&thcpm);
			if(cpmthread != pdPASS)
				break;
			/* Send the welcome message. */
			iosize = FreeRTOS_send( xConnectedSocket,  ( void * ) pcWelcomeMessage,  strlen( pcWelcomeMessage ), 0 );
			xQueueReset(cpminq);
		}

		/* Process the socket as long as it remains connected. */
		do {
			char c;
			/* Receive data on the socket. */
			iosize = FreeRTOS_recv( xConnectedSocket, &c, 1, 0 );

			if( iosize > 0 )
			{
				xQueueSend(cpminq,&c,0);
			}
			else
				break;
		} while( iosize > 0 );
		/* Release the accepted child before accepting another connection. */
		FreeRTOS_closesocket(xConnectedSocket);
	}
	if(cpmthread == pdPASS)
		vTaskDelete( thcpm);
	
	FreeRTOS_closesocket(xConnectedSocket);
	FreeRTOS_closesocket(xListeningSocket);

}
#endif // ifdef CPM22
