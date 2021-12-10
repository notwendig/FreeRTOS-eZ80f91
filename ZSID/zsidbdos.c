/*
 * zsidbdos.c
 *
 *  Created on: 10.12.2021
 *      Author: juergen
 */
#include <stdint.h>
#include <Stdio.h>
#include "zsidbdos.h"
#include "exbios.h"

#define CNTRLC	0x03		//control-c
#define CNTRLE	0x05		//control-e
#define BELL	0x07
#define BS		0x08		//backspace
#define TAB		0x09		//tab
#define LF		0x0A		//line feed
#define FF		0x0C		//form feed
#define CR		0x0D		//carriage return
#define CNTRLP	0x10		//control-p
#define CNTRLR	0x12		//control-r
#define CNTRLS	0x13		//control-s
#define CNTRLU	0x15		//control-u
#define CNTRLX	0x18		//control-x
#define CNTRLZ	0x1A		//control-z (end-of-file mark)
#define DEL		0x7F		//rubout

//
//	I/O ports
//
#define MONITOR	0x30

#define CONIO	0x31		// CONOLE, PRT, AUX Device
#define CONSTA	0x01		//console status port
#define CONDAT	0x02		//console data port
#define PRTSTA	0x03		//printer status port
#define PRTDAT	0x04		//printer data port
#define AUXDAT	0x05		//auxiliary data port

#define FDIO	0x32		// FLOPPY Device
#define FDCD	0x01		//fdc-port: # of drive in A. 0 = A, 1 = B ... HL points to DPH
#define FDCTBC	0x02		//fdc-port: # of track in BC
#define FDCSBC	0x03		//fdc-port: # of sector in BC
#define FDCOP	0x04		//fdc-port: command	A == 0 write, 1 read
#define FDCST	0x05		//fdc-port: status	Disk Status to A

#define DMAIO	0x33		// DMA Device
#define DMABC	0x01		//dma-port: dma address BC

#define ROMBOOT 0x34		// boot buildin CP/M 2.2

#define PORTOUT	0
#define PORTINP	1


#define N_BDOS	41

typedef uint8_t* (*bdoscall)(trapframe_t *);

static uint8_t* notsup(trapframe_t *context)
{
	context->af |= 0xFF00;
	return *context->pc;
}

static uint8_t* constat(trapframe_t *context)
{
 	return ConsoleIO(context, PORTINP, CONSTA);
}

static uint8_t* conin(trapframe_t *context)
{
	ConsoleIO(context, PORTINP, CONDAT);
	return ConsoleIO(context, PORTOUT, CONDAT);
}

static uint8_t* conout(trapframe_t *context)
{
	context->af = (context->af & 0xFF) | (context->de << 8);
	return ConsoleIO(context, PORTOUT, CONDAT);
}

static uint8_t* rdline(trapframe_t *context)     
{
	uint8_t* buf = farptr((uint8_t*)context->de);
	buf[1] = 0;
	while(buf[0] > buf[1])
	{
		uint8_t c;
		ConsoleIO(context, PORTINP, CONDAT);
		c = context->af >> 8;
		if(c == CR || c == LF)
			break;
		if(c == BS)
		{
			if(buf[1])
				buf[1]--;
			else
				context->af = (context->af & 0xFF) | (BELL << 8);
		}
		else
		{
			buf[2+buf[1]] = c;
			buf[1]++;
		}
		ConsoleIO(context, PORTOUT, CONDAT);
	}
	return *context->pc;
}

static const bdoscall fbdos[N_BDOS] = {
	notsup,  // 0	System Reset
	conin ,  // 1	Console Input
	conout,  // 2	Console Output
	notsup,  // 3	Reader Input
	notsup,  // 4	Punch Output
	notsup,  // 5	List Output
	notsup,  // 6	Direct Console I/O
	notsup,  // 7	Get I/O Byte
	notsup,  // 8	Set I/O Byte
	notsup,  // 9	Print String
	rdline,  // 10	Read Console Buffer
	constat, // 11	Get Console Status
	notsup,  // 12	Return Version Number
	notsup,  // 13	Reset Disk System
	notsup,  // 14	Select Disk
	notsup,  // 15	Open File
	notsup,  // 16	Close File
	notsup,  // 17	Search for First
	notsup,  // 18	Search for Next
	notsup,  // 19	Delete File
	notsup,  // 20	Read Sequential
	notsup,  // 21	Write Sequential
	notsup,  // 22	Make File
	notsup,  // 23	Rename File
	notsup,  // 24	Return Login Vector
	notsup,  // 25	Return Current Disk
	notsup,  // 26	Set DMA Address
	notsup,  // 27	Get Addr(Alloc)
	notsup,  // 28	Write Protect Disk
	notsup,  // 29	Get R/O Vector
	notsup,  // 30	Set File Attributes
	notsup,  // 31	Get Addr(Disk Parms)
	notsup,  // 32	Set/Get User Code
	notsup,  // 33	Read Random
	notsup,  // 34	Write Random
	notsup,  // 35	Compute File Size
	notsup,  // 36	Set Random Record
	notsup,  // 37	Reset Drive
	notsup   // 40	Write Random with Zero Fill
};


uint8_t *bdos(trapframe_t *context)
{
	unsigned idx = (unsigned)(context->bc & 0xff);
	uint8_t *res;
	printf("BDOS %d\n",idx);
	if(idx >= N_BDOS)
		res = notsup(context);
	else
		res = fbdos[idx](context);
	return res;
}