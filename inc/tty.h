/*
 * tty.h
 *
 *  Created on: 13.08.2020
 *      Author: juergen
 */

#ifndef EZ80F91_99C0879_TTY_H_
#define EZ80F91_99C0879_TTY_H_

/* Serial Port Settings */
/* Flow control bits */
#define SERSET_DTR_ON	0x8	/* Assert DTR on open, reset it on close */
#define SERSET_RTSCTS	0x4	/* Use RTS/CTS HW flow control */
#define SERSET_DTRDSR	0x2	/* Use DTR/DSR HW flow control */
#define SERSET_XONXOFF	0x1	/* Use XON/XOFF SW flow control */

/* Misc. settings */
#define SERSET_ONLCR	0x10	/* Map NL to CR-NL on output */
#define SERSET_SYNC		0x20	/* Use Synchronous routines instead of interrupts */
#define SERSET_IGNHUP	0x40	/* Ignore Hangup (CD drop) */

#define UART_FIFODEPTH			16
#define FLOW_ON_LEVEL			128
#define UART_RX_BUF_SIZE		4096
#define FLOW_OFF_LEVEL			(UART_RX_BUF_SIZE-(UART_FIFODEPTH<<2))

extern int puts(const char *s);
int tty_putch(int c);
int tty_getch();

typedef enum
{
	SERIAL_STATE_UNINITIALIZED,
	SERIAL_STATE_CLOSED,
	SERIAL_STATE_OPENED
} SERIAL_STATE_E;


/* UART status values */
typedef enum {
	UARTDEV_ERR_SUCCESS = 0,
	UARTDEV_ERR_OVERRUN = -20,
	UARTDEV_ERR_INIT_FAILED,
	UARTDEV_ERR_INVALID_OPERATION,
	UARTDEV_ERR_INVALID_ARGS,
	UARTDEV_ERR_TEMP_BUSY,
	UARTDEV_ERR_KERNEL_ERROR,
	UARTDEV_ERR_INVALID_BAUDRATE,
	UARTDEV_ERR_INVALID_DATABITS,
	UARTDEV_ERR_INVALID_STOPBITS,
	UARTDEV_ERR_INVALID_PARITY_TYPE,
	UARTDEV_ERR_INVALID_PARITY_OP
} UARTDEV_ERR_NUMS ;


typedef enum {
	DISABLE_PARITY,
	ENABLE_PARITY,
	SET_BAUD,
	SET_DATABIT,
	SET_PARITY,
	SET_STOPBITS,
	SET_FIFO_TRIG_LEVEL,
	SET_ALL,
	DISABLE_UART_INTR,
	ENABLE_UART_INTR,
	GET_FIFO_COUNT,
	SET_READ_DELAY /* CR#6571 */

} UARTDEV_IOCTL_FUNC ;

typedef enum { PAREVEN, PARODD, PARNONE } parity_t;

typedef struct TtyParams_s {
	uint24_t	baud;
	uint16_t	databits;
	uint16_t	stopbits;
	parity_t 	parity;
	uint16_t	settings;
	TickType_t  timeout;
}TtyParams_t;


typedef struct	TtyBlk_s
{
	uint8_t			State;
	TtyParams_t 	serparam;
	QueueHandle_t	TxQueue;
	QueueHandle_t	RxQueue;
	TaskHandle_t	IntTask;
	uint8_t			UARTbase;
	uint24_t		overruns;
	uint24_t		spill_count;
	uint24_t		livesign;
	uint24_t		uart_rdr_no_rd;
} TtyBlk_t;


BaseType_t TTYInit();
#endif /* EZ80F91_99C0879_TTY_H_ */
