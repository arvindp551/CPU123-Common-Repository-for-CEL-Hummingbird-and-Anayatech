#include "drvr_iic.h"
#include "stdio.h"

 #define IIC_WRITE 0x00
 #define IIC_READ  0x01



/* IIC constructor */
 void iic_ctor (iic_struct_t* handle) {
   handle->bytes    = 0;        // number of bytes to read or write
   handle->regaddr  = 0;        // register address of slave
   //handle->data[2]  = 0;      // byte array where data is temporarily stored
   handle->indx    = 0;         // byte count and index of byte array
   handle->state    = IDLE;     // current state of master state machine;
   handle->cmd_type = 'N';
   handle->wr_complete = 1;
   handle->rd_complete = 1;
	 handle->time_counting = 0;
 /*
  *  Initializes a specific IIC instance.
  */
   XIic_Initialize(&handle->InstancePtr, handle->id);
   XIic_Reset(&handle->InstancePtr);
   //XIic_SetAddress(&handle->InstancePtr, XII_ADDR_TO_RESPOND_TYPE, handle->slaveaddr); // write own address
   //XIic_CfgInitialize(&IicInstance, ConfigPtr,ConfigPtr->BaseAddress);

   if(!handle->is_master){
	   XIic_WriteReg(handle->InstancePtr.BaseAddress, XIIC_ADR_REG_OFFSET, handle->slaveaddr);
   }

   if(handle->interruptFlags ==1){

	   XIic_WriteReg(handle->InstancePtr.BaseAddress, XIIC_DGIER_OFFSET, 0x80000000); //
	   XIic_WriteReg(handle->InstancePtr.BaseAddress, XIIC_IIER_OFFSET, 0x50); //
	   XIic_WriteReg(handle->InstancePtr.BaseAddress, XIIC_RFD_REG_OFFSET, 0x01); //
	   //XIic_SetRecvHandler(&handle->InstancePtr, handle ,handle->read_cb);
     //XIic_SetSendHandler(&handle->InstancePtr, handle ,handle->write_cb);
     XIic_SetStatusHandler(&handle->InstancePtr, handle ,handle->st_intr);
   }

   XIic_Start(&handle->InstancePtr);
 }

void iic_slave_sm(iic_struct_t* handle) {
  uint32_t status;
  static uint8_t index = 0;
  /*
   * checking iic is configured as master or as slave.
   * This state machine is for iic as slave so when iic
   * is in master then return from this function.
   * */
  if (handle->is_master) return;

	status = XIic_ReadReg(handle->InstancePtr.BaseAddress, XIIC_SR_REG_OFFSET);
	//cr = XIic_ReadReg(handle->InstancePtr.BaseAddress, XIIC_CR_REG_OFFSET);

	/*During IIC Transaction IIC bus is always busy when
	 * iic bus is released it means transaction is completed*/
	if(IIC_ISBUSBSY(status)== 0)
		handle->state = IDLE;
  switch(handle->state) {
    case IDLE:
      if (IIC_ISAAS(status)) { /*new request from master*/
        /* checking new IIC request is for write?? */
        if (!IIC_ISRXFEMPTY(status)) { // data received from slave
          if (IIC_ISBUSBSY(status)) {   // check if handle bus is busy -> sck low
	          handle->regaddr = XIic_ReadReg(handle->InstancePtr.BaseAddress, XIIC_DRR_REG_OFFSET); /*read received data*/
            #ifdef IIC_SLAVE_PRINT_ON
	          xil_printf("R -> %02x ",handle->regaddr);
            #endif
            handle->state = WRITE;
            handle->indx = 0;
          }
        }
        /* checking new IIC request is for read?? */
        else if (IIC_ISRDREQ(status)) {  // slave read with no repeated start. use regaddr written before this transaction
          // call function that fills data array or passes a pointer based on regaddr
          //handle->read_cb;
          handle->state = READ;
        }
      }
      break;

    case WRITE:
    	status = XIic_ReadReg(handle->InstancePtr.BaseAddress, XIIC_SR_REG_OFFSET);
    	/*checking transaction is completed or not*/
    	 if (!IIC_ISBUSBSY(status)){
    		 handle->state = IDLE;
             #ifdef IIC_SLAVE_PRINT_ON
    		  xil_printf("\r\n");
             #endif
    	 }

      if (IIC_ISAAS(status)) {
        if (!IIC_ISRXFEMPTY(status)) { // if rx fifo empty -> no write from slave
                 	handle->data[handle->indx++] = XIic_ReadReg(handle->InstancePtr.BaseAddress, XIIC_DRR_REG_OFFSET);
            #ifdef IIC_SLAVE_PRINT_ON
        	  xil_printf("D ->%02x ",temp);
            #endif
        }
        else if (IIC_ISRDREQ(status)) { // read request with repeated start
          handle->state = READ;

        }
      }
      else if (!IIC_ISBUSBSY(status)) {
        handle->state = IDLE;
        handle->rd_complete = 1;
        /* this functoin is called when iic slace write transaction is completed*/
        handle->write_cb(handle,0);
      }
      break;

    case READ:
      if (IIC_ISBUSBSY(status)) {
        if (IIC_ISTXFEMPTY(status)){
          XIic_WriteReg(handle->InstancePtr.BaseAddress, XIIC_DTR_REG_OFFSET, handle->data[handle->indx++]); // set iic enable
           #ifdef IIC_SLAVE_PRINT_ON
        	  xil_printf("W ->%02x ",handle->data[index]);
            #endif
        	XIic_WriteReg(handle->InstancePtr.BaseAddress, XIIC_DTR_REG_OFFSET, handle->data[index++]); // set iic enable
        }
     }
     else{
    	 handle->state = IDLE;
    	 index = 0;
    	 XIic_WriteReg(handle->InstancePtr.BaseAddress, XIIC_CR_REG_OFFSET, XIIC_CR_TX_FIFO_RESET_MASK); // set iic enable
    	 XIic_WriteReg(handle->InstancePtr.BaseAddress, XIIC_CR_REG_OFFSET, XIIC_CR_ENABLE_DEVICE_MASK); // set iic enable
    	 handle->read_cb(handle,0);
	 }

	  case WAIT_DONE:
      if (IIC_ISBUSBSY(status)) 
       handle->state = IDLE;

	default: break;
  }
}

 /* IIC master state machine */
 void iic_master_sm(iic_struct_t* handle) {
   uint32_t status=0;
   uint32_t cr=0,d=0;
   static  uint8_t cnt = 0; // not used
   if (!handle->is_master) return; /*do nothing if slave*/
   status = XIic_ReadReg(handle->InstancePtr.BaseAddress, XIIC_SR_REG_OFFSET);
   cr = XIic_ReadReg(handle->InstancePtr.BaseAddress, XIIC_CR_REG_OFFSET);
	 if(handle->time_counting != 0)  handle->time_counting--;
	 if(handle->time_counting == 1) {
      cr &= ~XIIC_CR_ENABLE_DEVICE_MASK;
      XIic_WriteReg(handle->InstancePtr.BaseAddress, XIIC_CR_REG_OFFSET, cr);
      handle->state = IDLE;
      handle->wr_complete = 1;
      handle->rd_complete = 1;
      handle->error = 1;
	 }
   switch(handle->state) {
     case IDLE: // do nothing
         handle->tot_byte = handle->bytes;   /*Copy of transfer byte size*/
    	 if(handle->cmd_type == 'W' || handle->cmd_type == 'R'){ /*checking */
    	 if(handle->cmd_type == 'W'){   /*checking iiic command is for write command*/
    		 handle->cmd_type = 'C';
    		 handle->state = START_WRITE;
				 handle->time_counting = handle->timeout;
    	 }
    	 else if(handle->cmd_type == 'R') {  /*checking iiic command is for read command*/
    		 handle->cmd_type = 'C';
    		 handle->state = START_READ;
				 handle->time_counting = handle->timeout;
    	 }
    	   XIic_WriteReg(handle->InstancePtr.BaseAddress, XIIC_CR_REG_OFFSET,XIIC_CR_TX_FIFO_RESET_MASK);/*reset iic fifo*/
    	   cr |= XIIC_CR_ENABLE_DEVICE_MASK;
    	   /*Enable iic core*/
    	   XIic_WriteReg(handle->InstancePtr.BaseAddress, XIIC_CR_REG_OFFSET, cr);
    	   /*write slave address with start condition*/
      	   XIic_WriteReg(handle->InstancePtr.BaseAddress, XIIC_DTR_REG_OFFSET, (0x100 |handle->slaveaddr | IIC_WRITE));
    	 }
    	 handle->error = 0;
         break;

     case START_WRITE:
    	 /*checking IIC TX FIFO is empty*/
         if (IIC_ISTXFEMPTY(status)) {
        	 /*transfer register address for data write */
        	 XIic_WriteReg(handle->InstancePtr.BaseAddress, XIIC_DTR_REG_OFFSET, handle->regaddr);
             handle->state = WRITE;
				     handle->time_counting = handle->timeout;
             break;
         }

     case WRITE:
    	 /*checking IIC TX FIFO is empty*/
         if (IIC_ISTXFEMPTY(status)) {
        	/*checking number of byte for transfer is more
        	 * than one byte then single single byte is transfer*/
           if (handle->bytes > 1) {
        	   d = *handle->data++;
        	   XIic_WriteReg(handle->InstancePtr.BaseAddress, XIIC_DTR_REG_OFFSET, d);
				     handle->time_counting = handle->timeout;
              handle->bytes--;
           }
           /*When last byte is going to then send stop condition with transfer data*/
           else if (handle->bytes == 1) {
        	   d = *handle->data++;
        	   XIic_WriteReg(handle->InstancePtr.BaseAddress, XIIC_DTR_REG_OFFSET, (0x200 | d));
               handle->bytes = 0;
               if(handle->write_cb != NULL) handle->write_cb(handle,handle->tot_byte);
               handle->state = WAIT_DONE;
				       handle->time_counting = handle->timeout;
           }
         }
         else if(!IIC_ISBUSBSY(status)) {
            // the slave hasn't acked the request
            handle->wr_complete = 1;
            handle->error = 1;
            handle->state = IDLE; // go back to IDLE and close the request
         }
         break;

     case START_READ:
    	 /*checking transmit fifo is empty or not*/
         if (IIC_ISTXFEMPTY(status)) {
        	 /*cheching repeated start*/
        	 if(handle->repstart)
        		 XIic_WriteReg(handle->InstancePtr.BaseAddress, XIIC_DTR_REG_OFFSET, (handle->regaddr));
        	 else
        		 XIic_WriteReg(handle->InstancePtr.BaseAddress, XIIC_DTR_REG_OFFSET, (0x200 | handle->regaddr));
       	  handle->state = SET_REPEATED_START;

				  handle->time_counting = handle->timeout;
       	  break;
         }

     case SET_REPEATED_START:
          if (IIC_ISTXFEMPTY(status)) {
     		//wr_reg(CR_ADDR,CR_MSMS|CR_EN|CR_RSTA);
        	 /*Send repeatd  start*/
      	   XIic_WriteReg(handle->InstancePtr.BaseAddress, XIIC_CR_REG_OFFSET,(XIIC_CR_MSMS_MASK|
      			                                                              XIIC_CR_REPEATED_START_MASK|
      			                                                              XIIC_CR_ENABLE_DEVICE_MASK));
      	   /*Send start condition with slave addess and read command*/
      	   XIic_WriteReg(handle->InstancePtr.BaseAddress, XIIC_DTR_REG_OFFSET, (0x100 |handle->slaveaddr | IIC_READ));
      	   /*Send stop condition with number of bytes to be read*/
      	   XIic_WriteReg(handle->InstancePtr.BaseAddress, XIIC_DTR_REG_OFFSET, (0x200 | handle->bytes));
           handle->state = READ; //SBSET_REPEATED_SART_SIZE;
				   handle->time_counting = handle->timeout;
      	   break;
         }
         else if(!IIC_ISBUSBSY(status)) {
        	 // the slave hasn't acked the request
        	 handle->state = IDLE; // go back to IDLE and close the request
           handle->error = 1;
        	 handle->rd_complete = 1;
         }
     case READ:
    	 /*Reading single byte */
    	 if(handle->bytes > 0){
			 cnt++;
			 if(!IIC_ISRXFEMPTY(status)) {
				 handle->data[handle->tot_byte - handle->bytes] = (uint8_t)((XIic_ReadReg(handle->InstancePtr.BaseAddress, XIIC_DRR_REG_OFFSET)) & 0xFF);
				 handle->bytes--;
				 handle->time_counting = handle->timeout;
			 }
			 else if(!IIC_ISBUSBSY(status)) {
			    // the slave hasn't acked the request
			    handle->state = IDLE; // go back to IDLE and close the request
			    handle->error = 1;
			    handle->rd_complete = 1;
			 }
		 }
			 /*When all byte are read then call read calback function*/
		 else{
			  handle->state = WAIT_DONE;
				handle->time_counting = handle->timeout;
			  if(handle->read_cb != NULL) handle->read_cb(handle,handle->tot_byte);
		  }
		  break;

     case WAIT_DONE:
			 // this is to handle IIC state machine stuck condition wherein new trnsaction initiated just after stop.
			 // It is seen that on I2C bus , just before stop condition SCK transitions Hi->Lo and then Lo->Hi once that 
			 // results in I2C IP state machine to go in stuck state 
			 // disable I2C and reenabling brings it out of this state
       if (!IIC_ISBUSBSY(status)) {
				 handle->state = IDLE;
			   handle->rd_complete = 1;
         handle->wr_complete = 1;
			 }
       break;

		default: break;
   }
 }

#if 0
void iic_write(iic_struct_t* handle, int argc, ...) {	
		uint16_t l = 0, i = 0;
		va_list args;                     
		uint8_t data[16]; 
	
	  while(handle->wr_complete == 0 || handle->rd_complete == 0); // wait if another transaction in progress	
		va_start(args, argc);           
		for(i = 0; i < argc; i++) {  
			if(i==0) handle->regaddr = *(va_arg(args,uint8_t*)); 
				else data[i-1] = *(va_arg(args,uint8_t*));
		}
		va_end(args);   
		handle->data = data;
		handle->bytes = argc-1;
		handle->wr_complete = 0;
		handle->repstart = 0;
		handle->cmd_type = 'W';
}
#endif

void iic_write(iic_struct_t* handle, uint8_t reg, uint8_t bytes, uint8_t* data) {	
		handle->regaddr = reg;
		handle->data = data;
		handle->bytes = bytes;
		handle->wr_complete = 0;
		handle->repstart = 0;
		handle->cmd_type = 'W';
}

void iic_read(iic_struct_t* handle, uint8_t reg, uint8_t bytes, uint8_t* data) {	
		handle->regaddr = reg;
		handle->data = data;
		handle->bytes = bytes;
		handle->rd_complete = 0;
		//handle->repstart = 0;
		handle->cmd_type = 'R';
}
