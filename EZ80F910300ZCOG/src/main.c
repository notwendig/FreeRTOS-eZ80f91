/*
 * main.c
 *
 *  Created on: 13.08.2020
 *      Author: juergen
 */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "FreeRTOS_IP.h"
#include "console.h"
#include "tty.h"
#include "stdio.h"
#include "String.h"
#include "ez80_rtc.h"
#include "ez80_ntp.h"
#include "ez80_buttons.h"
#include "monitor.h"
#include "time.h"

#include "_ez80.h"
#include "uzlib.h"
//#include "CPM22_A.h"

#ifndef  _MULTI_THREAD
#error _MULTI_THREAD must by defined
#endif

static const uint8_t ucIPAddress[ 4 ] = { 0,0,0,0 };
static const uint8_t ucNetMask[ 4 ] = { 0,0,0,0 };
static const uint8_t ucGatewayAddress[ 4 ] = { 0,0,0,0 };
/* The following is the address of an OpenDNS server. */
static const uint8_t ucDNSServerAddress[ 4 ] = { 0,0,0,0 };
static const uint8_t ucMACAddress[ 6 ] = {0x00,0x90,0x23,0x00,0x01,0x02}; // 00:90:23:00:01:02
const char *hostname = "ez80";
//void vStartTCPCommandInterpreterTask( uint16_t usStackSize, uint32_t ulPort, UBaseType_t uxPriority );
//void vStartUDPCommandInterpreterTask( uint16_t usStackSize, uint32_t ulPort, UBaseType_t uxPriority );
//void vRegisterCLICommands( void );

//void loadcpm(char *mem);
extern uint8_t cpmram;


TaskHandle_t hndlA,hndlB,hndlC,hndlD,hndlCPM;
uint8_t volatile last=sizeof(size_t);

void prvTCPCpmIOTask( void *ram );

void PutchThread(void *arg)
{
	uint8_t c = (uint24_t) arg;
	UINT24  cnt = 0;
	while(1)
	{		
		portENTER_CRITICAL();
		cnt++;
		printf("\x1b[s\x1b[%d;0H %c %u   \x1b[u", c - 'A' + 2, c, cnt);
		portEXIT_CRITICAL();
		if(cnt > c)
			cnt = 0;
		vTaskDelay(pdMS_TO_TICKS(10*c)); 
	}
}

void StressThread(void *arg)
{	
	uint24_t c = (uint24_t) arg;
	uptime_t upHZ;
	unsigned h,m,s;
	
	while(1)
	{		
		upHZ = uptime( );       
		h = upHZ.sec / 3600;
		m = upHZ.sec % 3600 / 60;
		s = upHZ.sec % 60;		
		printf("\x1b[s\x1b[%d;0HUT %u:%u:%u %ums \n\x1b[u",c , h, m, s, upHZ.msec);
		vTaskDelay(pdMS_TO_TICKS(1000)); 
	}
	
}


#include "ez80_leds.h"
// Show a banner on the 5x7 LED Display
void TaskLED( void *pvParameters )
{
	#define LINESZ 120
    TickType_t ticks = (int)pvParameters;
	char *line5x7 = pvPortMalloc(LINESZ+1);
	dir_e dir = SHIFT_NONE;
	int idx;
	
	while(1)
    {
		LED5x7_putchar(dir,portMAX_DELAY);
		for(idx=' '; idx <= 0x7F; idx++)
		{
			LED5x7_putchar(idx, portMAX_DELAY);
		}
		
		if(line5x7)
		{
			strncpy(line5x7," * * * NadiSoft - ", LINESZ);
			getsDate(line5x7+strlen(line5x7),LINESZ - strlen(line5x7));
			strcat(line5x7," - ");
			getsTime(line5x7+strlen(line5x7),LINESZ - strlen(line5x7));
			idx = 0;
			while(idx < strlen(line5x7))
			{
				idx += LED5x7_puts(line5x7+idx,ticks);
			}
		}

		dir++;
		if(dir > SHIFT_DOWN)
			dir = SHIFT_NONE;
    }
}

int main(int argc, void *argv[])
{
	BaseType_t res;
	
	portSetup();
	
	TTYInit();
	puts("\033[2J\x1b[9;0H eZ80 tty");
#if 0
{
	size_t slen = cpm22img_length-4;
	size_t dlen = *(uint32_t*)((size_t) cpm22img + slen);
	uint8_t*  ramdisk = pvPortMalloc(dlen); 
	if(ramdisk)
	{
		int uz;
		memset(ramdisk,0xAA,dlen);
		uz = uzip(ramdisk,dlen,cpm22img,slen,NULL,0);
		if(TINF_DONE == uz)
			puts("OK");
		else
			puts("ERR");
	}							
}
#endif
	initRTC();
	initButtons();
	initLED5x7();

	res = FreeRTOS_IPInit( ucIPAddress, ucNetMask, ucGatewayAddress, ucDNSServerAddress, ucMACAddress );
	initConsole();
	
	res = xTaskCreate( TaskLED, "TaskLED", configMINIMAL_STACK_SIZE, (void *)portMAX_DELAY, 3, NULL);

//	vStartNTPTask( 2048, 4 );
	res = xTaskCreate(PutchThread, "PutchThreadA", 2048, (void*)'A', 2, &hndlA);
	res = xTaskCreate(PutchThread, "PutchThreadB", 2048, (void*)'B', 3, &hndlB);
//	vRegisterCLICommands();
//	vStartUDPCommandInterpreterTask( 1024, CGI_PORT, 3 );
	res = xTaskCreate( sysinfo, "SysInfo", configMINIMAL_STACK_SIZE*5, (void *)portMAX_DELAY, 3, NULL);


	res = xTaskCreate(StressThread, "StressThreadA", 2048, (void*)5, 3, &hndlC);
	res = xTaskCreate(StressThread, "StressThreadB", 2048, (void*)6, 4, &hndlD);
	res = xTaskCreate(prvTCPCpmIOTask,"CPM22",2048,&cpmram,4,&hndlCPM);

	vTaskStartScheduler();
 
	return res;
}
