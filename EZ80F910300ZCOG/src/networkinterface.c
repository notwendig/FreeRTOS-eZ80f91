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


	File: 	NetworkInterface.c
			module ethernet EMAC driver.
			Connect ansi-terminal to comport0 (115200,8,1,n) to see what's happen.
			Part of FreeRTOS Port for the eZ80F91 Development Kit eZ80F910300ZCOG
			See www.zilog.com for desciption.


	Developer:
	SIE	 Juergen Sievers <JSievers@NadiSoft.de>

	150804:	SIE Start this port.

*/

/*
Memory
EMAC memory is the shared Ethernet memory location of the Transmit and Receive buff-
ers. This memory is broken into two parts: the Tx buffer and the Rx buffer. The Transmit
Lower Boundary Pointer Register, EmacTLBP, is the register that holds the starting
address of the Tx buffer. The Boundary Pointer Register, EmacBP, points to the start of the
Rx buffer (end of Tx buffer + 1). The Receive High Boundary Pointer Register,
EmacRHBP, points to the end of the Rx buffer + 1. The Tx and Receive buffers are
divided into packet buffers of either 256, 128, 64, or 32 bytes. These buffer sizes are
selected by EmacBufSize register bits 7 and 6.

The EmacBlksLeft register contains the number of Receive packet buffers remaining in
the Rx buffer. This buffer is used for software flow control. If the Block_Level is nonzero
(bits 5:0 of the EmacBufSize register), hardware flow control is enabled. If in FULL-
DUPLEX mode, the EMAC transmits a pause control frame when the EmacBlksLeft reg-
ister is less than the Block_Level. In HALF-DUPLEX mode, the EMAC continually trans-
mits a nibble pattern of hexadecimal 5s to jam the channel.

Four pointers are defined for reading and writing the Tx and Rx buffers. The Transmit
Write Pointer, TWP, is a software pointer that points to the next available packet buffer.
The TWP is reset to the value stored in EmacTLBP. The Transmit Read Pointer, TRP, is a
hardware pointer in the Transmit Direct Memory Access Register, TxDMA, that contains
the address of the next packet to be transmitted. It is automatically reset to the EmacTLBP.
The Receive Write Pointer, RWP, is a hardware pointer in the Receive Direct Memory
Access Register, RxDMA, which contains the storage address of the incoming packet. The
RWP pointer is automatically initialized to the Boundary Pointer registers. The Receive
Read Pointer, RRP, is a software pointer to where the next packet must be read from. The
RRP pointer must be initialized to the Boundary Pointer registers. For the hardware flow
control to function properly, the software must update the hardware RRP (EmacRrp)
pointer whenever the software version is updated. The RxDMA uses RWP and the RRP to
determine how many packet buffers remain in the Rx buffer.
*/

#include "stdint.h"
#include "FreeRTOS.h"
#include "list.h"
#include "semphr.h"



#include "FreeRTOS_IP.h"
#include "FreeRTOS_IP_Private.h"
#include "NetworkInterface.h"
#include "NetworkBufferManagement.h"
#include "Stdio.h"
#include "ez80_emac.h"
#include "_ez80.h"
#include "_ez80f91.h"
#define F91_EMAC_CONF_t void*
#include "AMD79C874_phy.h"

#define	ETHPKT_DLEN		ipconfigNETWORK_MTU	/* length of data field ep		*/
#define	ETHPKT_HLEN		sizeof(NetworkBufferDescriptor_t)	/* size of (extended) Ether header	*/

#define	ETHPKT_MINLEN	ipconfigETHERNET_MINIMUM_PB_PACKET_BYTES	   /* minimum packet length		*/
#define	ETHPKT_MAXLEN	(ETHPKT_HLEN+ETHPKT_DLEN)

/*******************************************************************************
*                            GLOBAL VARIABLES
*******************************************************************************/

// EMAC Tables
#define EmacRwp       *((IORegInt16)((BYTE)0x51)) // EMAC_RWP_L & EMAC_RWP_H
#define EmacTrp       *((IORegInt16)((BYTE)0x53)) // EMAC_RWP_L & EMAC_RWP_H

#define _rwp ((_tSEMAC_DescTbl*) (emacram + EmacRwp))
#define _trp ((_tSEMAC_DescTbl*) (emacram + EmacTrp))
#define _rrp ((_tSEMAC_DescTbl*) (emacram + EmacRrp))


/*******************************************************************************
*                            LOCALE VARIABLES
*******************************************************************************/

static _tSEMAC_DescTbl* _twp;

static uint16_t uiEMAC_BufSize;       // EMAC packet buffer size
static uint16_t uiEMAC_BufMask;       // EMAC BUFZ mask

static SemaphoreHandle_t txsem = 0;
static SemaphoreHandle_t phySemSync = 0;
static SemaphoreHandle_t phySem = 0;
static TaskHandle_t 	 xRxTask = 0;
static TaskHandle_t 	 xWatchTask = 0;

static UINT8 *emacram;
static unsigned iowatch;

static _tSEMAC_DescTbl* _rhbp;
static _tSEMAC_DescTbl* _bp;

static _tSEMAC_DescTbl* _tlbp;

void emacConnThd(void *argv);
static const TickType_t xDontBlock = 0;

/*******************************************************************************
*                            FUNCTION PROTOTYPES
*******************************************************************************/
static void EMAC_Reset(void);
static void EMAC_EnableIrq(void);
static void EMAC_DisableIrq(void);

BaseType_t sETH_TransmitPkt(const NetworkBufferDescriptor_t *_asTxPacket);

static BaseType_t ETH_SendNow(const UINT8 *pkt, uint16_t usPktLen);
static void ETH_ReceivePkt(NetworkBufferDescriptor_t *_asRxPacket);

static emacStat_t stats;
const emacStat_t* emac_stat() { return &stats;}

static int nwout = 0; 


BaseType_t sPHY_Init(void)
{
    uint16_t 	phyData;
    int 		isAutoNeg;
    uint16_t 	i,
				temp;
    BaseType_t	res = pdFAIL;

	iowatch++;
	if(phySem && phySemSync)
	{
		xSemaphoreGive(phySem);

		EMAC_FIAD = PHY_ADDRESS;	// Configure PHY address on the F91

		// Check if PHY detected
		if (usPHY_ReadReg(PHY_ID1_REG, &phyData) == pdTRUE && phyData == PHY_ID1 &&
			usPHY_ReadReg(PHY_ID2_REG, &phyData) == pdTRUE && phyData == PHY_ID2)
		{
			// Reset the PHY
			usPHY_WriteReg(PHY_CREG, PHY_RST);    // Reinitialize the PHY controller

			// Check PHY capabilities on connection
			if(usPHY_ReadReg(PHY_SREG, &phyData) == pdTRUE)
			{
				if(phyData & PHY_CAN_AUTO_NEG)
				{    // PHY device can do auto-negotiation
					phyData = PHY_RESTART_AUTO_NEG | PHY_AUTO_NEG_ENABLE;
					isAutoNeg = pdTRUE;
				}
				else
				{    // PHY device cannot do auto-negotiation
					phyData = 0;
					isAutoNeg = pdFALSE;
					EMAC_CFG1 = PADEN | CRCEN;    // disable padding; enable CRC
				}

				// Set desired link capabilities
				switch(EMAC_NEGMOD)
				{
					case F91_AUTO:        // Auto-Negotiation
						if(isAutoNeg)
						{    // Set PHY auto negotiation mode
							temp = PHY_ANEG_100_FD | PHY_ANEG_100_HD | PHY_ANEG_10_FD | PHY_ANEG_10_HD | PHY_ANEG_802_3;
							usPHY_WriteReg(PHY_ANEG_ADV_REG, temp);
						}
						break;
					case F91_100_FD:    // 100 Mbps Full Duplex
						phyData |= PHY_100BT | PHY_FULLD;
						temp = PHY_ANEG_100_FD | PHY_ANEG_100_HD | PHY_ANEG_10_FD | PHY_ANEG_10_HD | PHY_ANEG_802_3 ;
						usPHY_WriteReg(PHY_ANEG_ADV_REG, temp);
						break;
					case F91_100_HD:    // 100 Mbps Half Duplex
						phyData |= PHY_100BT | PHY_HALFD;
						temp = PHY_ANEG_100_HD | PHY_ANEG_10_FD | PHY_ANEG_10_HD | PHY_ANEG_802_3;
						usPHY_WriteReg(PHY_ANEG_ADV_REG, temp);
						break;
					case F91_10_FD:        // 10 Mbps Full Duplex
						phyData |= PHY_10BT | PHY_FULLD;
						temp = PHY_ANEG_10_FD | PHY_ANEG_10_HD | PHY_ANEG_802_3;
						usPHY_WriteReg(PHY_ANEG_ADV_REG, temp);
						break;
					caseF91_10_HD:        // fall through
					default:            // 10 Mpbs Half Duplex
						phyData |= PHY_10BT | PHY_HALFD;
						temp = PHY_ANEG_10_HD | PHY_ANEG_802_3;
						usPHY_WriteReg(PHY_ANEG_ADV_REG, temp);
						break;
				}

				if(usPHY_WriteReg(PHY_CREG, phyData) == pdTRUE)
				{
					////////////////////////////////////////////////////////////////////////////
					// Wait for a link to be established
					////////////////////////////////////////////////////////////////////////////
					FreeRTOS_debug_printf( ("Link ") );
					for(i = 50; i; i--)
					{
						iowatch++;
						vTaskDelay(pdMS_TO_TICKS(200));  // delay before next attempt

						if(usPHY_ReadReg(PHY_SREG, &phyData) == pdTRUE)
						{
							if(phyData & (PHY_AUTO_NEG_COMPLETE | PHY_LINK_ESTABLISHED))
								break;
						}
						else
						{
							FreeRTOS_debug_printf( ("PHY_ReadReg SREG error\n") );
						}
						FreeRTOS_debug_printf( (".") );
					}

					if(phyData & PHY_LINK_ESTABLISHED)
					{
						iowatch++;
						usPHY_ReadReg( PHY_DIAG_REG, &phyData );
						EMAC_CFG1 |= PADEN | CRCEN;
						if( phyData & PHY_FULL_DUPLEX )
							EMAC_CFG1 |= FULLD;
						else
							EMAC_CFG1 &= ~FULLD;

						FreeRTOS_debug_printf( ("EMAC %dMBPS, %s_DUPLEX, ", phyData & PHY_100_MBPS?100:10, phyData & PHY_FULL_DUPLEX ? "FULL":"HALF") );

						res = pdTRUE;
					}
				}
				else
				{
					FreeRTOS_debug_printf( ("PHY_WriteReg CREG error\n") );
				}
			}
			else
			{
				FreeRTOS_debug_printf( ("PHY_ReadReg SREG error\n") );
			}
		}
		else
		{
			FreeRTOS_debug_printf( ("Can't detect PHY\n") );
		}
	}
	else
	{
		FreeRTOS_debug_printf( ("PHY semaphors not valide.\n") );
	}
	////////////////////////////////////////////////////////////////////////////
	// Determine link settings, if a link is successfully established
	////////////////////////////////////////////////////////////////////////////
	if(res == pdTRUE)
	{
		FreeRTOS_debug_printf( ("Link up\n") );
	}
	else
	{
		FreeRTOS_debug_printf( ("Link down\n") );
	}
	return res;
}

static BaseType_t usPHY_WriteReg(uint16_t usReg, uint16_t usData)
{
	BaseType_t res = xSemaphoreTake(phySem,portMAX_DELAY);
	if( pdTRUE == res )
	{
		EMAC_ISTAT = MGTDONE;
		EMAC_IEN   |= MGTDONEIEN;
		EmacCtld = usData;      // load data to write onto PHY
		EMAC_RGAD = usReg;      // load register # of PHY to be written
		EMAC_MIIMGT |= LCTLD;	// start writing to PHY
		res = xSemaphoreTake(phySemSync,portMAX_DELAY);
		EMAC_IEN   &= ~MGTDONEIEN;
		xSemaphoreGive(phySem);
	}
    return res;
}

BaseType_t usPHY_ReadReg(uint16_t usReg, uint16_t *ausData)
{
	BaseType_t res = xSemaphoreTake(phySem,portMAX_DELAY);

	if( pdTRUE == res )
    {
		EMAC_ISTAT = MGTDONE;
		EMAC_IEN   |= MGTDONEIEN;
		EMAC_RGAD = usReg;                // load register # of PHY to be read
		EMAC_MIIMGT |= RSTAT;            // start reading from PHY

		res = xSemaphoreTake(phySemSync,portMAX_DELAY);
		EMAC_IEN   &= ~MGTDONEIEN;
		// Get read data from EMAC register
		if( pdTRUE == res)
			*ausData = (EMAC_PRSD_L + ((uint16_t)EMAC_PRSD_H << 8));
		xSemaphoreGive(phySem);
	}

    return res;
}

BaseType_t xIsEthernetConnected( void )
{
   uint16_t		phy_data;

   /*
    * Read the Phy status register to determine if there is a physical connection.
    */
	return ( usPHY_ReadReg( PHY_SREG, &phy_data ) == pdTRUE
	   && (phy_data & (PHY_AUTO_NEG_COMPLETE | PHY_LINK_ESTABLISHED))) ? pdTRUE:pdFALSE;
}

static void netWatchTask(void*x)
{

	while(1)
	{
		unsigned iow = iowatch;
		vTaskDelay(pdMS_TO_TICKS(2000));  // delay before next attempt
		if(iow == iowatch && xIsEthernetConnected() == pdFALSE)
		{
			static const IPStackEvent_t xNetworkDownEvent = { eNetworkDownEvent, NULL };
			FreeRTOS_debug_printf( ("Link down, restartig.\n") );
			xSendEventStructToIPTask( &xNetworkDownEvent, xDontBlock );
			vTaskDelay(pdMS_TO_TICKS(15000));
		}
	}
}

static void /*nested_interrupt*/ EMAC_EthTxIsr(void)
{
	uint8_t istat = EMAC_ISTAT;
	EMAC_ISTAT  = istat & (TXPCF|TXDONE); // Read and Clear the interrupt
	iowatch++;

	if(istat & TXPCF)
		stats.txpcf++;

	if(istat & TXDONE)
		stats.txdone++;
}

static void RxTask(void*x)
{
	while(1)
	{
		vTaskSuspend(0);	// EMAC  RxQueue empty. Wakeup on RxInterrupts

		while(_rrp != _rwp)	// as long as reception on EMAC-Queue
		{

			if(_rrp->stat & RxCrcError)
				stats.rxcrcerr++;

			if(_rrp->stat & RxAlignError)
				stats.rxalignerr++;

			if(_rrp->stat & RxLongEvent)
				stats.rxlongevent++;

			if(_rrp->stat & RxOK)	// Push receiped messageges to IP-Stack
			{
				uint16_t plen = _rrp->pktsz - 4;

				stats.rxok++;
				stats.rxsz += plen;

				if( nwout && plen >= ETHPKT_MINLEN && plen < ETHPKT_MAXLEN)
				{
					NetworkBufferDescriptor_t *nwDis = pxGetNetworkBufferWithDescriptor( plen, 0);

					if(nwDis && nwDis->pucEthernetBuffer)
					{
						xIPStackEvent_t xNetworkRxEvent = { eNetworkRxEvent, NULL };

						// move the data from the rx ring buffers into system buffer
						int8_t *psrc = (int8_t*) &_rrp[1];	// skip EMAC header

						// check and handle wrap on rx ring buffer
						if ((psrc + plen) > (int8_t*)_rhbp)
						{
							size_t temp = (int8_t*)_rhbp - psrc;
							memcpy(nwDis->pucEthernetBuffer, psrc, temp);
							memcpy(nwDis->pucEthernetBuffer+temp, _bp, plen - temp);
						}
						else
						{
							memcpy(nwDis->pucEthernetBuffer, psrc, plen);
						}

						xNetworkRxEvent.pvData  = nwDis;
						if(xSendEventStructToIPTask( &xNetworkRxEvent, xDontBlock ) == pdFALSE)
						{
							stats.rxnospace++;
							vReleaseNetworkBufferAndDescriptor( nwDis );
						}
					}
					else
					{
						if(nwDis)
							vReleaseNetworkBufferAndDescriptor( nwDis );
						stats.rxnospace++;
					}

				}
				else
					stats.rxinvsize++;
			}
			else
				stats.rxnotok++;

			EmacRrp = (uint16_t) _rrp->np;
		}

	}
}

static void /*nested_interrupt*/ EMAC_EthRxIsr(void)
{
	uint8_t istat;

	EMAC_ISTAT  = (istat = EMAC_ISTAT) & (RXCF|RXPCF|RXDONE); // Get and clear the interrupt
	iowatch++;

	if(istat & RXCF)
		stats.rxcf++;

	if(istat & RXPCF)
		stats.rxpcf++;

	if(istat & RXDONE)
		stats.rxdone++;

	xTaskResumeFromISR(xRxTask);	// wakeup the rx worker thread
}

static void /*nested_interrupt*/ EMAC_EthSysIsr(void)
{
	uint8_t istat = EMAC_ISTAT;

	EMAC_ISTAT  = istat & (RXOVRRUN|TXFSMERR|MGTDONE); // Get and clear the interrupt ;

	if( istat & RXOVRRUN)
		stats.rxover++;

	if( istat & TXFSMERR)	// ToDo: Should reset whole eth-device
		stats.txfsmerr++;

	if( istat & MGTDONE)	// Phy read/write finisched interrupt
	{
		xSemaphoreGiveFromISR(phySemSync,0);
		stats.mgdone++;
	}
}

static void EMAC_EnableIrq(void)
{
    EMAC_IEN = 	  TXFSMERRIEN  // Enable Tx State Machine Err Isr (system isr)
				| MGTDONEIEN   // Enable MII Mgt Done Isr (system isr)
				| RXCFIEN      // Enable Rx Ctrl Frame Isr (Rx isr)
				| RXPCFIEN     // Enable Rx Pause Ctrl Frame isr (Rx isr)
				| RXDONEIEN    // Enable Rx Done Isr (Rx isr)
				| RXOVRRUNIEN  // Enable Rx Overrun Isr (Rx isr) SIE: (system isr)
				| TXPCFIEN     // Enable Tx Ctrl Frame Isr (Tx isr)
				| TXDONEIEN    // Enable Tx Done Isr (Tx isr)
				;
}

static void EMAC_DisableIrq(void)
{
    EMAC_IEN 	= 0; 		// disable the tx and rx interrupts
    EMAC_ISTAT 	= 0xFF;     // clear interrupt status flags
}

static void EMAC_Reset(void)
{
    EMAC_DisableIrq();
	memset(&stats, 0, sizeof(stats));
    // Reset all pemac blocks
    EMAC_RST = SRST | HRTFN | HRRFN | HRTMC | HRRMC | HRMGT;
}

static BaseType_t sETH_Init()
{
	static BaseType_t isinit = pdFAIL;
	if(isinit != pdPASS)
	{
		isinit = pdPASS;
		if(!txsem )txsem = xSemaphoreCreateBinary ();
		if(!phySem) phySem = xSemaphoreCreateBinary ();
		if(!phySemSync) phySemSync = xSemaphoreCreateBinary ();

		if( txsem && phySem && phySemSync)
		{
			xSemaphoreGive(txsem);

			isinit = xTaskCreate( netWatchTask, "NetWatch", 512, 0, ipconfigIP_TASK_PRIORITY-3, &xWatchTask);

			isinit = xTaskCreate( RxTask, "RxTask", 1024, 0, ipconfigIP_TASK_PRIORITY+1, &xRxTask);

			if(isinit == pdPASS)
			{
				////////////////////////////////////////////////////////////////////////////
				// Enable EMAC phy-interrupts
				////////////////////////////////////////////////////////////////////////////
				_set_vector(EMACRX_IVECT, EMAC_EthRxIsr);    // setup EMAC interrupts
				_set_vector(EMACTX_IVECT, EMAC_EthTxIsr);
				_set_vector(EMACSYS_IVECT, EMAC_EthSysIsr);
			}
		}
		else
			isinit = pdFAIL;
	}

	return isinit;
}

BaseType_t xNetworkInterfaceInitialise()
{
	BaseType_t result = pdFAIL;
    uint16_t i;
    uint16_t bufSizeShift;
	const uint8_t* ucMACAddress = FreeRTOS_GetMACAddress();

    ////////////////////////////////////////////////////////////////////////////
    // Place EMAC in reset state
    ////////////////////////////////////////////////////////////////////////////
	EMAC_DisableIrq();
	EMAC_Reset();	// reset pointer

    ////////////////////////////////////////////////////////////////////////////
    // Initialize EMAC RAM
    ////////////////////////////////////////////////////////////////////////////

	emacram = (UINT8 *)(((UINT32)RAM_ADDR_U << 16) + (UINT8*)EMAC_RAM_OFFSET);

	RAM_CTL |= ERAM_EN;    // Ensure internal EMAC Ram is enabled
    // Check access to EMAC shared ram
    for(i = 0; i < 18 ; i++)
        emacram[i] = i;

    for(i = 0; i < 18; i++)
        if(emacram[i] != (UINT8)i)
        {
            return (PEMAC_RAMERR);
        }

    memset(emacram, 0, EMAC_RAM_SIZE);

    ////////////////////////////////////////////////////////////////////////////
    // Setup EMAC buffer size
    ////////////////////////////////////////////////////////////////////////////

    bufSizeShift   = 8 - EMAC_BUFSZCFG;
    EMAC_BUFSZ     = EMAC_BUFSZCFG << 6;
    uiEMAC_BufMask = 0xFFFF << bufSizeShift;
    uiEMAC_BufSize = 1 << bufSizeShift;

	////////////////////////////////////////////////////////////////////////////
    // locate boundary pointers in EMAC shared memory
    ////////////////////////////////////////////////////////////////////////////
    // load EMAC shared memory registers with appropriate addresses
	_tlbp = (_tSEMAC_DescTbl*) (emacram);
	_bp   = (_tSEMAC_DescTbl*) (emacram + EMAC_TXBSZ);
	_rhbp = (_tSEMAC_DescTbl*) (emacram + EMAC_RAM_SIZE);

	EMAC_RHBP_L = (UINT8) (UINT24)_tlbp;
    EMAC_RHBP_H = (UINT8)((UINT24)_tlbp >> 8);
    EMAC_BP_L   = (UINT8) (UINT24)_bp;
    EMAC_BP_H   = (UINT8)((UINT24)_bp >> 8);
    EMAC_BP_U   = (UINT8)((UINT24)_bp >> 16);
    EMAC_TLBP_L = (UINT8) (UINT24)_rhbp;
	EMAC_TLBP_H = (UINT8)((UINT24)_rhbp >> 8);

	// Set status of the first Tx packet buffer to HOST OWNS to ensure that
    // the EMAC will not try to send the packet as soon as this init routine
    // is done.
	_twp  = (_tSEMAC_DescTbl*) (emacram + EmacTrp);
    _twp->stat = HOST_OWNS;
	_twp->np   = 0;
	_twp->pktsz= 0;

	////////////////////////////////////////////////////////////////////////////
    // Setup the EMAC station address
    ////////////////////////////////////////////////////////////////////////////
    for (i = 0; i < 6; i++)
    {
        EmacStad(i) = (BYTE)*(ucMACAddress+i);
    }
    /*
    for (i = 0; i < 6; i++)         // Check if MAC addr is properly saved
    {
        if ((BYTE)*(ucMACAddress+i) != EmacStad(i))
        {
            return(PEMAC_NOT_FOUND);
        }
    }
	*/

    ////////////////////////////////////////////////////////////////////////////
    // Set pause frame control timeout value
    ////////////////////////////////////////////////////////////////////////////
    EMAC_TPTV_L = 0x14;
    EMAC_TPTV_H = 0x00;


    ////////////////////////////////////////////////////////////////////////////
    // Set transmit polling timer for minimum timeout period
    ////////////////////////////////////////////////////////////////////////////
    EMAC_PTMR = 1;

    ////////////////////////////////////////////////////////////////////////////
    // Disable EMAC test modes
    ////////////////////////////////////////////////////////////////////////////
    EMAC_TEST = 0;

    ////////////////////////////////////////////////////////////////////////////
    // Configure EMAC behavior
    ////////////////////////////////////////////////////////////////////////////
    EMAC_CFG1 = 0           // b7=0: disable padding
                            // b6=0: disable frame autodetection
                            // b5=0: do not pad short frames
                            // b4=0: disable CRC
                            // b3=0: operate in half duplex; CSMA/CD enabled
                            // b2=0: ignore length field in Tx/Rx frames
                | HUGEN     // b1=1: allow large frames to be received
                            // b0=0: no proprietary header
	;

    EMAC_CFG2 =   BPNB    	// b7=0: no back pressure
                            // b6=0: enable exponential back-off
                | 0x38      // b5-0=56: set late collision at byte 56
	;

    EMAC_CFG3 = 0           // b7=0: allow any preamble length
                            // b6=0: no preamble error checking
                            // b5=0: abort when deferral limit is reached
                            // b4=0: disable 10 Mbps ENDEC mode
                |RETRY_MASK // b3-0=15: max # collisions = 15
    ;

    EMAC_CFG4 = 0           // b7=0: rsv
                            // b6=0: do not transmit a pause control frame
                            // b5=0: disable back-off pressure
              | PARF        // b4=1: receive all frames
              | RXFC        // b3=1: react to pause control frames received
                            // b2=0: do not transmit a pause control frames
                            // b1=0: do not force a pause condition
              | RXEN        // b0=1: enable EMAC receiver
	;
    // Set the address filter register
    EMAC_AFR = 0x0F;        // allow broadcast and multicast addresses

    // Specify max frame length
    EMAC_MAXF_L = 0x00;     // set the max # bytes per packet to 1536d
    EMAC_MAXF_H = 0x06;

    // Re-enable EMAC operation by taking it out of reset state
    EMAC_RST = 0;

    // Set the MII Management clock to enable PHY access
    EMAC_MIIMGT = CLKDIV20;

    // Clear interrupt status register
    EMAC_ISTAT = 0xFF;

    if((result = sETH_Init()) == pdPASS &&
	   (result = sPHY_Init()) == pdPASS)
    {
    	// Enable EMAC interrupts
		EMAC_EnableIrq();
		vTaskDelay(pdMS_TO_TICKS(500));
    }
    return result;
}


static BaseType_t ETH_SendNow(const UINT8 *pkt, uint16_t usPktLen)
{
	_tSEMAC_DescTbl *ptNextPkt;
	UINT24 usBufLen = (usPktLen + uiEMAC_BufSize - 1 + DESC_OVH) & uiEMAC_BufMask;
    UINT24 WrapLen;
    UINT8 *pDst;
	UINT24 maxsz;
	BaseType_t ret = xSemaphoreTake(txsem, portMAX_DELAY);

    ////////////////////////////////////////////////////////////////////////////
    // Predetermine the location of the packet that will immediately follow
    // the one that we are about to transmit.
    ////////////////////////////////////////////////////////////////////////////
	if(pdTRUE == ret )
	{
		ptNextPkt = _trp;
		if(ptNextPkt  > _twp)
			maxsz = (UINT24)((INT8*)ptNextPkt - (INT8*)_twp);
		else
			maxsz = (UINT24)(((INT8*)_bp - (INT8*)_twp) + ((INT8*) ptNextPkt - (INT8*)_tlbp));

		if(usBufLen < maxsz)
		{
			stats.txsz += usPktLen;

			ptNextPkt = (_tSEMAC_DescTbl*) (((UINT8*)_twp) + usBufLen);
			if((INT8*)ptNextPkt >= (INT8*)_bp) // if this is the last buffer space,
			{                            	   // roll back to start of Tx buffer
				ptNextPkt = (_tSEMAC_DescTbl*)((UINT8*)_tlbp + ((UINT8*)ptNextPkt - (UINT8*)_bp));
			}

			////////////////////////////////////////////////////////////////////////////
			// Prevent the EMAC in sending the next frame by setting status of the
			// next packet to HOST OWNS.
			////////////////////////////////////////////////////////////////////////////
			ptNextPkt->stat = HOST_OWNS;
			ptNextPkt->np   = 0;
			ptNextPkt->pktsz= 0;
			////////////////////////////////////////////////////////////////////////////
			// Copy the enet packet into the Tx buffer. If the data will wrap around
			// the end of the Tx buffer, make sure to handle it properly.
			////////////////////////////////////////////////////////////////////////////
			pDst = (UINT8*)_twp + DESC_OVH;
			WrapLen = (UINT8*)_bp - pDst;
			if( WrapLen >= usPktLen )
			{    // No wrap
				memcpy((UINT8*)pDst, pkt, usPktLen );
			}
			else
			{    // Wrap
				memcpy((UINT8*)pDst, pkt, WrapLen);
				memcpy((UINT8*)_tlbp, &pkt[WrapLen], (usPktLen-WrapLen));
			}

			////////////////////////////////////////////////////////////////////////////
			// Generate entries for the Tx buffer descriptor of the current frame
			////////////////////////////////////////////////////////////////////////////
			_twp->np = ptNextPkt;    // np points to the predetermined packet
			_twp->pktsz = usPktLen;  // total length of enet frame
			////////////////////////////////////////////////////////////////////////////
			// Start txing the frame by setting status to EMAC owns.
			////////////////////////////////////////////////////////////////////////////
			_twp->stat = EMAC_OWNS;

			// Enable/Restart poll timer
			// EMAC_PTMR = 1;

			////////////////////////////////////////////////////////////////////////////
			// Move the transmit write pointer to the next packet buffer space.
			////////////////////////////////////////////////////////////////////////////
//			dumpDescTbl("twp",_twp);
			_twp = ptNextPkt;
		}
		else
		{
			printf("Tx no eth buff\n");
			stats.txover++;
			ret = pdFALSE;
		}
		xSemaphoreGive(txsem);
	}

    return ret;
}

BaseType_t xNetworkInterfaceOutput( NetworkBufferDescriptor_t * const pxDescriptor, BaseType_t xReleaseAfterSend)
{
	BaseType_t res = pdFAIL;
	if(!++nwout) ++nwout;
	if(pxDescriptor->xDataLength <= (ETHPKT_MAXLEN))  // if valid length
    {
        res = ETH_SendNow(pxDescriptor->pucEthernetBuffer, pxDescriptor->xDataLength);    // send now
    }

	if(xReleaseAfterSend != pdFALSE)
		vReleaseNetworkBufferAndDescriptor( pxDescriptor );

    return res;
}

/* Defined by the application code, but called by FreeRTOS+TCP when the network
connects/disconnects (if ipconfigUSE_NETWORK_EVENT_HOOK is set to 1 in
FreeRTOSIPConfig.h). */
void vApplicationIPNetworkEventHook( eIPCallbackEvent_t eNetworkEvent )
{
	uint32_t ulIPAddress, ulNetMask, ulGatewayAddress, ulDNSServerAddress;
	static BaseType_t xTasksAlreadyCreated = pdFALSE;
	FreeRTOS_debug_printf(( "NetworkEventHook eNetworkEvent:%i\n", eNetworkEvent));

    /* Check this was a network up event, as opposed to a network down event. */
    if( eNetworkEvent == eNetworkUp )
    {
        /* Create the tasks that use the TCP/IP stack if they have not already been
        created. */
        if( xTasksAlreadyCreated == pdFALSE )
        {
               /*
             * Create the tasks here.
             */

            xTasksAlreadyCreated = pdTRUE;
        }

        /* The network is up and configured.  Print out the configuration,
        which may have been obtained from a DHCP server. */
        FreeRTOS_GetAddressConfiguration( &ulIPAddress,
                                          &ulNetMask,
                                          &ulGatewayAddress,
                                          &ulDNSServerAddress );

        /* Convert the IP address to a string then print it out. */
        FreeRTOS_debug_printf(( "IP Address:            %lxip\n", FreeRTOS_htonl(ulIPAddress )));

        /* Convert the net mask to a string then print it out. */
        FreeRTOS_debug_printf(( "Subnet Mask:           %lxip\n", FreeRTOS_htonl(ulNetMask )));

        /* Convert the IP address of the gateway to a string then print it out. */
        FreeRTOS_debug_printf(( "Gateway IP Address:    %lxip\n", FreeRTOS_htonl(ulGatewayAddress )));

        /* Convert the IP address of the DNS server to a string then print it out. */
        FreeRTOS_debug_printf(( "DNS server IP Address: %lxip\n", FreeRTOS_htonl(ulDNSServerAddress )));
    }

}
// End of file
