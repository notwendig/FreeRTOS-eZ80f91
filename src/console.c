#include "FreeRTOS.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_TCP_server.h"
#include "queue.h"
#include "task.h"
#include "stream_buffer.h"
#include "console.h"
#include "tty.h"

#define CONSBUFSIZ 120

static Socket_t xSocketConsole;
static struct freertos_sockaddr xConsoleAddress;
static StreamBufferHandle_t cout;

int (*sysout)(int c) = tty_putch;

int putch(int c)
{
	int ret = sysout(c);
	return ret;
}

int console_putch(int c)
{
	int ret = xStreamBufferSend( cout, &c, 1, 10);
	return ret;
}

int console_getch()
{
	int ret = -1;
	return ret;
}

static Socket_t prvOpenTCPServerSocket( uint16_t usPort )
{
	struct freertos_sockaddr xBindAddress;
	Socket_t xSocket;

	static const TickType_t xReceiveTimeOut = portMAX_DELAY; // pdMS_TO_TICKS(1000);
	static const TickType_t xTransmitTimeOut = pdMS_TO_TICKS(100);
			
	const BaseType_t xBacklog = 20;
	BaseType_t xReuseSocket = pdTRUE;

	/* Attempt to open the socket. */
	xSocket = FreeRTOS_socket( FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP );
	configASSERT( xSocket != FREERTOS_INVALID_SOCKET );

	/* Set a time out so accept() will just wait for a connection. */
	FreeRTOS_setsockopt( xSocket, 0, FREERTOS_SO_RCVTIMEO, &xReceiveTimeOut, sizeof( xReceiveTimeOut ) );
	//FreeRTOS_setsockopt( xSocket, 0, FREERTOS_SO_SNDTIMEO, &xTransmitTimeOut, sizeof( xTransmitTimeOut ) );

	/* Only one connection will be used at a time, so re-use the listening
	socket as the connected socket.  See SimpleTCPEchoServer.c for an example
	that accepts multiple connections. */
	FreeRTOS_setsockopt( xSocket, 0, FREERTOS_SO_REUSE_LISTEN_SOCKET, &xReuseSocket, sizeof( xReuseSocket ) );

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

static void console_task(void *arg)
{
	char c;
	uint8_t *pucTCPPayloadBuffer = ( uint8_t * ) pvPortMalloc( CONSBUFSIZ);
	size_t n;
	struct freertos_sockaddr xClient;
	socklen_t xSize;
	Socket_t xConnectedSocket;
	BaseType_t iores;
	
	configASSERT(pucTCPPayloadBuffer);
	cout = xStreamBufferCreate(CONSBUFSIZ, CONSBUFSIZ - 20);
	configASSERT(cout );
	
	while(1)
	{	
		/* Create the socket. */
		xSocketConsole = prvOpenTCPServerSocket(CONSOLE_PORT);
		configASSERT( xSocketConsole != FREERTOS_INVALID_SOCKET );
		
		/* Wait for an incoming connection. */
		xSize = sizeof( xClient );
		xConnectedSocket = FreeRTOS_accept( xSocketConsole, &xClient, &xSize );
		configASSERT( xConnectedSocket == xSocketConsole );	
		
		sysout = console_putch;
		while(1)
		{
			n = xStreamBufferReceive(cout, pucTCPPayloadBuffer, CONSBUFSIZ, pdMS_TO_TICKS(200));
			if(n > 0)
			{
				iores = FreeRTOS_send( xConnectedSocket, pucTCPPayloadBuffer, n, 0);
				if(iores < 0)
					break;
			}
		}
		sysout = tty_putch;
		prvGracefulShutdown( xSocketConsole );
	}
}

BaseType_t initConsole()
{
	configASSERT(pdPASS == xTaskCreate( console_task, "console", configMINIMAL_STACK_SIZE*2, 0, 9, NULL));
	
	return 0;
}
