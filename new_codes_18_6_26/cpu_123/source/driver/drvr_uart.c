#include "common_header.h"
#include "drvr_uart.h"
#include "fifo.h"

void uart_ctor(uart_struct_t* handle) {
 UART_T* UARTx;
 uint8_t UARTx_IRQn;


 switch(handle->port) {
		case 0:
			UARTx = UART0;
    		/* Select UART module clock source as HIRC and UART module clock divider as 1 */
    		CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HIRC, CLK_CLKDIV0_UART0(1));
    		/* Enable UART peripheral clock */
    		CLK_EnableModuleClock(UART0_MODULE);
			UARTx_IRQn = UART0_IRQn;

			/*---------------------------------------------------------------------------------------------------------*/
			/* Init I/O Multi-function                                                                                 */
			/*---------------------------------------------------------------------------------------------------------*/
			/* Set multi-function pins for UART RXD and TXD */
    		SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB13MFP_Msk | SYS_GPB_MFPH_PB12MFP_Msk);
    		SYS->GPB_MFPH |= (SYS_GPB_MFPH_PB13MFP_UART0_TXD | SYS_GPB_MFPH_PB12MFP_UART0_RXD);
#if 0			
			/* Enable UART Module clock */
			CLK->APBCLK0 |= CLK_APBCLK0_UART0CKEN_Msk;
			/* Select UART Module clock source */
			CLK->CLKSEL1 = (CLK->CLKSEL1 & (~CLK_CLKSEL1_UART0SEL_Msk)) | CLK_CLKSEL1_UART0SEL_HIRC;		
			/*---------------------------------------------------------------------------------------------------------*/
			/* Init I/O Multi-function                                                                                 */
			/*---------------------------------------------------------------------------------------------------------*/
			/* Set multi-function pins for UART RXD and TXD */
    		SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB13MFP_Msk | SYS_GPB_MFPH_PB12MFP_Msk);
    		SYS->GPB_MFPH |= (SYS_GPB_MFPH_PB13MFP_UART0_TXD | SYS_GPB_MFPH_PB12MFP_UART0_RXD);

			/* Select UART function */
			UARTx->FUNCSEL = UART_FUNCSEL_UART;
			/* Set UART line configuration */
			UARTx->LINE = UART_WORD_LEN_8 | UART_PARITY_NONE | UART_STOP_BIT_1;
			/* Set UART Rx and RTS trigger level */
			UARTx->FIFO = UART_FIFO_RFITL_14BYTES | UART_FIFO_RTSTRGLV_14BYTES;
			/* Set UART baud rate */
			UARTx->BAUD = (UART_BAUD_MODE2 | UART_BAUD_MODE2_DIVIDER(__HIRC, handle->baudRate));
			/* Set time-out interrupt comparator */
			UARTx->TOUT = (UARTx->TOUT & ~UART_TOUT_TOIC_Msk) | (0x40);
#endif
			break;
		case 1:
			UARTx = UART1;
    		/* Select UART module clock source as HIRC and UART module clock divider as 1 */
    		CLK_SetModuleClock(UART1_MODULE, CLK_CLKSEL1_UART1SEL_HIRC, CLK_CLKDIV0_UART1(1));
    		/* Enable UART peripheral clock */
    		CLK_EnableModuleClock(UART1_MODULE);
			UARTx_IRQn = UART1_IRQn;
			/*---------------------------------------------------------------------------------------------------------*/
			/* Init I/O Multi-function                                                                                 */
			/*---------------------------------------------------------------------------------------------------------*/
			/* Set multi-function pins for UART RXD and TXD */
    		SYS->GPB_MFPL &= ~(SYS_GPB_MFPL_PB3MFP_Msk | SYS_GPB_MFPL_PB2MFP_Msk);
    		SYS->GPB_MFPL |= (SYS_GPB_MFPL_PB3MFP_UART1_TXD | SYS_GPB_MFPL_PB2MFP_UART1_RXD);

#if 0
			/* Enable UART Module clock */
			CLK->APBCLK0 |= CLK_APBCLK0_UART1CKEN_Msk;
			/* Select UART Module clock source */
			CLK->CLKSEL1 = (CLK->CLKSEL1 & (~CLK_CLKSEL1_UART1SEL_Msk)) | CLK_CLKSEL1_UART1SEL_HIRC;		
			/*---------------------------------------------------------------------------------------------------------*/
			/* Init I/O Multi-function                                                                                 */
			/*---------------------------------------------------------------------------------------------------------*/
			/* Set multi-function pins for UART RXD and TXD */
    		SYS->GPB_MFPL &= ~(SYS_GPB_MFPL_PB3MFP_Msk | SYS_GPB_MFPL_PB2MFP_Msk);
    		SYS->GPB_MFPL |= (SYS_GPB_MFPL_PB3MFP_UART1_TXD | SYS_GPB_MFPL_PB2MFP_UART1_RXD);

			/* Select UART function */
			UARTx->FUNCSEL = UART_FUNCSEL_UART;
			/* Set UART line configuration */
			UARTx->LINE = UART_WORD_LEN_8 | UART_PARITY_NONE | UART_STOP_BIT_1;
			/* Set UART Rx and RTS trigger level */
			UARTx->FIFO = UART_FIFO_RFITL_14BYTES | UART_FIFO_RTSTRGLV_14BYTES;
			/* Set UART baud rate */
			UARTx->BAUD = (UART_BAUD_MODE2 | UART_BAUD_MODE2_DIVIDER(__HIRC, handle->baudRate));
			/* Set time-out interrupt comparator */
			UARTx->TOUT = (UARTx->TOUT & ~UART_TOUT_TOIC_Msk) | (0x40);
#endif
			break;
		case 2:
			UARTx = UART2;
    		/* Select UART module clock source as HIRC and UART module clock divider as 1 */
    		CLK_SetModuleClock(UART2_MODULE, CLK_CLKSEL3_UART2SEL_HIRC, CLK_CLKDIV4_UART2(1));
    		/* Enable UART peripheral clock */
    		CLK_EnableModuleClock(UART2_MODULE);
			UARTx_IRQn = UART2_IRQn;
			/*---------------------------------------------------------------------------------------------------------*/
			/* Init I/O Multi-function                                                                                 */
			/*---------------------------------------------------------------------------------------------------------*/
			/* Set multi-function pins for UART RXD and TXD */
    		SYS->GPB_MFPL &= ~(SYS_GPB_MFPL_PB1MFP_Msk | SYS_GPB_MFPL_PB0MFP_Msk);
    		SYS->GPB_MFPL |= (SYS_GPB_MFPL_PB1MFP_UART2_TXD | SYS_GPB_MFPL_PB0MFP_UART2_RXD);
#if 0
			/* Enable UART Module clock */
			CLK->APBCLK0 |= CLK_APBCLK0_UART2CKEN_Msk;
			/* Select UART Module clock source */
			CLK->CLKSEL1 = (CLK->CLKSEL1 & (~CLK_CLKSEL3_UART2SEL_Msk)) | CLK_CLKSEL3_UART2SEL_HIRC;		
			/*---------------------------------------------------------------------------------------------------------*/
			/* Init I/O Multi-function                                                                                 */
			/*---------------------------------------------------------------------------------------------------------*/
			/* Set multi-function pins for UART RXD and TXD */
    		SYS->GPB_MFPL &= ~(SYS_GPB_MFPL_PB1MFP_Msk | SYS_GPB_MFPL_PB0MFP_Msk);
    		SYS->GPB_MFPL |= (SYS_GPB_MFPL_PB1MFP_UART2_TXD | SYS_GPB_MFPL_PB0MFP_UART2_RXD);

			/* Select UART function */
			UARTx->FUNCSEL = UART_FUNCSEL_UART;
			/* Set UART line configuration */
			UARTx->LINE = UART_WORD_LEN_8 | UART_PARITY_NONE | UART_STOP_BIT_1;
			/* Set UART Rx and RTS trigger level */
			UARTx->FIFO = UART_FIFO_RFITL_14BYTES | UART_FIFO_RTSTRGLV_14BYTES;
			/* Set UART baud rate */
			UARTx->BAUD = (UART_BAUD_MODE2 | UART_BAUD_MODE2_DIVIDER(__HIRC, handle->baudRate));
			/* Set time-out interrupt comparator */
			UARTx->TOUT = (UARTx->TOUT & ~UART_TOUT_TOIC_Msk) | (0x40);
#endif
			break;
		case 3:
			UARTx = UART3;
    		/* Select UART module clock source as HIRC and UART module clock divider as 1 */
    		CLK_SetModuleClock(UART3_MODULE, CLK_CLKSEL3_UART3SEL_HIRC, CLK_CLKDIV4_UART3(1));
    		/* Enable UART peripheral clock */
    		CLK_EnableModuleClock(UART3_MODULE);
			UARTx_IRQn = UART3_IRQn;
			/*---------------------------------------------------------------------------------------------------------*/
			/* Init I/O Multi-function                                                                                 */
			/*---------------------------------------------------------------------------------------------------------*/
			/* Set multi-function pins for UART RXD and TXD */
    		SYS->GPC_MFPL &= ~(SYS_GPC_MFPL_PC3MFP_Msk | SYS_GPC_MFPL_PC2MFP_Msk);
    		SYS->GPC_MFPL |= (SYS_GPC_MFPL_PC3MFP_UART3_TXD | SYS_GPC_MFPL_PC2MFP_UART3_RXD);
#if 0
			/* Enable UART Module clock */
			CLK->APBCLK0 |= CLK_APBCLK0_UART3CKEN_Msk;
			/* Select UART Module clock source */
			CLK->CLKSEL1 = (CLK->CLKSEL1 & (~CLK_CLKSEL3_UART3SEL_Msk)) | CLK_CLKSEL3_UART3SEL_HIRC;		
			/*---------------------------------------------------------------------------------------------------------*/
			/* Init I/O Multi-function                                                                                 */
			/*---------------------------------------------------------------------------------------------------------*/
			/* Set multi-function pins for UART RXD and TXD */
    		SYS->GPC_MFPL &= ~(SYS_GPC_MFPL_PC3MFP_Msk | SYS_GPC_MFPL_PC2MFP_Msk);
    		SYS->GPC_MFPL |= (SYS_GPC_MFPL_PC3MFP_UART3_TXD | SYS_GPC_MFPL_PC2MFP_UART3_RXD);

			/* Select UART function */
			UARTx->FUNCSEL = UART_FUNCSEL_UART;
			/* Set UART line configuration */
			UARTx->LINE = UART_WORD_LEN_8 | UART_PARITY_NONE | UART_STOP_BIT_1;
			/* Set UART Rx and RTS trigger level */
			UARTx->FIFO = UART_FIFO_RFITL_14BYTES | UART_FIFO_RTSTRGLV_14BYTES;
			/* Set UART baud rate */
			UARTx->BAUD = (UART_BAUD_MODE2 | UART_BAUD_MODE2_DIVIDER(__HIRC, handle->baudRate));
			/* Set time-out interrupt comparator */
			UARTx->TOUT = (UARTx->TOUT & ~UART_TOUT_TOIC_Msk) | (0x40);
#endif
			break;
		case 4:
			UARTx = UART4;
    		/* Select UART module clock source as HIRC and UART module clock divider as 1 */
    		CLK_SetModuleClock(UART4_MODULE, CLK_CLKSEL3_UART4SEL_HIRC, CLK_CLKDIV4_UART4(1));
    		/* Enable UART peripheral clock */
    		CLK_EnableModuleClock(UART4_MODULE);
			UARTx_IRQn = UART4_IRQn;
			/*---------------------------------------------------------------------------------------------------------*/
			/* Init I/O Multi-function                                                                                 */
			/*---------------------------------------------------------------------------------------------------------*/
			/* Set multi-function pins for UART RXD and TXD */
    		SYS->GPA_MFPL &= ~(SYS_GPA_MFPL_PA3MFP_Msk | SYS_GPA_MFPL_PA2MFP_Msk);
    		SYS->GPA_MFPL |= (SYS_GPA_MFPL_PA3MFP_UART4_TXD | SYS_GPA_MFPL_PA2MFP_UART4_RXD);

#if 0
			/* Enable UART Module clock */
			CLK->APBCLK0 |= CLK_APBCLK0_UART4CKEN_Msk;
			/* Select UART Module clock source */
			CLK->CLKSEL1 = (CLK->CLKSEL1 & (~CLK_CLKSEL3_UART4SEL_Msk)) | CLK_CLKSEL3_UART4SEL_HIRC;		
			/*---------------------------------------------------------------------------------------------------------*/
			/* Init I/O Multi-function                                                                                 */
			/*---------------------------------------------------------------------------------------------------------*/
			/* Set multi-function pins for UART RXD and TXD */
    		SYS->GPA_MFPL &= ~(SYS_GPA_MFPL_PA3MFP_Msk | SYS_GPA_MFPL_PA2MFP_Msk);
    		SYS->GPA_MFPL |= (SYS_GPA_MFPL_PA3MFP_UART4_TXD | SYS_GPA_MFPL_PA2MFP_UART4_RXD);

			/* Select UART function */
			UARTx->FUNCSEL = UART_FUNCSEL_UART;
			/* Set UART line configuration */
			UARTx->LINE = UART_WORD_LEN_8 | UART_PARITY_NONE | UART_STOP_BIT_1;
			/* Set UART Rx and RTS trigger level */
			UARTx->FIFO = UART_FIFO_RFITL_14BYTES | UART_FIFO_RTSTRGLV_14BYTES;
			/* Set UART baud rate */
			UARTx->BAUD = (UART_BAUD_MODE2 | UART_BAUD_MODE2_DIVIDER(__HIRC, handle->baudRate));
			/* Set time-out interrupt comparator */
			UARTx->TOUT = (UARTx->TOUT & ~UART_TOUT_TOIC_Msk) | (0x40);
#endif
			break;
		case 5:
			UARTx = UART5;
    		/* Select UART module clock source as HIRC and UART module clock divider as 1 */
    		CLK_SetModuleClock(UART5_MODULE, CLK_CLKSEL3_UART5SEL_HIRC, CLK_CLKDIV4_UART5(1));
    		/* Enable UART peripheral clock */
    		CLK_EnableModuleClock(UART5_MODULE);
			UARTx_IRQn = UART5_IRQn;
			/*---------------------------------------------------------------------------------------------------------*/
			/* Init I/O Multi-function                                                                                 */
			/*---------------------------------------------------------------------------------------------------------*/
			/* Set multi-function pins for UART RXD and TXD */
    		SYS->GPA_MFPL &= ~(SYS_GPA_MFPL_PA5MFP_Msk | SYS_GPA_MFPL_PA4MFP_Msk);
    		SYS->GPA_MFPL |= (SYS_GPA_MFPL_PA5MFP_UART5_TXD | SYS_GPA_MFPL_PA4MFP_UART5_RXD);

#if 0
			/* Enable UART Module clock */
			CLK->APBCLK0 |= CLK_APBCLK0_UART5CKEN_Msk;
			/* Select UART Module clock source */
			CLK->CLKSEL1 = (CLK->CLKSEL1 & (~CLK_CLKSEL3_UART5SEL_Msk)) | CLK_CLKSEL3_UART5SEL_HIRC;		
			/*---------------------------------------------------------------------------------------------------------*/
			/* Init I/O Multi-function                                                                                 */
			/*---------------------------------------------------------------------------------------------------------*/
			/* Set multi-function pins for UART RXD and TXD */
    		SYS->GPA_MFPL &= ~(SYS_GPA_MFPL_PA5MFP_Msk | SYS_GPA_MFPL_PA4MFP_Msk);
    		SYS->GPA_MFPL |= (SYS_GPA_MFPL_PA5MFP_UART5_TXD | SYS_GPA_MFPL_PA4MFP_UART5_RXD);

			/* Select UART function */
			UARTx->FUNCSEL = UART_FUNCSEL_UART;
			/* Set UART line configuration */
			UARTx->LINE = UART_WORD_LEN_8 | UART_PARITY_NONE | UART_STOP_BIT_1;
			/* Set UART Rx and RTS trigger level */
			UARTx->FIFO = UART_FIFO_RFITL_14BYTES | UART_FIFO_RTSTRGLV_14BYTES;
			/* Set UART baud rate */
			UARTx->BAUD = (UART_BAUD_MODE2 | UART_BAUD_MODE2_DIVIDER(__HIRC, handle->baudRate));
			/* Set time-out interrupt comparator */
			UARTx->TOUT = (UARTx->TOUT & ~UART_TOUT_TOIC_Msk) | (0x40);
#endif
			break;
	}

	UART_Open(UARTx,handle->baudRate);
	/* Set UART NVIC */
	if(handle->inten) {
		NVIC_SetPriority(UARTx_IRQn, 2);
		NVIC_EnableIRQ(UARTx_IRQn);
		/* Enable tim-out counter, Rx tim-out interrupt and Rx ready interrupt */
		UARTx->INTEN = (UART_INTEN_TOCNTEN_Msk | UART_INTEN_RXTOIEN_Msk | UART_INTEN_RDAIEN_Msk);
	}
	else
		NVIC_DisableIRQ(UARTx_IRQn);

	handle->UARTx = UARTx; 
}

uint8_t uart_write(uart_struct_t* handle) {
	UART_T* UARTx = handle->UARTx;
  uint8_t ch;
	
	if(handle->ptx == NULL) return 1;
	// TX UART FIFO has space and USER TX FIFO has data
	while(UART_IS_TX_FULL(UARTx) == 0 && handle->ptx->is_empty == 0) {
		fifo_read(handle->ptx,&ch);
  	UART_Write(UARTx, &ch, 1);
	}
	return 0;
}

uint8_t uart_read(uart_struct_t* handle) {
	UART_T* UARTx = handle->UARTx;
  uint8_t ch;

	if(handle->prx == NULL) return 1;
	// RX UART FIFO has data and USER RX FIFO has space
	while(UART_GET_RX_EMPTY(UARTx) == 0 && handle->prx->is_full == 0) {
  	UART_Read(UARTx, &ch, 1);
		fifo_write(handle->prx,ch);
	}
	return 0;
}
