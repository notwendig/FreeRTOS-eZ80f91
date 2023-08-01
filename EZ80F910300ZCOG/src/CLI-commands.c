/*
    FreeRTOS V8.2.3 - Copyright (C) 2015 Real Time Engineers Ltd.
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>!AND MODIFIED BY!<< the FreeRTOS exception.

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

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"


/* FreeRTOS+CLI includes. */
#include "FreeRTOS_CLI.h"

/* FreeRTOS+TCP includes, just to make the stats available to the CLI
commands. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "console.h"
#include "ez80_rtc.h"

static int8_t *    	curraddr= 0;
static size_t		counter = 0;
	
static char *skipws(const char *s)
{
	while(*s && isspace(*s))
		s++;
	return s;
}

static int32_t getnum(const char **s)
{
	int32_t res = 0;
	int base = 10;
	int negative = 0;
	char *tmp = skipws(*s);
	
	if(*tmp == '-' || *tmp == '+')
	{
		negative = *tmp== '-';
		tmp = skipws(tmp+1);
	}
	
	if(*tmp == '0')
	{
		tmp++;
		base = 8;
		if(*tmp && tolower(*tmp) == 'x')
		{
			tmp++;
			base = 16;
		}
	}

	while(*tmp)
	{
		uint8_t c = toupper(*tmp);
		if(c >= '0')
		{
			c -= '0';
			if( c > 9)
				c -= 'A' - '9' - 1;
			
			if(c < base)
			{
				res *= base;
				res += c;
				tmp++;
				continue;
			}
		} 
		break;
	}
	
	*s = tmp;
	
	return negative ? -res:res;
}

static char* getaddrrange(const char *s, char **addr, uint24_t *len)
{
	char* tmp = s;
	int32_t n = getnum(&tmp);
	*len = 0;
	
	if(n >= 0 && n < 0x1000000)
	{
		*addr = (char*) n;
		if(*tmp)
		{
			*len = getnum(&tmp);
			if(*len < 0)
				*len = -*len - n;
		}
	}
	return tmp;
}

typedef enum 
	{
		DBYTE 	= 'B',
		DWORD16 = 'W',
		DWORD24 = 'D'
	} dumptfmt_t;

static BaseType_t prvDumpCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	static dumptfmt_t	dumpfmt = DBYTE;
	size_t n;

	int8_t pcnt=0;
	char ascii[25];
	
	char *pascii;
	
	if(!counter)
	{
		BaseType_t length;
		char *param = FreeRTOS_CLIGetParameter(pcCommandString, 1, &length);

		if(param && length==1)
		{
			int8_t x = toupper(*param);
			if(x == DBYTE || x == DWORD16 || x == DWORD24)
			{
				dumpfmt = (dumptfmt_t) x;
				param =  FreeRTOS_CLIGetParameter(pcCommandString, 2, &length);
			}
		}
		if(param)
		{
			param = skipws(getaddrrange(param, &curraddr, &counter));
			if(*param || !counter)
			{
				snprintf(pcWriteBuffer,xWriteBufferLen,ANSI_SATT(0,31,40)"wrong addres range. Use start ([+]count|-end).");
				return pdFALSE;
			}
		}
	}		

	pascii = ascii;
	switch(dumpfmt)
	{
		case DBYTE:
			pcnt = 16;
			break;
		case DWORD16:
			pcnt = 8;
			break;
		case DWORD24:
			pcnt = 5;
			break;
	}
	
	snprintf(pcWriteBuffer,xWriteBufferLen,ANSI_SATT(0,33,40)"%06X:" ANSI_SATT(0,32,40),curraddr);
	
	while(pcnt--)
	{
		n = strlen(pcWriteBuffer);
		if(!counter) 
			switch(dumpfmt)
			{
				case DBYTE:
					snprintf(pcWriteBuffer+n, xWriteBufferLen-n,"    "); 
					*pascii++ = ' ';
					break;
				case DWORD16:
					snprintf(pcWriteBuffer+n, xWriteBufferLen-n,"      "); 
					*pascii++ = ' ';
					*pascii++ = ' ';
					*pascii++ = ' ';
					break;
				case DWORD24:
					snprintf(pcWriteBuffer+n, xWriteBufferLen-n,"      "); 
					*pascii++ = ' ';
					*pascii++ = ' ';
					*pascii++ = ' ';
					*pascii++ = ' ';
					break;
			}
		else 
			switch(dumpfmt)
			{
				case DBYTE: snprintf(pcWriteBuffer+n, xWriteBufferLen-n," %02X", *(uint8_t*)curraddr); 
					*pascii++ = isprint(*curraddr) ? *curraddr:'.'; 
					curraddr++;
					counter--;
				break;
				case DWORD16:snprintf(pcWriteBuffer+n, xWriteBufferLen-n," %04X", *(UINT16*)curraddr);
					*pascii++ = isprint(*curraddr+1) ? *curraddr+1:'.'; 
					*pascii++ = isprint(*curraddr) ? *curraddr:'.'; 
					*pascii++ = '|';
					curraddr +=2;
					if(counter >= 2)
						counter -=2;
					else 
						counter = 0;
				break;
				case DWORD24:snprintf(pcWriteBuffer+n, xWriteBufferLen-n," %06X", *(UINT24*)curraddr); 
					*pascii++ = isprint(*curraddr+2) ? *curraddr+2:'.'; 
					*pascii++ = isprint(*curraddr+1) ? *curraddr+1:'.'; 
					*pascii++ = isprint(*curraddr) ? *curraddr:'.'; 
					*pascii++ = '|';
					curraddr +=3;
					if(counter >= 3)
						counter -=3;
					else 
						counter = 0;
				break;
			}
	}
	
	n = strlen(pcWriteBuffer);
	*pascii = 0;
	snprintf(pcWriteBuffer+n, xWriteBufferLen-n,ANSI_SATT(0,36,40)" |%s\n"ANSI_NORM,ascii);	
	return counter > 0 ? pdTRUE:pdFALSE;
}

static uint8_t hexbyte(uint8_t x)
{
	uint8_t n = x - '0';
	if(n > 9)
		n -= 7;
	return n;
}

static BaseType_t pvrIHex16Command( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	BaseType_t  i;
	uint8_t     chks;

	if(!counter)
	{
		char *param =  FreeRTOS_CLIGetParameter(pcCommandString, 1, &i);
		
		if(param)
		{
			uint24_t len;
			param = skipws(getaddrrange(param, &curraddr, &counter));
			
			snprintf(pcWriteBuffer, xWriteBufferLen, "# Intel hex from %p, %lu bytes.\n", curraddr, counter);
			if(*param)
				snprintf(pcWriteBuffer, xWriteBufferLen, "# Extra parameter ignored.\n");
		} 
		else
			snprintf(pcWriteBuffer, xWriteBufferLen, "# use ihex addr bytes.\n");
	} 
	else
	{
		UBaseType_t a = (UBaseType_t)curraddr >> 16; 
		UBaseType_t b = (UBaseType_t)curraddr & 0xFFFF;
		UBaseType_t l = (counter > 16) ? 16 : counter;		
		
		if(!b)
		{
			chks  = 0x02 + 0x04 + a;
			snprintf(pcWriteBuffer,xWriteBufferLen,":0200000400%02X%02X\n", a, (~chks +1) & 0xFF);
		}
		else
			*pcWriteBuffer = 0;
		
		if((b + l) > 0xFFFF)
			l = 16 - ( b & 0xF);
		
		snprintf(pcWriteBuffer+strlen(pcWriteBuffer),xWriteBufferLen-strlen(pcWriteBuffer),":%02X%04X00", l, b);
		chks = l + (b >> 8) + b;
		
		while( l--)
		{
			unsigned s = (UBaseType_t)*curraddr++ & 0xFF;
			snprintf(pcWriteBuffer+strlen(pcWriteBuffer),xWriteBufferLen-strlen(pcWriteBuffer),"%02X", s);
			chks += s;
			counter--;
		} 
		snprintf(pcWriteBuffer+strlen(pcWriteBuffer),xWriteBufferLen-strlen(pcWriteBuffer),"%02X\n",(UBaseType_t)(~chks +1) & 0xFF);
	}
	
	if(!counter)
		snprintf(pcWriteBuffer+strlen(pcWriteBuffer),xWriteBufferLen-strlen(pcWriteBuffer),":00000001FF\n");

	return counter? pdTRUE:pdFALSE;
}

static BaseType_t prvDateCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	BaseType_t length;
	char *param = FreeRTOS_CLIGetParameter(pcCommandString, 1, &length);
	int8_t *error;
	
	if(param)
	{
		uint8_t dow, day, mon;
		uint16_t year;
		
		dow = getnum(&param);
		if(*param != ' ' || dow < 1 || dow > 7 )
		{
			snprintf(pcWriteBuffer,xWriteBufferLen,ANSI_SATT(1,31,40)"Invalid day of week (1=Mo. - 7=Su.): %s", pcCommandString);
			return pdFALSE;
		}
		
		day = getnum(&param);
		if(*param != ' ' || day < 1 || day > 31 )
		{
			snprintf(pcWriteBuffer,xWriteBufferLen,ANSI_SATT(1,31,40)"Invalid day of month (1 - 31): %s", pcCommandString);
			return pdFALSE;
		}
		
		mon = getnum(&param);
		if(*param != ' ' || mon < 1 || mon > 12 )
		{
			snprintf(pcWriteBuffer,xWriteBufferLen,ANSI_SATT(1,31,40)"Invalid month (1 - 12): %s", pcCommandString);
			return pdFALSE;
		}
		
		year = getnum(&param);
		if(*param || year > 9999 )
		{
			snprintf(pcWriteBuffer,xWriteBufferLen,ANSI_SATT(1,31,40)"Invalid year: %s", pcCommandString);
			return pdFALSE;
		}
		
		setDate(dow, day, mon, year);
	}
	
	getsDate(pcWriteBuffer, xWriteBufferLen);
	return pdFALSE; 
}

static BaseType_t prvTimeCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	BaseType_t length;
	char *param = FreeRTOS_CLIGetParameter(pcCommandString, 1, &length);
	int8_t *error;
	
	if(param)
	{
		uint8_t hrs, min, sec;
		hrs = getnum(&param);
		if(*param != ' ' || hrs > 23 )
		{
			snprintf(pcWriteBuffer,xWriteBufferLen,ANSI_SATT(1,31,40)"Invalid hour ( 0 - 23): %s", pcCommandString);
			return pdFALSE;
		}
		
		min = getnum(&param);
		if(*param != ' ' || min > 59 )
		{
			snprintf(pcWriteBuffer,xWriteBufferLen,ANSI_SATT(1,31,40)"Invalid minute ( 0 - 59): %s", pcCommandString);
			return pdFALSE;
		}
		
		sec = getnum(&param);
		if(*param || sec > 59 )
		{
			snprintf(pcWriteBuffer,xWriteBufferLen,ANSI_SATT(1,31,40)"Invalid secound ( 0 - 59): %s", pcCommandString);
			return pdFALSE;
		}
		setTime(hrs, min, sec);
	}
	getsTime(pcWriteBuffer, xWriteBufferLen);
	return pdFALSE;
}

static const CLI_Command_Definition_t xMemoryDump =
	{
		"dump", /* The command string to type. */
		"dump [b|w|d] [start-adr] [count]\n\tDumps count items (b=byte, w=word16, l=word24) at startt-addr.\n",
		prvDumpCommand, /* The function to run. */
		-1 /* No parameters are expected. */
	};
		
static const CLI_Command_Definition_t xDate =
	{
		"date", /* The command string to type. */
		"date [day-of-week day mon year]\n\tOptional set and display RTC date.\n",
		prvDateCommand, /* The function to run. */
		-1 /* No parameters are expected. */
	};

static const CLI_Command_Definition_t xTime =
	{
		"time", /* The command string to type. */
		"time [ hour min sec]\n\tOptional set and display RTC time.\n",
		prvTimeCommand, /* The function to run. */
		-1 /* No parameters are expected. */
	};
	
static const CLI_Command_Definition_t xIHex16 =
	{
		"ihex", /* The command string to type. */
		"ihex start-addr end-addr"
		"Dump content in intel hex16 format.\n",
		pvrIHex16Command, /* The function to run. */
		-1 /* No parameters are expected. */
	};		
	

/*
 * Implements the echo-three-parameters command.
 */
static BaseType_t prvThreeParameterEchoCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );

/*
 * Implements the echo-parameters command.
 */
static BaseType_t prvParameterEchoCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );

/*
 * Defines a command that prints out IP address information.
 */
static BaseType_t prvDisplayIPConfig( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );

/*
 * Defines a command that sends an ICMP ping request to an IP address.
 */
static BaseType_t prvPingCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );

/*
 * Defines a command that calls FreeRTOS_netstat().
 */
static BaseType_t prvNetStatCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );

/* Structure that defines the "ip-config" command line command. */
static const CLI_Command_Definition_t xIPConfig =
{
	"ip-config",
	"ip-config:\r\n Displays IP address configuration\r\n\r\n",
	prvDisplayIPConfig,
	0
};

/* Structure that defines the "echo_3_parameters" command line command.  This
takes exactly three parameters that the command simply echos back one at a
time. */
static const CLI_Command_Definition_t xThreeParameterEcho =
{
	"echo-3-parameters",
	"echo-3-parameters <param1> <param2> <param3>:\r\n Expects three parameters, echos each in turn\r\n\r\n",
	prvThreeParameterEchoCommand, /* The function to run. */
	3 /* Three parameters are expected, which can take any value. */
};

/* Structure that defines the "echo_parameters" command line command.  This
takes a variable number of parameters that the command simply echos back one at
a time. */
static const CLI_Command_Definition_t xParameterEcho =
{
	"echo-parameters",
	"echo-parameters <...>:\r\n Take variable number of parameters, echos each in turn\r\n\r\n",
	prvParameterEchoCommand, /* The function to run. */
	-1 /* The user can enter any number of commands. */
};

#ifdef ipconfigUSE_TCP
	#if( ipconfigUSE_TCP == 1 )
		/* Structure that defines the "task-stats" command line command.  This generates
		a table that gives information on each task in the system. */
		static const CLI_Command_Definition_t xNetStats =
		{
			"net-stats", /* The command string to type. */
			"net-stats:\r\n Calls FreeRTOS_netstat()\r\n\r\n",
			prvNetStatCommand, /* The function to run. */
			0 /* No parameters are expected. */
		};
	#endif /* ipconfigUSE_TCP == 1 */
#endif /* ifdef ipconfigUSE_TCP */

#if ipconfigSUPPORT_OUTGOING_PINGS == 1

	/* Structure that defines the "ping" command line command.  This takes an IP
	address or host name and (optionally) the number of bytes to ping as
	parameters. */
	static const CLI_Command_Definition_t xPing =
	{
		"ping",
		"ping <ipaddress> <optional:bytes to send>:\r\n for example, ping 192.168.0.3 8, or ping www.example.com\r\n\r\n",
		prvPingCommand, /* The function to run. */
		-1 /* Ping can take either one or two parameter, so the number of parameters has to be determined by the ping command implementation. */
	};

#endif /* ipconfigSUPPORT_OUTGOING_PINGS */
/*-----------------------------------------------------------*/

void vRegisterCLICommands( void )
{
static BaseType_t xCommandRegistered = pdFALSE;

	/* Prevent commands being registered more than once. */
	if( xCommandRegistered == pdFALSE )
	{
		
		FreeRTOS_CLIRegisterCommand( &xMemoryDump );
		FreeRTOS_CLIRegisterCommand( &xIHex16);
		FreeRTOS_CLIRegisterCommand( &xDate );
		FreeRTOS_CLIRegisterCommand( &xTime );


		FreeRTOS_CLIRegisterCommand( &xThreeParameterEcho );
		FreeRTOS_CLIRegisterCommand( &xParameterEcho );
		FreeRTOS_CLIRegisterCommand( &xIPConfig );

		#if ipconfigSUPPORT_OUTGOING_PINGS == 1
		{
			FreeRTOS_CLIRegisterCommand( &xPing );
		}
		#endif /* ipconfigSUPPORT_OUTGOING_PINGS */

		#ifdef ipconfigUSE_TCP
		{
			#if ipconfigUSE_TCP == 1
			{
				FreeRTOS_CLIRegisterCommand( &xNetStats );
			}
			#endif /* ipconfigUSE_TCP == 1 */
		}
		#endif /* ifdef ipconfigUSE_TCP */

		#if configINCLUDE_TRACE_RELATED_CLI_COMMANDS == 1
		{
			FreeRTOS_CLIRegisterCommand( & xStartStopTrace );
		}
		#endif

		xCommandRegistered = pdTRUE;
	}
}

static BaseType_t prvThreeParameterEchoCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
const char *pcParameter;
BaseType_t xParameterStringLength, xReturn;
static BaseType_t lParameterNumber = 0;

	/* Remove compile time warnings about unused parameters, and check the
	write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	write buffer length is adequate, so does not check for buffer overflows. */
	( void ) pcCommandString;
	configASSERT( pcWriteBuffer );

	if( lParameterNumber == 0 )
	{
		/* The first time the function is called after the command has been
		entered just a header string is returned. */
		snprintf( pcWriteBuffer, xWriteBufferLen, "The three parameters were:\r\n" );

		/* Next time the function is called the first parameter will be echoed
		back. */
		lParameterNumber = 1L;

		/* There is more data to be returned as no parameters have been echoed
		back yet. */
		xReturn = pdPASS;
	}
	else
	{
		/* Obtain the parameter string. */
		pcParameter = FreeRTOS_CLIGetParameter
						(
							pcCommandString,		/* The command string itself. */
							lParameterNumber,		/* Return the next parameter. */
							&xParameterStringLength	/* Store the parameter string length. */
						);

		/* Sanity check something was returned. */
		configASSERT( pcParameter );

		/* Return the parameter string. */
		memset( pcWriteBuffer, 0x00, xWriteBufferLen );
		snprintf( pcWriteBuffer, xWriteBufferLen, "%d: ", ( int ) lParameterNumber );
		strncat( pcWriteBuffer, pcParameter, xParameterStringLength );
		strncat( pcWriteBuffer, "\r\n", strlen( "\r\n" ) );

		/* If this is the last of the three parameters then there are no more
		strings to return after this one. */
		if( lParameterNumber == 3L )
		{
			/* If this is the last of the three parameters then there are no more
			strings to return after this one. */
			xReturn = pdFALSE;
			lParameterNumber = 0L;
		}
		else
		{
			/* There are more parameters to return after this one. */
			xReturn = pdTRUE;
			lParameterNumber++;
		}
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

static BaseType_t prvParameterEchoCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
const char *pcParameter;
BaseType_t xParameterStringLength, xReturn;
static BaseType_t lParameterNumber = 0;

	/* Remove compile time warnings about unused parameters, and check the
	write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	write buffer length is adequate, so does not check for buffer overflows. */
	( void ) pcCommandString;
	configASSERT( pcWriteBuffer );

	if( lParameterNumber == 0 )
	{
		/* The first time the function is called after the command has been
		entered just a header string is returned. */
		snprintf( pcWriteBuffer, xWriteBufferLen, "The parameters were:\r\n" );

		/* Next time the function is called the first parameter will be echoed
		back. */
		lParameterNumber = 1L;

		/* There is more data to be returned as no parameters have been echoed
		back yet. */
		xReturn = pdPASS;
	}
	else
	{
		/* Obtain the parameter string. */
		pcParameter = FreeRTOS_CLIGetParameter
						(
							pcCommandString,		/* The command string itself. */
							lParameterNumber,		/* Return the next parameter. */
							&xParameterStringLength	/* Store the parameter string length. */
						);

		if( pcParameter != NULL )
		{
			/* Return the parameter string. */
			memset( pcWriteBuffer, 0x00, xWriteBufferLen );
			snprintf( pcWriteBuffer, xWriteBufferLen, "%d: ", ( int ) lParameterNumber );
			strncat( pcWriteBuffer, pcParameter, xParameterStringLength );
			strncat( pcWriteBuffer, "\r\n", strlen( "\r\n" ) );

			/* There might be more parameters to return after this one. */
			xReturn = pdTRUE;
			lParameterNumber++;
		}
		else
		{
			/* No more parameters were found.  Make sure the write buffer does
			not contain a valid string. */
			pcWriteBuffer[ 0 ] = 0x00;

			/* No more data to return. */
			xReturn = pdFALSE;

			/* Start over the next time this command is executed. */
			lParameterNumber = 0;
		}
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

#ifdef ipconfigUSE_TCP

	#if ipconfigUSE_TCP == 1

		static BaseType_t prvNetStatCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
		{
			( void ) pcWriteBuffer;
			( void ) xWriteBufferLen;
			( void ) pcCommandString;

			FreeRTOS_netstat();
			snprintf( pcWriteBuffer, xWriteBufferLen, "FreeRTOS_netstat() called - output uses FreeRTOS_printf\r\n" );
			return pdFALSE;
		}

	#endif /* ipconfigUSE_TCP == 1 */

#endif /* ifdef ipconfigUSE_TCP */
/*-----------------------------------------------------------*/

#if ipconfigSUPPORT_OUTGOING_PINGS == 1

	static BaseType_t prvPingCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
	{
	char * pcParameter;
	BaseType_t lParameterStringLength, xReturn;
	uint32_t ulIPAddress, ulBytesToPing;
	const uint32_t ulDefaultBytesToPing = 8UL;
	char cBuffer[ 16 ];

		/* Remove compile time warnings about unused parameters, and check the
		write buffer is not NULL.  NOTE - for simplicity, this example assumes the
		write buffer length is adequate, so does not check for buffer overflows. */
		( void ) pcCommandString;
		configASSERT( pcWriteBuffer );

		/* Start with an empty string. */
		pcWriteBuffer[ 0 ] = 0x00;

		/* Obtain the number of bytes to ping. */
		pcParameter = ( char * ) FreeRTOS_CLIGetParameter
								(
									pcCommandString,		/* The command string itself. */
									2,						/* Return the second parameter. */
									&lParameterStringLength	/* Store the parameter string length. */
								);

		if( pcParameter == NULL )
		{
			/* The number of bytes was not specified, so default it. */
			ulBytesToPing = ulDefaultBytesToPing;
		}
		else
		{
			ulBytesToPing = atol( pcParameter );
		}

		/* Obtain the IP address string. */
		pcParameter = ( char * ) FreeRTOS_CLIGetParameter
								(
									pcCommandString,		/* The command string itself. */
									1,						/* Return the first parameter. */
									&lParameterStringLength	/* Store the parameter string length. */
								);

		/* Sanity check something was returned. */
		configASSERT( pcParameter );

		/* Attempt to obtain the IP address.   If the first character is not a
		digit, assume the host name has been passed in. */
		if( ( *pcParameter >= '0' ) && ( *pcParameter <= '9' ) )
		{
			ulIPAddress = FreeRTOS_inet_addr( pcParameter );
		}
		else
		{
			/* Terminate the host name. */
			pcParameter[ lParameterStringLength ] = 0x00;

			/* Attempt to resolve host. */
			ulIPAddress = FreeRTOS_gethostbyname( pcParameter );
		}

		/* Convert IP address, which may have come from a DNS lookup, to string. */
		FreeRTOS_inet_ntoa( ulIPAddress, cBuffer );

		if( ulIPAddress != 0 )
		{
			xReturn = FreeRTOS_SendPingRequest( ulIPAddress, ( uint16_t ) ulBytesToPing, portMAX_DELAY );
		}
		else
		{
			xReturn = pdFALSE;
		}

		if( xReturn == pdFALSE )
		{
			snprintf( pcWriteBuffer, xWriteBufferLen, "%s", "Could not send ping request\r\n" );
		}
		else
		{
			snprintf( pcWriteBuffer, xWriteBufferLen, "Ping sent to %s with identifier %d\r\n", cBuffer, xReturn );
		}

		return pdFALSE;
	}
	/*-----------------------------------------------------------*/

#endif /* ipconfigSUPPORT_OUTGOING_PINGS */

static BaseType_t prvDisplayIPConfig( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
static BaseType_t xIndex = 0;
BaseType_t xReturn;
uint32_t ulAddress;

	/* Remove compile time warnings about unused parameters, and check the
	write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	write buffer length is adequate, so does not check for buffer overflows. */
	( void ) pcCommandString;
	configASSERT( pcWriteBuffer );

	switch( xIndex )
	{
		case 0 :
			FreeRTOS_GetAddressConfiguration( &ulAddress, NULL, NULL, NULL );
			snprintf( pcWriteBuffer, xWriteBufferLen, "\r\nIP address " );
			xReturn = pdTRUE;
			xIndex++;
			break;

		case 1 :
			FreeRTOS_GetAddressConfiguration( NULL, &ulAddress, NULL, NULL );
			snprintf( pcWriteBuffer, xWriteBufferLen, "\r\nNet mask " );
			xReturn = pdTRUE;
			xIndex++;
			break;

		case 2 :
			FreeRTOS_GetAddressConfiguration( NULL, NULL, &ulAddress, NULL );
			snprintf( pcWriteBuffer, xWriteBufferLen, "\r\nGateway address " );
			xReturn = pdTRUE;
			xIndex++;
			break;

		case 3 :
			FreeRTOS_GetAddressConfiguration( NULL, NULL, NULL, &ulAddress );
			snprintf( pcWriteBuffer, xWriteBufferLen, "\r\nDNS server address " );
			xReturn = pdTRUE;
			xIndex++;
			break;

		default :
			ulAddress = 0;
			snprintf( pcWriteBuffer, xWriteBufferLen, "\r\n\r\n" );
			xReturn = pdFALSE;
			xIndex = 0;
			break;
	}

	if( ulAddress != 0 )
	{
		FreeRTOS_inet_ntoa( ulAddress,  &( pcWriteBuffer[ strlen( pcWriteBuffer ) ] ) );
	}

	return xReturn;
}
/*-----------------------------------------------------------*/
