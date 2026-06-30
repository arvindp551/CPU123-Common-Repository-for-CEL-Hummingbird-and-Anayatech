
#include "drvr_spi.h"
#include "stdarg.h"

#define	SS_LOW_1  SPI_SET_SS_LOW(handle->SPIx)
#define	SS_LOW_2  PC4 = 0
#define	SS_LOW_3  PC5 = 0
#define	SS_HIGH_1 SPI_SET_SS_HIGH(handle->SPIx)
#define	SS_HIGH_2 PC4 = 1
#define	SS_HIGH_3 PC5 = 1

uint8_t spi_ctor(spi_struct_t* handle) {
	SPI_T* SPIx;
	uint8_t SPIx_IRQn;

	handle->todo = SPI_NONE;
	handle->complete = 1;
	handle->state = SPI_IDLE;

	switch (handle->port) {
	case 2:
		SPIx = SPI2;

    	/* Select PCLK as the clock source of SPI */
    	CLK_SetModuleClock(SPI2_MODULE, CLK_CLKSEL2_SPI2SEL_PCLK1, MODULE_NoMsk);
    	/* Enable SPI peripheral clock */
    	CLK_EnableModuleClock(SPI2_MODULE);

   		/*---------------------------------------------------------------------------------------------------------*/
   		/* Init I/O Multi-function                                                                                 */
   		/*---------------------------------------------------------------------------------------------------------*/
   		/* Setup SPI1 multi-function pins */
   		SYS->GPA_MFPH &= ~(SYS_GPA_MFPH_PA15MFP_Msk | SYS_GPA_MFPH_PA14MFP_Msk | SYS_GPA_MFPH_PA13MFP_Msk | SYS_GPA_MFPH_PA12MFP_Msk);
	   	SYS->GPA_MFPH |= (SYS_GPA_MFPH_PA15MFP_SPI2_MOSI | SYS_GPA_MFPH_PA14MFP_SPI2_MISO | SYS_GPA_MFPH_PA13MFP_SPI2_CLK | SYS_GPA_MFPH_PA12MFP_SPI2_SS);
	   	/* Enable SPI2 clock pin (PA13) schmitt trigger */
	   	PA->SMTEN |= GPIO_SMTEN_SMTEN13_Msk;

		if(handle->is_master) {	
			// Slave Select Output settings
			GPIO_SetMode(PC, BIT5, GPIO_MODE_OUTPUT); // CPU3SS
			SS_HIGH_3;
		}
		SPIx_IRQn = SPI2_IRQn;
		break;
	case 1:
		SPIx = SPI1;

    	/* Select PCLK as the clock source of SPI */
    	CLK_SetModuleClock(SPI1_MODULE, CLK_CLKSEL2_SPI1SEL_PCLK0, MODULE_NoMsk);
    	/* Enable SPI peripheral clock */
    	CLK_EnableModuleClock(SPI1_MODULE);

	   	/*---------------------------------------------------------------------------------------------------------*/
	   	/* Init I/O Multi-function                                                                                 */
	   	/*---------------------------------------------------------------------------------------------------------*/
	   	/* Setup SPI1 multi-function pins */
	   	SYS->GPA_MFPL &= ~(SYS_GPA_MFPL_PA7MFP_Msk | SYS_GPA_MFPL_PA6MFP_Msk);
	   	SYS->GPA_MFPL |= (SYS_GPA_MFPL_PA7MFP_SPI1_CLK | SYS_GPA_MFPL_PA6MFP_SPI1_SS);
	   	SYS->GPC_MFPL &= ~(SYS_GPC_MFPL_PC7MFP_Msk | SYS_GPC_MFPL_PC6MFP_Msk);
	   	SYS->GPC_MFPL |= (SYS_GPC_MFPL_PC7MFP_SPI1_MISO | SYS_GPC_MFPL_PC6MFP_SPI1_MOSI);
	   	/* Enable SPI1 clock pin (PA7) schmitt trigger */
	   	PA->SMTEN |= GPIO_SMTEN_SMTEN7_Msk;
	
		if(handle->is_master) {
			GPIO_SetMode(PC, BIT4, GPIO_MODE_OUTPUT); // CPU2SS
			SS_HIGH_2;
		}
		SPIx_IRQn = SPI1_IRQn;
	}

	if(handle->inten)
   		NVIC_EnableIRQ(SPIx_IRQn);
	else
   		NVIC_DisableIRQ(SPIx_IRQn);

   	/* Set Tx and Rx FIFO threshold and enable FIFO mode. */
	SPI_SetFIFO(SPIx,4,4);
   	/* Configure SPI as a master, MSB first, 8-bit transaction, SPI Mode-0 timing, clock is 12MHz */

   	
   	if(handle->is_master) {
   		SPI_Open(SPIx, SPI_MASTER, SPI_MODE_0, 8, 1200000);
		/* Disable auto SS function, control SS signal manually. */
   		SPI_DisableAutoSS(SPIx);
  	}
	else
   		SPI_Open(SPIx, SPI_SLAVE, SPI_MODE_0, 8, 12000000);

	handle->SPIx = SPIx;
	return 0;
}

// IDLE: check if read or write command
// WAIT: if read send 0xFF on MOSI goto BUSY
// WAIT: if write send data from USER FIFO on MOSI goto BUSY
// BUSY: if data sent and rcvd push rcvd data from MISO to USER FIFO goto WAIT
// goto IDLE when all data sent and read
void spi_master_sm(spi_struct_t* handle) {
	uint8_t ch;
	
	switch (handle->state) {
		case SPI_IDLE:
			// check for new request
			if(!(handle->todo == SPI_WRITE || handle->todo == SPI_READ))
				return;
			handle->complete = 0;
			// /CS: active
			switch (handle->agent_id) {
				case 0:
					SS_LOW_1;
					break;
				case 1:	
					SS_LOW_2;
					break;
				case 2:	
					SS_LOW_3;
					break;
			}
			// configure transaction length as 8 bits
			SPI_SET_DATA_WIDTH(handle->SPIx, 8);
			handle->state = SPI_WAIT;
			break;
		case SPI_WAIT:
			// check busy
			if (SPI_IS_BUSY(handle->SPIx)) return;
			if(handle->ptx->is_empty) {
				handle->state = SPI_IDLE;
				handle->complete = 1;
				handle->todo = SPI_NONE;
				switch (handle->agent_id) {
					case 0:
						SS_HIGH_1;
						break;
					case 1:	
						SS_HIGH_2;
						break;
					case 2:	
						SS_HIGH_3;
						break;
				}
			}
			else {
				if(handle->todo == SPI_WRITE) {
					fifo_read_isr(handle->ptx,&ch);
					while(SPI_GET_RX_FIFO_EMPTY_FLAG(handle->SPIx) == 0)
						SPI_READ_RX(handle->SPIx);
					
					SPI_WRITE_TX(handle->SPIx,ch);
				}
				else if(handle->todo == SPI_READ) {
					fifo_read_isr(handle->ptx,&ch);
					SPI_WRITE_TX(handle->SPIx,ch);
				}
				handle->state = SPI_BUSY;
			}
			break;

		case SPI_BUSY:
			// USER RX FIFO has space
			if(handle->prx->is_full == 0 && SPI_GET_RX_FIFO_EMPTY_FLAG(handle->SPIx) == 0 ) {
				ch = SPI_READ_RX(handle->SPIx);
				fifo_write_isr(handle->prx,ch);
				handle->state = SPI_WAIT;
			} 
			break;

		case SPI_ERROR: // do nothing
			break;
		
		default:;
	}
}

// 1. SPI master first writes command with data and deassert SS
// 2. SPI master waits for say 1ms
// 3. SPI master then reads data from last requested register by sending 0xFF bytes 
void spi_agent_sm(spi_struct_t* handle) {
	SPI_T* SPIx = handle->SPIx;
  uint8_t ch;
	
	switch (handle->state) {
		case SPI_IDLE:
			// Check Slave Select Active Status
			if(SPIx->STATUS & SPI_STATUS_SSACTIF_Msk) {
				// Clear Slave Select Active Status
				SPIx->STATUS |= SPI_STATUS_SSACTIF_Msk;
				handle->state = SPI_READ_FIFO;
			}
			else break;
		//break; let it continue 
		case SPI_READ_FIFO:
			while(SPI_GET_RX_FIFO_EMPTY_FLAG(SPIx) == 0) {
				ch = SPI_READ_RX(handle->SPIx);
				fifo_write(handle->prx,ch);
			}
			if(SPIx->STATUS & SPI_STATUS_SSINAIF_Msk) {
				// Clear Slave Select Inactive Status
				SPIx->STATUS |= SPI_STATUS_SSINAIF_Msk;
            // process received data and generate reply data
				handle->cb(handle);
				// clear RX FIFO and TX FIFO
				SPIx->FIFOCTL |= (SPI_FIFOCTL_RXFBCLR_Msk | SPI_FIFOCTL_TXFBCLR_Msk);
				handle->state = SPI_WRITE_FIFO;
			}
			else return;			// let it goto next state, no break!
		case SPI_WRITE_FIFO:
			// write data to SPI TX FIFO
			while(handle->ptx->is_empty == 0) {
				if(SPI_GET_TX_FIFO_FULL_FLAG(handle->SPIx) == 0) {
					fifo_read(handle->ptx,&ch);
					SPI_WRITE_TX(handle->SPIx,ch);
				}
				else return; //prevent blocking
			}
			handle->state = SPI_IDLE;
			break;
		case SPI_ERROR: // do nothing
			break;
		
		default:;
	}
}

void spi_master_read(spi_struct_t* handle, uint8_t agent_id) {
	handle->complete = 0;
	handle->agent_id = agent_id;
	handle->todo = SPI_READ;
}

void spi_master_write(spi_struct_t* handle, uint8_t agent_id) {
	handle->complete = 0;
	handle->agent_id = agent_id;
	handle->todo = SPI_WRITE;
}
