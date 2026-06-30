
#ifndef __DRVR_UART_H__
#define __DRVR_UART_H__

#include "common_header.h"
#include "fifo.h"
#include "uart.h"

 typedef struct {
	// user setting
   uint32_t   baudRate;
   fifo_t*    prx;
   fifo_t*    ptx;
   uint8_t    inten;
   uint8_t    port;

	// set internally
   UART_T*    UARTx;
 } uart_struct_t;

 void uart_ctor(uart_struct_t* handle);
 uint8_t uart_write(uart_struct_t* handle);
 uint8_t uart_read(uart_struct_t* handle);
 uint8_t uart_write_isr(uart_struct_t* handle);
 uint8_t uart_read_isr(uart_struct_t* handle);

#endif
