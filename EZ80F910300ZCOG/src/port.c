/*
 * port.c
 *
 *  Created on: 10.07.2020
 *      Author: juergen
 */
#include "FreeRTOS.h"
#include "FreeRTOS_IP.h"
#include "portable.h"
#include "task.h"
#include "semphr.h"
#include "uzlib.h"
#include <eZ80F91.h>
#include <stdlib.h>
#include <Stdio.h>
#include <Stdarg.h>

#ifndef _DEBUG
#error Release not supported yet
#endif

//uint8_t ucHeap[configTOTAL_HEAP_SIZE] _At 0xB80000;
HeapRegion_t xHeapRegions[] =
{
  { 0, 0 }, 
  { NULL, 0 }
};

void portSetup()
{
	xHeapRegions[0].pucStartAddress = &_heapbot;
	xHeapRegions[0].xSizeInBytes = (unsigned)&_heaptop - (unsigned)&_heapbot;
	vPortDefineHeapRegions(xHeapRegions);
	
	uzlib_init();
 
}

int tolower(int c);
const unsigned ticks = (configCPU_CLOCK_HZ / 16 / configTICK_RATE_HZ);

extern void SIG_OUTPUT_COMPARE1A();
extern void firstTASK();

volatile uint8_t criticalcounter = 0;

extern volatile TickType_t xTickCount;
extern volatile TickType_t xTickCountHeigh;

TickType_t get_sysuptimemsec()
{
	return xTickCount%1000;
}

TickType_t get_sysuptimesec()
{
	return xTickCountHeigh+xTickCount/1000;
}

void EnterCritical()
{
	DI();
	++criticalcounter;
	configASSERT(criticalcounter);
}

void ExitCritical()
{
	if(criticalcounter && !--criticalcounter)
		EI();
}

uptime_t uptime()
{
	uptime_t t;
	EnterCritical();
	t.msec = get_sysuptimemsec();
	t.sec = get_sysuptimesec();
	ExitCritical();
	return t;
}
/*-----------------------------------------------------------*/
static void preTask()
{
	asm(" ei");
}

void free(void * ptr)
{
	vPortFree(ptr);
}

void * malloc(size_t size)
{
	return pvPortMalloc(size);
}

void * calloc(int num, size_t size)
{
	void *tmp = pvPortMalloc(num*size);
	if(tmp)
	{
		memset(tmp,0,num*size);
	}
	return tmp;
}

void * realloc(void *old, size_t size)
{
	extern const size_t xHeapStructSize;
	const size_t *csz = old  - xHeapStructSize + sizeof(void*);
	void *tmp;
	
	if(!old)
		return pvPortMalloc(size);
	
	if(!size)
	{
		vPortFree(old);
		return NULL;
	}
	
	if(*csz <= size)
		return old;
	
	tmp = pvPortMalloc(size);
	if(tmp)
	{
		memcpy(tmp,old,size);
		vPortFree(old);
	}
	return tmp;
}

static uint24_t mbase()
{
	uint24_t mb=0;
	asm(" ld a,mb");
	asm(" ld (ix-1),a");
	return mb;
}


uint8_t* farptr(uint8_t *nearptr)
{
	return (uint8_t*) ((uint24_t) nearptr & 0xFFFF | mbase());
}
/*
 * Setup valid stack-frame for a new task and return top of frame.
 */
 #if ( portHAS_STACK_OVERFLOW_CHECKING == 1 )
        StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                             StackType_t * pxEndOfStack,
                                             TaskFunction_t pxCode,
                                             void * pvParameters ) PRIVILEGED_FUNCTION
#else
        StackType_t * pxPortInitialiseStack( StackType_t * pxTopOfStack,
                                             TaskFunction_t pxCode,
                                             void * pvParameters ) PRIVILEGED_FUNCTION
#endif
{
    /* Place the parameter on the stack in the expected location. */
    *--pxTopOfStack = ( StackType_t ) pvParameters;

    /* Place the task return address on stack. Not used
	   if you're wanting to return from tasks
	   enable the following lines AND
	   fix pcParameters access on each task funtion!!
	*/
    *--pxTopOfStack = ( StackType_t ) 0;
	
    /* The start of the task code will be popped off the stack last, so place
    it on first. */
    *--pxTopOfStack = ( StackType_t ) pxCode;
	*--pxTopOfStack = ( StackType_t ) preTask;
	*--pxTopOfStack = ( StackType_t ) 0;		 /* criticalcounter */
#if ( portHAS_STACK_OVERFLOW_CHECKING == 1 )
	configASSERT( ( pxTopOfStack >= pxEndOfStack ));
#endif	
	return pxTopOfStack;
}

BaseType_t xPortStartScheduler ()
{
	uint8_t th = ticks >> 8;
	uint8_t tl = ticks;
	DI();
	TMR0_IER = 0;
	_set_vector(TIMER0_IVECT, vPortYieldFromTick);
	
	TMR0_DR_H= th;
	TMR0_DR_L= tl;
	TMR0_RR_H= th;
	TMR0_RR_L= tl;
	
	TMR0_CTL = 0x8F;// 1 00 01 1 1 1
	TMR0_IER = 1;
	EI();
	firstTASK();
	configASSERT(0);
	return 0;
}

void vPortEndScheduler ()
{
	DI();
	TMR0_IER = 0;
	asm("slp");
}

static StaticTask_t TimerTask_tcb;
static StackType_t  TimerTask_stk[1024];

void vApplicationGetTimerTaskMemory(  StaticTask_t **pxTimerTaskTCBBuffer,  StackType_t **pxTimerTaskStackBuffer, uint32_t *ulTimerTaskStackSize )
{	
	*pxTimerTaskTCBBuffer =  &TimerTask_tcb;
	*pxTimerTaskStackBuffer =  &TimerTask_stk;
	*ulTimerTaskStackSize  = sizeof(TimerTask_stk);
}

	static StaticTask_t IdleTask_tcb;
	static StackType_t  IdleTask_stk[1024];
	
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize ) /*lint !e526 Symbol not defined as it is an application callback. */
{
	*ppxIdleTaskTCBBuffer =  &IdleTask_tcb;
	*ppxIdleTaskStackBuffer =  &IdleTask_stk;
	*pulIdleTaskStackSize = sizeof(IdleTask_stk);
}


void vAssertCalled (int x, const char *file, int line)
{
	if(!x)
	{
		char buf[128];
		char *tmp = buf;

		TMR0_IER = 0;
		sprintf(buf,"ASSERT: %s:%d\n", file, line);
			
		while(*tmp)
			putchar(*tmp++);
		
		while(!x)
			asm(" nop");
		TMR0_IER = 1;
	}
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
	vAssertCalled(1,pcTaskName, 0);
}

void vApplicationMallocFailedHook()
{
	vAssertCalled(1,"malloc", 0);
}

static long ulNextRand = 0x12345678;

long uxRand( void )
{
	static const long ulMultiplier = 0x375a4e35L; 
	static const long ulIncrement  = 7L;

	/* Utility function to generate a pseudo random number. */
	ulNextRand = ulMultiplier * ulNextRand + ulIncrement;
	return ulNextRand;
}

BaseType_t xApplicationGetRandomNumber( uint32_t *pulValue )
{
	return uxRand();
}

uint32_t ulApplicationGetNextSequenceNumber( uint32_t ulSourceAddress, uint16_t usSourcePort, uint32_t ulDestinationAddress, uint16_t usDestinationPort )
{
     ( void ) ulSourceAddress;
     ( void ) usSourcePort;
     ( void ) ulDestinationAddress;
     ( void ) usDestinationPort;

     return uxRand();
}

int strcasecmp(const char* s1, const char* s2)
{
	int res = 0;
	
	while(*s1 && *s2 && !res)
		res = tolower(*s1++) - tolower(*s2++);
	if(!res)
		res = *s1 - *s2;
	return res;
}

BaseType_t xApplicationDNSQueryHook( const char *pcName )
{
	return pdTRUE;
}

void vApplicationPingReplyHook(ePingReplyStatus_t eStatus, uint16_t usIdentifier )
{
	return;
}

const char *pcApplicationHostnameHook()
{
	extern const char *hostname;
	return hostname;
}



void vApplicationIdleHook( void )
{

}


