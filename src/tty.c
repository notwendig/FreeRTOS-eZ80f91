#include "stdint.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "tty.h"
#include "_ez80.h"

TtyBlk_t tty =
{ -1,
{ 115200, 8, 1, PARNONE, SERSET_RTSCTS | SERSET_DTR_ON, 10000/*portMAX_DELAY*/ },
   0, 0, 0,	UART0, 0, 0, 0, 0 };

void ttyisr()
{
	char iir = UART_IIR(tty.UARTbase);
	char ier = UART_IER(tty.UARTbase);
	char spr = (iir << 4) | ier;
	UART_IER(tty.UARTbase) = 0;
	UART_SPR(tty.UARTbase) = spr;
	xTaskNotifyFromISR( tty.IntTask, 0, eIncrement, 0);
}

int tty_putch(int c)
{
	if(!intrap() && taskSCHEDULER_NOT_STARTED != xTaskGetSchedulerState())
	{
		uint8_t ier = UART_IER(tty.UARTbase);
		BaseType_t r = xQueueSend(tty.TxQueue, &c, pdMS_TO_TICKS(100));
		if (!(ier & IER_TIE))
			UART_IER(tty.UARTbase) |= IER_TIE;
		return r == pdTRUE ? c : -1;
	}

	while(!(UART0_LSR & (1 << 5)))
		asm(" nop");
	
	UART0_THR = c;
	return c;
}

int tty_getch()
{
	int c = 0;
	xQueueReceive(tty.RxQueue, &c, portMAX_DELAY);
	return c;
}

static uint8_t RxQueueStorageArea[configRXQUEUE_LENGTH];
static uint8_t TxQueueStorageArea[configTXQUEUE_LENGTH];
static StaticQueue_t RxStaticQueue;
static StaticQueue_t TxStaticQueue;

void TtyTask(TtyBlk_t *pDevBlk)
{
	UINT8 uart_base = tty.UARTbase;
	volatile UINT8 ier;
	volatile UINT8 lsr;
	volatile UINT8 msr;
	UINT8 ch;
	UINT8 do_cts_check = tty.serparam.settings & SERSET_RTSCTS;
	UINT8 do_dcd_check = !(tty.serparam.settings & SERSET_IGNHUP);
	UINT8 TxFlowOn = pdTRUE;

	lsr = UART_LSR(tty.UARTbase);
	msr = UART_MSR(tty.UARTbase);
	ier = UART_IIR(tty.UARTbase);

	do
	{
		while (!ulTaskNotifyTake(pdFALSE, tty.serparam.timeout))
		{
			tty.livesign++;
		}
		portENTER_CRITICAL();
		ier = UART_SPR( uart_base );
		ier	|= UART_IER(uart_base);
		lsr = UART_LSR(uart_base);
		msr = UART_MSR(uart_base);

		if (lsr & LSR_OE)
		{
			tty.overruns++;
		}

		if (ier & (IIR_RDR << 4))
		{
			if (!(lsr & LSR_DR))
			{
				/*
				 * In odd situations the Uart will trigger an Rx interrupt,
				 * but the LSR indicates that no data is available...
				 *
				 * The problem seems most likely to occur in cases where
				 * the UART interrupts were disabled while the remote was
				 * sending data.
				 *
				 * This situation will immediately clear itself once the
				 * remote sends some data.  However, we can't be sure
				 * this will ever happen.  Therefore to clear this condition,
				 * we momentarily put the Uart in loopback mode and transmit
				 * a single character.
				 */
				UART_MCTL(uart_base) |= MCTL_LOOP;
				UART_THR(uart_base) = 'Z';
				lsr = UART_LSR(uart_base);
				while (!(lsr & LSR_DR))
				{
					/*
					 * CR# 2168
					 * Wait until data reaches the RBR before emptying FIFO
					 */
					lsr = UART_LSR(uart_base);
				}
				while (lsr & LSR_DR)
				{
					// Toss everything we read - should just be 1 byte.
					lsr = UART_RBR(uart_base);
					lsr = UART_LSR(uart_base);
				}
				UART_MCTL( uart_base ) &= ~MCTL_LOOP;
				tty.uart_rdr_no_rd++;
			}
		}
		ier &= 0x1F;

		if (lsr & LSR_DR)
		{
			while (lsr & LSR_DR)
			{
				ch = UART_RBR(uart_base);

				if ((lsr & (LSR_BI | LSR_FE | LSR_PE | LSR_OE | LSR_DR))
						== LSR_DR)
				{
					if (!uxQueueSpacesAvailable(
							tty.RxQueue) || xQueueSend(tty.RxQueue, (void*)&ch,0) != pdPASS)
					{
						tty.spill_count++;
					}
				}
				lsr = UART_LSR(uart_base);
			}

			/*
			 * If the Rx buffer is close to filling, flow off
			 */
			if (do_cts_check)
			{
				if (uxQueueSpacesAvailable(tty.RxQueue) <= FLOW_OFF_LEVEL)
				{
					UART_MCTL(uart_base) &= ~MCTL_RTS;
				}
				else
				{
					UART_MCTL(uart_base) |= MCTL_RTS;
				}
			}
		}
		
		if ((lsr & LSR_THRE) && (ier & IER_TIE))
		{

			if (TxFlowOn == pdTRUE)
			{
				uint16_t TxCount = UART_FIFODEPTH;
				uint16_t tx = 0;
				while (TxCount--)
				{
					if(xQueueReceive(tty.TxQueue, &ch, 0) == pdTRUE)
						UART_THR(uart_base) =ch;
					else
						break;
					tx++;
				}
				if (!tx)
					ier &= ~IER_TIE;

			}
		}

		if ((msr & MSR_DDCD) && !(msr & MSR_DCD) && (do_dcd_check))
		{
			ier = 0;
		}

		if (do_cts_check)
		{
			if (msr & MSR_DCTS)
			{
				if (msr & MSR_CTS)
				{
					TxFlowOn = pdTRUE;
					ier |= IER_TIE;
				}
				else
				{
					TxFlowOn = pdFALSE;
				}
			}
		}

		UART_IER(uart_base) |= ier;
		portEXIT_CRITICAL();
	} while (1);
}

void UART_setparams()
{
	uint16_t bgen; /* baud rate generator valu */
	uint8_t  tmp = LCTL_5DATABITS;


	/* Set the baud rate */
	UART_LCTL(tty.UARTbase) |= LCTL_DLAB; /* DLAB */
    bgen = configCPU_CLOCK_HZ/tty.serparam.baud;
    bgen = (bgen + 8) >> 4;
	BRG_DLRL(tty.UARTbase) = (UINT8)bgen;
	BRG_DLRH(tty.UARTbase) = (bgen&0xff00)>>8;
	UART_LCTL(tty.UARTbase) &= ~LCTL_DLAB; /* DLAB */

	switch( tty.serparam.databits ) 
	{
		case 6:
			tmp|=LCTL_6DATABITS;
			break;
		case 7:
			tmp|=LCTL_7DATABITS;
			break;
		case 8:
			tmp|=LCTL_8DATABITS;		
			break;
		default:
			;
	}

	switch( tty.serparam.stopbits ) 
	{
		case 1:
			break;
		case 2:
			tmp|=LCTL_2STOPBITS;
			break;
		default:
			;
	}

	switch( tty.serparam.parity ) 
	{
		case PAREVEN:
			tmp|=(LCTL_PEN|LCTL_EPS);
			break;
		case PARODD:
			tmp|=(LCTL_PEN);
			break;
		case PARNONE:
			break;
		default:
			;
	}
	/* Set the port attributes */
	UART_LCTL(tty.UARTbase)=tmp;
}


BaseType_t TTYInit()
{
	BaseType_t res = pdFAIL;
	
	if (tty.State == (uint8_t) -1)
	{
		res = xTaskCreate(TtyTask, "TtyTask", 512, &tty, 7, &tty.IntTask);
		
		PD_DDR  |= 0x03;				//! Set the bits corresponding to the pins in DDR and ALT2 and reset corresponding bits in ALT1.
		PD_ALT1 &= (0xFF ^ 0x03);
		PD_ALT2 |= 0x03;

		if(res == pdPASS)
		{
			uint8_t ier = IER_RIE;
			
			UART_IER(tty.UARTbase)  = 0;
			UART_LCTL(tty.UARTbase) = 0;
			UART_MCTL(tty.UARTbase) = 0;
			UART_setparams();
			
			tty.RxQueue = xQueueCreateStatic(configRXQUEUE_LENGTH, 1, RxQueueStorageArea, &RxStaticQueue);
			tty.TxQueue = xQueueCreateStatic(configTXQUEUE_LENGTH, 1, TxQueueStorageArea, &TxStaticQueue);
			
			/* Set up and flush the FIFOs */
			UART_FCTL(tty.UARTbase) = FCTL_TRIG_8 | FCTL_FIFOEN | FCTL_CLRTXF | FCTL_CLRRXF;

			/* Flow control: tell sender we're ready for data */
			if (tty.serparam.settings & SERSET_RTSCTS)
			{
				ier |= IER_MIIE;
				UART_MCTL(tty.UARTbase) |= MCTL_RTS;
			}
			else
			{
				/* Enable modem status to detect changes in DCD */
				if (!(tty.serparam.settings & SERSET_IGNHUP))
				{
					ier |= IER_MIIE;
				}
			}

			if (tty.serparam.settings & SERSET_DTR_ON)
			{
				UART_MCTL(tty.UARTbase) |= MCTL_DTR;
			}

			/*
			 * Install the interrupt vector and create the interrupt task.
			 */

			set_vector(UART0_IVECT, ttyisr);
			UART_IER(tty.UARTbase) = ier;

			tty.State = 0;
		}
	}
	return pdTRUE;
}
