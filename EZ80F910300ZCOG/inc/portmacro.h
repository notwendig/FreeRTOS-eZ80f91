/*
 * portmacro.h
 *
 *  Created on: 02.07.2020
 *      Author: juergen
 */

#ifndef INCLUDE_PORTMACRO_H_
#define INCLUDE_PORTMACRO_H_
#include <stdint.h>
#include <limits.h>
#include "portable.h"

typedef uint24_t 	StackType_t;
typedef int24_t		BaseType_t;
typedef uint24_t 	UBaseType_t;
typedef uint24_t 	TickType_t;

#define portBASE_TYPE BaseType_t
#define intmax_t long
int putch(int c);

typedef struct {
	TickType_t msec;
	TickType_t sec;
} uptime_t;

void portSetup();
uptime_t uptime();
void * pvPortMallocAligned( size_t xWantedSize, size_t alignment);
TickType_t get_sysuptimemsec();
TickType_t get_sysuptimesec();
int	intrap();
int clrtrap();
uint8_t* farptr(uint8_t *nearptr);	

#define interrupt 
#define nested_interrupt 
void* set_vector(unsigned int vector, void (*handler)(void));

#define InvalidInterruptHandler _default_mi_handler
#define portMAX_DELAY	UINT_MAX
#define portTICK_PERIOD_MS			( TickType_t )(1000LU / configTICK_RATE_HZ)
#define portBYTE_ALIGNMENT			1
#define portSTACK_GROWTH			-3

#define portPOINTER_SIZE_TYPE	unsigned

#define portINLINE
#define _static

int strcasecmp(const char* s1, const char* s2);
void vPortFree64k( void * pv );
void * pvPortMalloc64k();

void EnterCritical();
void ExitCritical();

#define portENABLE_INTERRUPTS() 	EI()
#define portDISABLE_INTERRUPTS() 	DI()

#define portENTER_CRITICAL()		EnterCritical()
#define portEXIT_CRITICAL()			ExitCritical()

#define portHAS_STACK_OVERFLOW_CHECKING		1

#define portTASK_FUNCTION( f, p ) void f(void *p)
#define portTASK_FUNCTION_PROTO( f, p ) void f(void *p)
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() 
#define portGET_RUN_TIME_COUNTER_VALUE() xTickCount

void vAssertCalled(int x, const char *file, int line);
void vPortYieldFromTick();
void vPortYield();
#define portYIELD() vPortYield()
void * _set_vector(unsigned int vector, void(*handler)(void));



#endif /* INCLUDE_PORTMACRO_H_ */
