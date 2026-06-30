
#ifndef __DRVR_UUART_H__
#define __DRVR_UUART_H__

#include "common_header.h"
#include "fifo.h"
#include "usci_uart.h"

 typedef struct {
	// user setting
   uint32_t   baudRate;
   fifo_t*    prx;
   fifo_t*    ptx;
   uint8_t    inten;
   uint8_t    port;

	// set internally
   UUART_T*    UARTx;
 } uuart_struct_t;

 void uuart_ctor(uuart_struct_t* handle);
 uint8_t uuart_write(uuart_struct_t* handle);
 uint8_t uuart_read(uuart_struct_t* handle);

#endif
