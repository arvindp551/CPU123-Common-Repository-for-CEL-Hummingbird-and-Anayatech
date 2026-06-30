#ifndef DRVR_SPI_H__
#define DRVR_SPI_H__

 #include "common_header.h"
 #include "spi.h"
 #include "gpio.h"
 #include "fifo.h"

 #define SPI_WRITE    0U
 #define SPI_READ     1U
 #define XACTION_DONE 2U
 #define SPI_NONE     3U

 #define CODE_SIZE 20810U //bytes
 #define CODE_SIZE_ALIGNED ((CODE_SIZE+2047U)/2048U)*2048U // rounded up and aligned to 2048 

 typedef enum { SPI_IDLE, SPI_BUSY, SPI_WAIT, SPI_ERROR, SPI_READ_FIFO, SPI_WRITE_FIFO } spi_state_e;
 
 typedef struct {
	//user setting
   	fifo_t* ptx;
   	fifo_t* prx;
	uint8_t inten;
   	uint8_t port;
	uint8_t is_master;
   	void   (*cb)(void);

	// set internally
   	uint8_t      todo;
   	spi_state_e  state;
   	SPI_T*       SPIx;
	uint8_t      agent_id;
   	volatile uint8_t  complete;
 } spi_struct_t;
 
 	void spi_ctor(spi_struct_t* handle);
	#ifdef ENABLE_SPI_MASTER
	void spi_master_sm(spi_struct_t* handle);
	void spi_master_write(spi_struct_t* handle, uint8_t agent_id);
	void spi_master_read(spi_struct_t* handle, uint8_t agent_id);
 	//void spi_write_alt(spi_struct_t* handle, int argc, ...);
	#else
	void spi_agent_sm(spi_struct_t* handle);
	#endif
#endif
