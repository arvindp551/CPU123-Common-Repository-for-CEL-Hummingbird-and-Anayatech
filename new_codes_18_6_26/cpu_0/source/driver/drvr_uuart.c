#include "common_header.h"
#include "drvr_uuart.h"
#include "fifo.h"

void uuart_ctor(uuart_struct_t* handle) {
 UUART_T* UARTx;

 switch(handle->port) {
		case 0:
			UARTx = UUART0;
    		/* Select UART module clock source as HIRC and UART module clock divider as 1 */
    	//	CLK_SetModuleClock(USCI0_MODULE, CLK_CLKSEL1_USCI0SEL_HIRC, CLK_CLKDIV0_UART0(1));
    		/* Enable UART peripheral clock */
    		CLK_EnableModuleClock(USCI0_MODULE);

			/*---------------------------------------------------------------------------------------------------------*/
			/* Init I/O Multi-function                                                                                 */
			/*---------------------------------------------------------------------------------------------------------*/
			/* Set multi-function pins for UART RXD and TXD */
    		SYS->GPD_MFPH &= ~(SYS_GPD_MFPL_PD1MFP_Msk | SYS_GPD_MFPL_PD2MFP_Msk);
    		SYS->GPD_MFPH |= (SYS_GPD_MFPL_PD1MFP_USCI0_DAT0 | SYS_GPD_MFPL_PD2MFP_USCI0_DAT1);
			break;
	}

	UUART_Open(UARTx,handle->baudRate);

	handle->UARTx = UARTx; 
}

uint8_t uuart_write(uuart_struct_t* handle) {
	UUART_T* UARTx = handle->UARTx;
  uint8_t ch;
	
	if(handle->ptx == NULL) return 1;
	// TX UART FIFO has space and USER TX FIFO has data
	while(UUART_IS_TX_FULL(UARTx) == 0 && handle->ptx->is_empty == 0) {
		fifo_read(handle->ptx,&ch);
  	UUART_Write(UARTx, &ch, 1);
	}
	return 0;
}

uint8_t uuart_read(uuart_struct_t* handle) {
	UUART_T* UARTx = handle->UARTx;
  uint8_t ch;

	if(handle->prx == NULL) return 1;
	// RX UART FIFO has data and USER RX FIFO has space
	while(UUART_GET_RX_EMPTY(UARTx) == 0 && handle->prx->is_full == 0) {
  	UUART_Read(UARTx, &ch, 1);
		fifo_write(handle->prx,ch);
	}
	return 0;
}
