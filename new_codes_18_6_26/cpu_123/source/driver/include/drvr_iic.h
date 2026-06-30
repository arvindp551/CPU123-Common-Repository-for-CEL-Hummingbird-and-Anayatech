#ifndef DRVR_IIC_H__
#define DRVR_IIC_H__

 #include "hal.h"
 #include "stdint.h"
 #include "xiic.h"

// #define XIIC_SR_RX_FIFO_EMPTY_MASK    0x40
// #define XIIC_SR_TX_FIFO_EMPTY_MASK    0x80
// #define XIIC_SR_RX_FIFO_FULL_MASK     0x20
// #define XIIC_SR_TX_FIFO_FULL_MASK    0x10
// #define XIIC_SR_BUS_BUSY_MASK         0x04
// #define XIIC_SR_ADDR_AS_SLAVE_MASK    0x02
// #define XIIC_SR_MSTR_RDING_SLAVE_MASK 0x08

 #define IIC_ISRXFEMPTY(status)  (status & XIIC_SR_RX_FIFO_EMPTY_MASK) /*RX Empty Fifo status*/
 #define IIC_ISTXFEMPTY(status)  (status & XIIC_SR_TX_FIFO_EMPTY_MASK) /*TX Empty Fifo status*/
 #define IIC_ISRXFFULL(status)   (status & XIIC_SR_RX_FIFO_FULL_MASK)  /*RX FULL Fifo status*/
 #define IIC_ISTXFFULL(status)   (status & XIIC_SR_TX_FIFO_FULL_MASK)  /*TX FULL Fifo status*/
 #define IIC_ISBUSBSY(status)    (status & XIIC_SR_BUS_BUSY_MASK)      /*Check Bus Busy Status*/
 #define IIC_ISAAS(status)       (status & XIIC_SR_ADDR_AS_SLAVE_MASK) /*Check Address as slave*/
 #define IIC_ISRDREQ(status)     (status & XIIC_SR_MSTR_RDING_SLAVE_MASK) /* slave read(1) / write(0)*/

/*IIC state machine state*/
 typedef enum {
   IDLE,
   READ,
   WRITE,
   START_READ,
   WRITE_READ_ADDR,
   SET_REPEATED_START,
   SET_REPEATED_START_SIZE,
   START_WRITE,
   WAIT_DONE
 } iic_states_e;

 typedef void (*iic_callback) (void *callbackRef);

 // currently polled mode is implemented
 typedef struct {
   uint8_t bytes;     /*Number of Bytes to be transfer*/
   uint8_t regaddr;   /*Register address for read and write command*/
   uint8_t* data;     /*pointer for data read and write*/
   uint8_t indx;
   char cmd_type; /*IIC command type 'W' for write and 'R' for read*/
   uint8_t is_master; /*Master or slave field if this field is 1 then iic is configured as master else slave*/
   uint8_t interruptFlags; // not used
   iic_states_e state;  /*IIC state*/
   uint8_t slaveaddr;  /*Should be set to Slave Device address << 1*/
   uint8_t repstart;   /*Set repeated start generally in master read*/
   uint8_t wr_complete;/*Master write complete indication*/
   uint8_t rd_complete; /*Master Reada complete indication*/
   // callback
   void   (*write_cb)(void*,unsigned int); /*Master write complete call back*/
   void   (*read_cb)(void*,unsigned int);  /*Master read complete call back*/
   void*   st_intr;                 /*Status interrupt for interrupt callback*/
   XIic    InstancePtr;             /*Xlinx iic strcture */
   uint16_t id;                     /*IIC index for iic core please refer xparameters.h file for indx */
   uint8_t  timeout;
   uint8_t  time_counting;
   uint8_t  error;
   uint8_t  tot_byte;
	 uint8_t buffer[16];
 } iic_struct_t;

 /* IIC constructor */
 void iic_ctor (iic_struct_t* handle);
 /* IIC slave state machine */
 void iic_slave_sm(iic_struct_t* handle);
 /* IIC master state machine */
 void iic_master_sm(iic_struct_t* handle);
 void iic_write(iic_struct_t* handle, uint8_t reg, uint8_t bytes, uint8_t* data);
 void iic_read(iic_struct_t* handle, uint8_t reg, uint8_t bytes, uint8_t* data);

#endif
