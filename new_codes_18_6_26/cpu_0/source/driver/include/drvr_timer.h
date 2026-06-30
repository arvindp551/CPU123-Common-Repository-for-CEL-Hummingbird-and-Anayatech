#ifndef DRVR_TIMER_H__
#define DRVR_TIMER_H__

#include "stdint.h"
#include "xtmrctr.h"

#define TIMER_IT_UPDATE_MASK  0x1

#define TIMER_PRESCALER_1     0x1
#define TIMER_DIR_UP          0x0  /*Timer Counting UP Direction*/
#define TIMER_DIR_DOWN        0x1  /*Timer Counting Down Direction*/

/**
 this is done as pasting Token ## doesn't translate constants (#define) but use it's name 
 one additional layer of TIMER_IRQ_HANDLER added to resolve this issue
  */
#define TIMER_IRQ_HANDLER(port,callback,params) TIMER_IRQ_HANDLER__(port,callback,params)
#define TIMER_IRQ_HANDLER__(port,callback,params) \
 void TIM##port##_IRQHandler(void* callbackref, uint8_t timer_num) { \
    __disable_irq();\
    callback(timer_num, params);\
		 __enable_irq();\
 }     

 typedef struct timer_struct{
   uint8_t       id;        /* Xilinx: Keep Id odd for second timer in timer IP*/
   uint16_t      preScalar; /* keep it 1 as no prescalar option in Xilinx IP*/
   uint8_t       dir;       /* DIR = 0 up counter; DIR = 1 down counter */
   uint32_t      period;    /* TIme Period for which timer run*/
   uint8_t       clkdiv;
   uint8_t       onePulse;   /*onePulse = 1 means single shot else multi shot*/
   uint8_t       interruptFlags; /*Setting this field 1 call cb_handler function after timer expires*/
   uint8_t       irqPrio;
   uint8_t       initDone;  /*init done bit is set when timer initialized*/
   uint8_t       timer_num; /*TIMER DEICE ID*/
   uint8_t       timer_ch;  /*TMRCNTR DEICE ID there are two TMRCNTR in 1 timer*/
   void*         cb_handler;  /*Callback handler call when timer expire*/
   XTmrCtr      inst;   // Xilinx timer driver instance
 } timer_struct_t;

 //XTmrCtr_Handler tim_1_IRQHandler;
 //XTmrCtr_Handler tim_2_IRQHandler;
 //XTmrCtr_Handler tim_3_IRQHandler;
 /*Timer Constructor*/
void timer_ctor(timer_struct_t* handle);

/*This Function is used to start timer, has required two parameter
 * Param1 - handle of timer instance
 * param2 - timer_index refer which timer needs to start
 * */
void timer_start(timer_struct_t* handle,uint8_t timer_index);

/*This Function is used to stop timer, has required two parameter
 * Param1 - handle of timer instance
 * param2 - timer_index refer which timer needs to stop
 * */
void timer_stop(timer_struct_t* handle,uint8_t timer_index);

/*This Function is used to set timer period, has required two parameter
 * Param1 - handle of timer instance
 * param2 - time period
 * */
void timer_setPeriod(timer_struct_t* handle, uint32_t period);

/*This Function is used to get timer count, has required a parameter
 * Param1 - handle of timer instance
 * return - current count of timer
 * */
uint32_t timer_getCount(timer_struct_t* handle);

/*This function is used to check timer is expired or not*/
uint32_t timer_getStatus(timer_struct_t* handle);

/*This Function is used to clear timer status*/
void timer_clearStatus(timer_struct_t* handle);
 
#endif /*DRVR_TIMER_H_*/
