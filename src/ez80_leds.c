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
	
	
	File: 	ez80_led.c
			plattform 5x7 LED Matrx driver
			Part of FreeRTOS Port for the eZ80F91 Development Kit eZ80F910300ZCOG
			See www.zilog.com for desciption.


	Developer:
	JSIE	 Juergen Sievers <JSievers@NadiSoft.de>

	150804:	JSIE Start this port.
	
*/

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "ez80_leds.h"
#include <eZ80F91.h>

#include <String.h>

extern void shiftDisplay();
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* LED anode and cathode memory maped I/O pointers */
#define LEDMATRIX_ANODE   (*(volatile unsigned char*)0x800000)  //Anode
#define LEDMATRIX_CATHODE (*(volatile unsigned char*)0x800001)  //Cathode


/****************************************************************************
 * Private Types
 ****************************************************************************/
	
/****************************************************************************
 * Private Data
 ****************************************************************************/

static TaskHandle_t LEDTaskHandle;
unsigned ledticks = (configCPU_CLOCK_HZ / 16 / configLED5x7TICK_RATE_HZ);
	
/* The current selected glyph 7 rows*/
static volatile CHAR *currglyph;

static UINT8  frames;	// display refresch delay 
static UINT16 chardly;	// delay between letters
static uint8_t shift_width = 6;
/* Display character queue	*/
static QueueHandle_t xLED5x7Queue;

/* current shift direction */
uint8_t dir;

/* display shift buffer */
volatile uint16_t dply[8] = {0xAAAA, 0xAAAA, 0xAAAA, 0xAAAA, 0xAAAA, 0xAAAA, 0xAAAA, 0xAAAA};

static uint8_t shiftdly = configTICK_RATE_HZ / LED5x7_SHIFTT / 7 +30;
static uint8_t row=0;
static const uint8_t col[7] = {0x81, 0x82, 0x84, 0x88, 0x90, 0xA0, 0xC0};
static const uint8_t width[]= {6,6,8,6,8};

/* 5x7 LED matrix character glyphs.  Each glyph consists of 7 bytes, one
 * each row and each containing 5 bits of data, one for each column
 */
static const CHAR cmatrix[96][7] = {			 // hex- ascii
	{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},  // 20 - space
	{0xfb, 0xfb, 0xfb, 0xfb, 0xfb, 0xff, 0xfb},  // 21 - !
	{0xf5, 0xf5, 0xf5, 0xff, 0xff, 0xff, 0xff},  // 22 - "
	{0xff, 0xf5, 0xe0, 0xf5, 0xe0, 0xf5, 0xff},  // 23 - #
	{0xfb, 0xf1, 0xea, 0xf1, 0xea, 0xf1, 0xfb},  // 24 - $
	{0xff, 0xfe, 0xf5, 0xfb, 0xf5, 0xef, 0xff},  // 25 - %
	{0xf1, 0xee, 0xee, 0xf1, 0xf5, 0xee, 0xf0},  // 26 - &
	{0xfb, 0xfb, 0xfb, 0xff, 0xff, 0xff, 0xff},  // 27 - '
	{0xfd, 0xfb, 0xf7, 0xf7, 0xf7, 0xfb, 0xfd},  // 28 - (
	{0xf7, 0xfb, 0xfd, 0xfd, 0xfd, 0xfb, 0xf7},  // 29 - )
	{0xff, 0xea, 0xf1, 0xe0, 0xf1, 0xea, 0xff},  // 2a - *
	{0xff, 0xfb, 0xfb, 0xe0, 0xfb, 0xfb, 0xff},  // 2b - +
	{0xff, 0xff, 0xff, 0xff, 0xff, 0xfb, 0xf7},  // 2c - ,
	{0xff, 0xff, 0xff, 0xe0, 0xff, 0xff, 0xff},  // 2d - -
	{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfb},  // 2e - .
	{0xff, 0xfe, 0xfd, 0xfb, 0xf7, 0xef, 0xff},  // 2f - /
	{0xf1, 0xee, 0xec, 0xea, 0xe6, 0xee, 0xf1},  // 30 - 0
	{0xfb, 0xf3, 0xfb, 0xfb, 0xfb, 0xfb, 0xf1},  // 31 - 1
	{0xf1, 0xee, 0xfd, 0xfb, 0xf7, 0xef, 0xe0},  // 32 - 2
	{0xf1, 0xee, 0xfe, 0xf9, 0xfe, 0xee, 0xf1},  // 33 - 3
	{0xee, 0xee, 0xee, 0xf0, 0xfe, 0xfe, 0xfe},  // 34 - 4
	{0xe0, 0xef, 0xef, 0xe1, 0xfe, 0xee, 0xf1},  // 35 - 5
	{0xf1, 0xef, 0xef, 0xe1, 0xee, 0xee, 0xf1},  // 36 - 6
	{0xe0, 0xfe, 0xfe, 0xfd, 0xfb, 0xfb, 0xfb},  // 37 - 7
	{0xf1, 0xee, 0xee, 0xf1, 0xee, 0xee, 0xf1},  // 38 - 8
	{0xf1, 0xee, 0xee, 0xf0, 0xfe, 0xfd, 0xfb},  // 39 - 9
	{0xff, 0xff, 0xfb, 0xff, 0xfb, 0xff, 0xff},  // 3a - :
	{0xff, 0xff, 0xfb, 0xff, 0xfb, 0xf7, 0xff},  // 3b - ;
	{0xfd, 0xfb, 0xf7, 0xef, 0xf7, 0xfb, 0xfd},  // 3c - <
	{0xff, 0xff, 0xe0, 0xff, 0xe0, 0xff, 0xff},  // 3d - =
	{0xf7, 0xfb, 0xfd, 0xfe, 0xfd, 0xfb, 0xf7},  // 3e - >
	{0xf1, 0xee, 0xed, 0xfb, 0xfb, 0xff, 0xfb},  // 3f - ?
	{0xf1, 0xea, 0xe4, 0xe4, 0xe5, 0xea, 0xf1},  // 40 - @
	{0xf1, 0xee, 0xee, 0xee, 0xe0, 0xee, 0xee},  // 41 - A
	{0xe1, 0xee, 0xee, 0xe1, 0xee, 0xee, 0xe1},  // 42 - B
	{0xf1, 0xee, 0xef, 0xef, 0xef, 0xee, 0xf1},  // 43 - C
	{0xe1, 0xee, 0xee, 0xee, 0xee, 0xee, 0xe1},  // 44 - D
	{0xe0, 0xef, 0xef, 0xe1, 0xef, 0xef, 0xe0},  // 45 - E
	{0xe0, 0xef, 0xef, 0xe1, 0xef, 0xef, 0xef},  // 46 - F
	{0xf1, 0xee, 0xef, 0xe8, 0xee, 0xee, 0xf1},  // 47 - G
	{0xee, 0xee, 0xee, 0xe0, 0xee, 0xee, 0xee},  // 48 - H
	{0xe0, 0xfb, 0xfb, 0xfb, 0xfb, 0xfb, 0xe0},  // 49 - I
	{0xe0, 0xfd, 0xfd, 0xfd, 0xed, 0xed, 0xf3},  // 4a - J
	{0xee, 0xed, 0xeb, 0xe7, 0xeb, 0xed, 0xee},  // 4b - K
	{0xef, 0xef, 0xef, 0xef, 0xef, 0xef, 0xe0},  // 4c - L
	{0xee, 0xe4, 0xea, 0xea, 0xee, 0xee, 0xee},  // 4d - M
	{0xee, 0xee, 0xe6, 0xea, 0xec, 0xee, 0xee},  // 4e - N
	{0xf1, 0xee, 0xee, 0xee, 0xee, 0xee, 0xf1},  // 4f - O
	{0xe1, 0xee, 0xee, 0xe1, 0xef, 0xef, 0xef},  // 50 - P
	{0xf1, 0xee, 0xee, 0xee, 0xea, 0xec, 0xf0},  // 51 - Q
	{0xe1, 0xee, 0xee, 0xe1, 0xeb, 0xed, 0xee},  // 52 - R
	{0xf1, 0xee, 0xef, 0xf1, 0xfe, 0xee, 0xf1},  // 53 - S
	{0xe0, 0xfb, 0xfb, 0xfb, 0xfb, 0xfb, 0xfb},  // 54 - T
	{0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xf1},  // 55 - U
	{0xee, 0xee, 0xee, 0xee, 0xee, 0xf5, 0xfb},  // 56 - V
	{0xee, 0xee, 0xea, 0xea, 0xea, 0xea, 0xf5},  // 57 - W
	{0xee, 0xee, 0xf5, 0xfb, 0xf5, 0xee, 0xee},  // 58 - X
	{0xee, 0xee, 0xf5, 0xfb, 0xfb, 0xfb, 0xfb},  // 59 - Y
	{0xe0, 0xfe, 0xfd, 0xfb, 0xf7, 0xef, 0xe0},  // 5a - Z
	{0xe3, 0xef, 0xef, 0xef, 0xef, 0xef, 0xe3},  // 5b - [
	{0xff, 0xef, 0xf7, 0xfb, 0xfd, 0xfe, 0xff},  // 5c - backslash
	{0xfc, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfc},  // 5d - ]
	{0xfb, 0xf5, 0xee, 0xff, 0xff, 0xff, 0xff},  // 5e - ^
	{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0},  // 5f - _
	{0xfb, 0xfb, 0xfb, 0xff, 0xff, 0xff, 0xff},  // 60 - '
	{0xff, 0xff, 0xf9, 0xf6, 0xf6, 0xf6, 0xf8},  // 61 - a
	{0xf7, 0xf7, 0xf1, 0xf6, 0xf6, 0xf6, 0xf1},  // 62 - b
	{0xff, 0xff, 0xf9, 0xf6, 0xf7, 0xf6, 0xf9},  // 63 - c
	{0xfe, 0xfe, 0xf8, 0xf6, 0xf6, 0xf6, 0xf8},  // 64 - d
	{0xff, 0xff, 0xf9, 0xf0, 0xf7, 0xf6, 0xf9},  // 65 - e
	{0xfd, 0xfa, 0xfb, 0xf1, 0xfb, 0xfb, 0xfb},  // 66 - f
	{0xff, 0xf9, 0xf6, 0xf6, 0xf8, 0xf6, 0xf9},  // 67 - g
	{0xf7, 0xf7, 0xf1, 0xf6, 0xf6, 0xf6, 0xf6},  // 68 - h
	{0xff, 0xff, 0xfb, 0xff, 0xfb, 0xfb, 0xfb},  // 69 - i
	{0xff, 0xfd, 0xff, 0xfd, 0xfd, 0xfd, 0xf3},  // 6a - j
	{0xf7, 0xf7, 0xf5, 0xf3, 0xf3, 0xf5, 0xf6},  // 6b - k
	{0xfb, 0xfb, 0xfb, 0xfb, 0xfb, 0xfb, 0xfb},  // 6c - l
	{0xff, 0xff, 0xe5, 0xea, 0xea, 0xea, 0xea},  // 6d - m
	{0xff, 0xff, 0xf1, 0xf6, 0xf6, 0xf6, 0xf6},  // 6e - n
	{0xff, 0xff, 0xf9, 0xf6, 0xf6, 0xf6, 0xf9},  // 6f - o
	{0xff, 0xf1, 0xf6, 0xf6, 0xf1, 0xf7, 0xf7},  // 70 - p
	{0xff, 0xf8, 0xf6, 0xf6, 0xf8, 0xfe, 0xfe},  // 71 - q
	{0xff, 0xff, 0xf1, 0xf6, 0xf7, 0xf7, 0xf7},  // 72 - r
	{0xff, 0xff, 0xf8, 0xf7, 0xf9, 0xfe, 0xf1},  // 73 - s
	{0xff, 0xff, 0xfb, 0xf1, 0xfb, 0xfb, 0xfb},  // 74 - t
	{0xff, 0xff, 0xf6, 0xf6, 0xf6, 0xf6, 0xf8},  // 75 - u
	{0xff, 0xff, 0xf6, 0xf6, 0xf6, 0xf6, 0xf9},  // 76 - v
	{0xff, 0xff, 0xea, 0xea, 0xea, 0xea, 0xf5},  // 77 - w
	{0xff, 0xff, 0xee, 0xf5, 0xfb, 0xf5, 0xee},  // 78 - x
	{0xff, 0xfa, 0xfa, 0xfa, 0xfd, 0xfb, 0xf7},  // 79 - y
	{0xff, 0xff, 0xf0, 0xfd, 0xfb, 0xf7, 0xf0},  // 7a - z
	{0xfd, 0xfb, 0xfb, 0xf7, 0xfb, 0xfb, 0xfd},  // 7b - {
	{0xfb, 0xfb, 0xfb, 0xff, 0xfb, 0xfb, 0xfb},  // 7c - |
	{0xf7, 0xfb, 0xfb, 0xfd, 0xfb, 0xfb, 0xf7},  // 7d - }
	{0xff, 0xfa, 0xf5, 0xff, 0xff, 0xff, 0xff},  // 7e - ~
	{0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0}   // 7f - block
}; 

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void setDisplay(const CHAR* src)
{
	int i;
	portENTER_CRITICAL();
	switch(dir)
	{
		case SHIFT_UP:
		case SHIFT_DOWN:
		case SHIFT_NONE: for(i = 0; i < 7; i++) dply[i] = dply[i] & 0x1F | (*src++ << 8); break;
		case SHIFT_LEFT: for(i = 0; i < 7; i++) dply[i] = dply[i] & 0x1F | 0x83E0 | (*src++ << 10); break;
		case SHIFT_RIGHT:for(i = 0; i < 7; i++) dply[i] = dply[i] & 0x1F | 0xF820 | (*src++ << 6); break;
	}
	dply[7] = 0xFFFF;
	portEXIT_CRITICAL();
}

/* get ascii-char from queue and 
 * copy its image to display buffer	
 */
static void nextDisplay(void)
{
	UCHAR c;
	while(1)
	{
		xQueueReceive( xLED5x7Queue, &c, portMAX_DELAY);
		c &= 0x7F;

		if(c >= ' ')
			break;
		if(c <= SHIFT_DOWN)
			LED5x7_setDir(c);
	}	
	
	c -= ' ';
	currglyph = cmatrix[c];
	setDisplay(currglyph);
}


/****************************************************************************
 * Name: LEDTick
 * Timer procedure to display all scan-lines 
 ****************************************************************************/

void /*nested_interrupt*/ LED5x7Tick( )
{
	uint8_t iir = TMR1_IIR;

	LEDMATRIX_CATHODE =dply[row];		// set row image
	LEDMATRIX_ANODE   = col[row]  ;		// enable row
	if(++row > 6)
	{
		row = 0;
		xTaskNotifyIndexedFromISR(LEDTaskHandle, 0, 0, eIncrement, NULL);
	}	
}

/****************************************************************************
 * Name: LED5x7Task
 * Task handle the 5x7 LED Matrix
 ****************************************************************************/
static void LED5x7Task( void *arg)
{
	int i;
	while(1)
	{
		nextDisplay();			// get next character image to displaybuffer
		ulTaskNotifyTake(pdFALSE, portMAX_DELAY );
		
		for(i = 0; i < shift_width; i++)
		{
			ulTaskNotifyTake( pdFALSE, portMAX_DELAY );
			shiftDisplay();		// shift in the new char image
			vTaskDelay(shiftdly);	// delay between chars
		}
		vTaskDelay(chardly);	// delay between chars
	}
}
/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* check if no chars on queue 
 * returns pdTRUE if queue is empty.
 */
UBaseType_t LED5x7Spaces()
{
	return uxQueueSpacesAvailable( xLED5x7Queue );
}

UBaseType_t LED5x7Count()
{
	return uxQueueMessagesWaiting( xLED5x7Queue );
}
/* append c-string to queue
 * returns number of chars added
 */
BaseType_t LED5x7_puts(const CHAR *s, TickType_t tout)
{
	BaseType_t res = 0;
	if(s)
		while(*s && LED5x7_putchar(*s++, tout))
			res++;
	return res;
}


/* append one char to queue with timeout
 * returns pdTRUE  if char added pdFALSE if not
 */
BaseType_t LED5x7_putchar(CHAR c, TickType_t tout)
{
	return xQueueSend( xLED5x7Queue, (const void*)&c, tout);
}

/****************************************************************************
 * initLED5x7
 * setup the 5x7 LED Matrix driver
 ****************************************************************************/
static int tid;
void initLED5x7()
{
	uint8_t th = ledticks >> 8;
	uint8_t tl = ledticks;

	currglyph  	= cmatrix[95];
	frames		= LED5x7_FRAMES;	// display refresch delay 
	chardly		= LED5x7_CDELAY;	// LED5x7_FRAMES deleys between letters
	dir			= SHIFT_NONE;
	memset(dply,0xFF,sizeof(dply));
	
	xLED5x7Queue= xQueueCreate( LED5x7_QUEUES, sizeof( CHAR));
	xTaskCreate( LED5x7Task, "LED5x7", configMINIMAL_STACK_SIZE, NULL, LED5x7_PRIO, &LEDTaskHandle);

	TMR1_IER = 0;
	_set_vector(TIMER1_IVECT, LED5x7Tick);
	
	TMR1_DR_H= th;
	TMR1_DR_L= tl;
	TMR1_RR_H= th;
	TMR1_RR_L= tl;
	
	TMR1_CTL = 0x8F;// 1 00 01 1 1 1
	TMR1_IER = 1;
}

/* set shift direction */
dir_e LED5x7_setDir(dir_e to)
{
	dir_e old = dir;
	dir = to;
	shift_width = width[to];
	return old;
}
