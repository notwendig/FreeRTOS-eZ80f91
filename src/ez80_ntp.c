/* Standard includes. */
#include <stdint.h>
#include <stdlib.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* FreeRTOS+TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_IP_Private.h"
#include "FreeRTOS_DNS.h"
#include "FreeRTOS_Stream_Buffer.h"

#include "time.h"
#include "ez80_ntp.h"
#include "ez80_rtc.h"
void epoch_to_date_time(rtc_t* date_time, time_t epoch);

enum EStatus {
	EStatusSocket,
	EStatusLookup,
	EStatusAsking,
	EStatusPause,
	EStatusFailed,
};

//static struct SNtpPacket* xNTPPacket;

static char cRecvBuffer[ sizeof( struct SNtpPacket ) + 64 ];
 
static enum EStatus xStatus = EStatusLookup;

static const char *pcTimeServers[] = {
	"ptbtime1.ptb.de",
	"ptbtime2.ptb.de",
	"ptbtime3.ptb.de"
};

static SemaphoreHandle_t xNTPWakeupSem = NULL;
static uint32_t ulIPAddressFound;
static Socket_t xUDPSocket = NULL;
static TaskHandle_t xNTPTaskhandle = NULL;
static TickType_t uxSendTime;

static void prvNTPTask( void *pvParameters );

static void vSignalTask( void )
{
	if( xNTPWakeupSem != NULL )
	{
		xSemaphoreGive( xNTPWakeupSem );
	}
}

void vStartNTPTask( uint16_t usTaskStackSize, UBaseType_t uxTaskPriority )
{
	/* The only public function in this module: start a task to contact
	some NTP server. */

	if( xNTPTaskhandle != NULL )
	{
		switch( xStatus )
		{
		case EStatusPause:
			xStatus = EStatusAsking;
			vSignalTask();
			break;
		case EStatusLookup:
			FreeRTOS_printf( ( "NTP looking up server\n" ) );
			break;
		case EStatusAsking:
			FreeRTOS_printf( ( "NTP still asking\n" ) );
			break;
		case EStatusFailed:
			FreeRTOS_printf( ( "NTP failed somehow\n" ) );
			ulIPAddressFound = 0ul;
			if(xUDPSocket != NULL)
			{
				FreeRTOS_closesocket(xUDPSocket);
				xUDPSocket = NULL;
			}
			xStatus = EStatusSocket;
			vSignalTask();
			break;
		}
	}
	else
	{
		if(pdPASS != xTaskCreate(prvNTPTask, (const char *) "NTPD", usTaskStackSize, (void*)NTP_PORT, uxTaskPriority, &xNTPTaskhandle ))
		{
			FreeRTOS_printf( ( "Creating socket failed\n" ) );
		}
	}
}
/*-----------------------------------------------------------*/

static void prvSwapFields( struct SNtpPacket *pxPacket)
{
	/* NTP messages are big-endian */
	pxPacket->rootDelay = FreeRTOS_htonl( pxPacket->rootDelay );
	pxPacket->rootDispersion = FreeRTOS_htonl( pxPacket->rootDispersion );

	pxPacket->referenceTimestamp.seconds = FreeRTOS_htonl( pxPacket->referenceTimestamp.seconds );
	pxPacket->referenceTimestamp.fraction = FreeRTOS_htonl( pxPacket->referenceTimestamp.fraction );

	pxPacket->originateTimestamp.seconds = FreeRTOS_htonl( pxPacket->originateTimestamp.seconds );
	pxPacket->originateTimestamp.fraction = FreeRTOS_htonl( pxPacket->originateTimestamp.fraction );

	pxPacket->receiveTimestamp.seconds = FreeRTOS_htonl( pxPacket->receiveTimestamp.seconds );
	pxPacket->receiveTimestamp.fraction = FreeRTOS_htonl( pxPacket->receiveTimestamp.fraction );

	pxPacket->transmitTimestamp.seconds = FreeRTOS_htonl( pxPacket->transmitTimestamp.seconds );
	pxPacket->transmitTimestamp.fraction = FreeRTOS_htonl( pxPacket->transmitTimestamp.fraction );
}
/*-----------------------------------------------------------*/

static void prvNTPPacketInit(struct SNtpPacket* xNTPPacket )
{
	/* use the recorded NTP time */
	time_t uxSecs = FreeRTOS_time( NULL );/* apTime may be NULL, returns seconds */

	memset (xNTPPacket, '\0', sizeof( xNTPPacket ) );

	xNTPPacket->flags = 0xDB;				/* value 0xDB : mode 3 (client), version 3, leap indicator unknown 3 */
	xNTPPacket->poll = 10;					/* 10 means 1 << 10 = 1024 seconds */
	xNTPPacket->precision = 0xFA;			/* = 250 = 0.015625 seconds */
	xNTPPacket->rootDelay = 0x5D2E;			/* 0x5D2E = 23854 or (23854/65535)= 0.3640 sec */
	xNTPPacket->rootDispersion = 0x0008CAC8;	/* 0x0008CAC8 = 8.7912  seconds */

	xNTPPacket->referenceTimestamp.seconds = uxSecs;	/* Current time */
	xNTPPacket->transmitTimestamp.seconds = uxSecs;

	/* Transform the contents of the fields from native to big endian. */
	prvSwapFields( xNTPPacket );
}
/*-----------------------------------------------------------*/

static void prvReadTime( struct SNtpPacket * pxPacket )
{
	rtc_t  xTimeStruct,rtc;
	time_t uxPreviousSeconds;
	time_t uxPreviousMS;

	time_t uxCurrentSeconds;
	time_t uxCurrentMS;
	const char *pcTimeUnit;
	int32_t ilDiff;
	TickType_t uxTravelTime;
	time_t uxSecs = FreeRTOS_time( NULL );
	
	rtc.flg =
	xTimeStruct.flg =
			   (1 << RID_SEC)	// Real-Time Clock Seconds Register 
			  |(1 << RID_MIN)	// Real-Time Clock Minutes Register 
			  |(1 << RID_HRS)	// Real-Time Clock Hours Register   
			  |(1 << RID_DOW)	// Real-Time Clock Day-of-the-Week Register
			  |(1 << RID_DOM)	// Real-Time Clock Day-of-the-Month Register
			  |(1 << RID_MON)	// Real-Time Clock Month Register
			  |(1 << RID_YR)	// Real-Time Clock Year Register 
			  |(1 << RID_CEN);	// Real-Time Clock Century Register

	getRTC(&rtc);
	/*
	FreeRTOS_printf( ("RTC time: %d/%d/%02d %2d:%02d:%02d cen %d, dow %d, t %lu\n",
		rtc.d.r.DOM,
		rtc.d.r.MON,
		rtc.d.r.YR + 1970 + (rtc.d.r.CEN ? 100:0),
		rtc.d.r.HRS,
		rtc.d.r.MIN,
		rtc.d.r.SEC,
		rtc.d.r.CEN,
		rtc.d.r.DOM, uxSecs ) );
	*/
	uxTravelTime = xTaskGetTickCount() - uxSendTime;

	/* Transform the contents of the fields from big to native endian. */
	prvSwapFields( pxPacket );

	uxCurrentSeconds = pxPacket->receiveTimestamp.seconds - TIME1970;
	uxCurrentMS = pxPacket->receiveTimestamp.fraction / SECFRACTION;
	uxCurrentSeconds += uxCurrentMS / 1000;
	uxCurrentMS = uxCurrentMS % 1000;

	// Get the last time recorded
	uxPreviousSeconds = FreeRTOS_get_secs_msec( &uxPreviousMS );

	// Set the new time with precision in msec. */
	FreeRTOS_set_secs_msec( &uxCurrentSeconds, &uxCurrentMS );

	if( uxCurrentSeconds >= uxPreviousSeconds )
	{
		ilDiff = ( int32_t ) ( uxCurrentSeconds - uxPreviousSeconds );
	}
	else
	{
		ilDiff = 0 - ( int32_t ) ( uxPreviousSeconds - uxCurrentSeconds );
	}

	if( ( ilDiff < -5 ) || ( ilDiff > 5 ) )
	{
		/* More than 5 seconds difference. */
		pcTimeUnit = "sec";
		
	}
	else
	{
		/* Less than or equal to 5 second difference. */
		uint32_t ulLowest = ( uxCurrentSeconds <= uxPreviousSeconds ) ? uxCurrentSeconds : uxPreviousSeconds;
		int32_t iCurMS = 1000 * ( uxCurrentSeconds - ulLowest ) + uxCurrentMS;
		int32_t iPrevMS = 1000 * ( uxPreviousSeconds - ulLowest ) + uxPreviousMS;
		ilDiff = iCurMS - iPrevMS;
		pcTimeUnit = "ms";
	}
	uxCurrentSeconds -= iTimeZone;

	epoch_to_date_time(&xTimeStruct, uxCurrentSeconds);

	FreeRTOS_printf( ("NTP time: %d/%d/%02d %2d:%02d:%02d.%03u Diff %d %s (%lu ms), t %lu\n",
		xTimeStruct.d.r.DOM,
		xTimeStruct.d.r.MON,
		xTimeStruct.d.r.YR + 1970,
		xTimeStruct.d.r.HRS,
		xTimeStruct.d.r.MIN,
		xTimeStruct.d.r.SEC,
		( unsigned )uxCurrentMS,
		( unsigned )ilDiff,
		pcTimeUnit,
		uxTravelTime, uxCurrentSeconds) );

	if(xTimeStruct.d.r.SEC != rtc.d.r.SEC)
	{
		//xTimeStruct.d.r.HRS--;
		setRTC(&xTimeStruct);
		//FreeRTOS_printf( ("NTP => RTC\n") );
	}

/* Remove compiler warnings in case FreeRTOS_printf() is not used. */
	( void ) pcTimeUnit;
	( void ) uxTravelTime;
}
/*-----------------------------------------------------------*/
	static BaseType_t xOnUDPReceive( Socket_t xSocket, void * pvData, size_t xLength, const struct freertos_sockaddr *pxFrom, const struct freertos_sockaddr *pxDest )
	{
		if( xLength >= sizeof( struct SNtpPacket ) )
		{
			prvReadTime( ( struct SNtpPacket *)pvData );
			if( xStatus != EStatusPause )
			{
				xStatus = EStatusPause;
			}
		}
		vSignalTask();
		/* Tell the driver not to store the RX data */
		return 1;
	}
	/*-----------------------------------------------------------*/
static void prvNTPTask( void *pvParameters )
{
	uint16_t port = (uint16_t)pvParameters;
	BaseType_t xServerIndex = -1;
	struct freertos_sockaddr xAddress;
	F_TCP_UDP_Handler_t xHandler;

	xStatus = EStatusSocket;
	xNTPWakeupSem = xSemaphoreCreateBinary();
	
	memset( &xHandler, '\0', sizeof( xHandler ) );
	xHandler.pxOnUDPReceive = xOnUDPReceive;
	for( ; ; )
	{
		switch( xStatus )
		{
		case EStatusSocket:
   			if(FreeRTOS_IsNetworkUp())	
			{	
				xUDPSocket = FreeRTOS_socket( FREERTOS_AF_INET, FREERTOS_SOCK_DGRAM,FREERTOS_IPPROTO_UDP );
				if( xUDPSocket != FREERTOS_INVALID_SOCKET )
				{
					struct freertos_sockaddr xAddress;
					BaseType_t xReceiveTimeOut = pdMS_TO_TICKS( 2000 );
					xAddress.sin_addr = 0ul;
					xAddress.sin_port = FreeRTOS_htons( NTP_PORT );
		
					FreeRTOS_bind( xUDPSocket, &xAddress, sizeof( xAddress ) );
					FreeRTOS_setsockopt( xUDPSocket, 0, FREERTOS_SO_RCVTIMEO, &xReceiveTimeOut, sizeof( xReceiveTimeOut ) );
					FreeRTOS_setsockopt( xUDPSocket, 0, FREERTOS_SO_UDP_RECV_HANDLER, ( void * ) &xHandler, sizeof( xHandler ) );
					FreeRTOS_setsockopt( xUDPSocket, 0, FREERTOS_SO_SET_SEMAPHORE, ( void * ) &xNTPWakeupSem, sizeof( xNTPWakeupSem ) );
					xStatus = EStatusLookup;
				}
				else 
					xStatus = EStatusFailed;
			}
			break;
			
		case EStatusLookup:
			if( ++xServerIndex == sizeof( pcTimeServers ) / sizeof( pcTimeServers[ 0 ] ) )
				xServerIndex = 0;
				
			FreeRTOS_printf( ( "Looking up NTP-Server '%s'\n", pcTimeServers[ xServerIndex ] ) );
			ulIPAddressFound = FreeRTOS_gethostbyname( pcTimeServers[ xServerIndex ]);
			
			if(ulIPAddressFound == 0ul)
				break;
			xStatus = EStatusAsking;
			 
		case EStatusAsking:
			{
				
				struct SNtpPacket* xNTPPacket = (struct SNtpPacket*)FreeRTOS_GetUDPPayloadBuffer( sizeof(struct SNtpPacket), portMAX_DELAY );
				if(xNTPPacket)
				{
					int32_t res;
					prvNTPPacketInit(xNTPPacket );
					xAddress.sin_addr = ulIPAddressFound;
					xAddress.sin_port = FreeRTOS_htons( port );
					//char pcBuf[16];
					//FreeRTOS_inet_ntoa( xAddress.sin_addr, pcBuf );
					//FreeRTOS_printf( ( "Sending UDP message to %s:%u\n", pcBuf,	FreeRTOS_ntohs( xAddress.sin_port ) ) );
					uxSendTime = xTaskGetTickCount( );
					res = FreeRTOS_sendto( xUDPSocket, ( void * )xNTPPacket, sizeof( struct SNtpPacket ), FREERTOS_ZERO_COPY, &xAddress, sizeof( xAddress ) );
					if(res == 0)
						FreeRTOS_ReleaseUDPPayloadBuffer( ( void * )xNTPPacket);
				}
			}
			break;

		case EStatusPause:
				//FreeRTOS_printf( ( "NTPD wait\n") );
				xStatus = EStatusAsking;
			break;

		case EStatusFailed:
			FreeRTOS_printf( ( "NTPD failed\n") );
			break;
		}
		xSemaphoreTake( xNTPWakeupSem, 50000 );
	}
}
/*-----------------------------------------------------------*/

