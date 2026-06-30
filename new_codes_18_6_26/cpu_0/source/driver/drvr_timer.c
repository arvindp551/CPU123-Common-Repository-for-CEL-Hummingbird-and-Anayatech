#include "drvr_timer.h"
//#include "xtimerctr.h"
#include "xtmrctr.h"


/*Timer Constructor*/
void timer_ctor(timer_struct_t* handle) {
   //uint8_t timer_num;// = handle->id & 0x1; // get timer counter number
   //uint8_t timer_id;// = handle->id >> 1;   // timer id
   uint32_t opts=0;

   // id : 1 -> timer0,unit0
   // id : 2 -> timer0,unit1
   // id : 3 -> timer1,unit0
   // id : 4 -> timer1,unit1

   switch (handle->id){
   case 1: handle->timer_num = 0; handle->timer_ch = 0; break;
   case 2: handle->timer_num = 0; handle->timer_ch = 1; break;
   case 3: handle->timer_num = 1; handle->timer_ch = 0; break;
   case 4: handle->timer_num = 1; handle->timer_ch = 1; break;
   case 5: handle->timer_num = 2; handle->timer_ch = 0; break;
   case 6: handle->timer_num = 2; handle->timer_ch = 1; break;
   default: handle->timer_num = 0; handle->timer_ch = 0;
   }

   if(handle->initDone) return;
   XTmrCtr_Initialize(&handle->inst, (handle->id));

  // opts = XTmrCtr_GetOptions(&handle->inst, handle->timer_ch);
      if (handle->dir == TIMER_DIR_UP)
   	   opts &= ~XTC_CSR_DOWN_COUNT_MASK;
      else
   	   opts |= XTC_CSR_DOWN_COUNT_MASK;

      if (handle->onePulse)
   	   opts &= ~XTC_CSR_AUTO_RELOAD_MASK;
      else
   	   opts |= XTC_CSR_AUTO_RELOAD_MASK;

      if (handle->interruptFlags) opts |= XTC_CSR_ENABLE_INT_MASK;
      else opts &= ~XTC_CSR_ENABLE_INT_MASK;
      XTmrCtr_SetHandler(&handle->inst, handle->cb_handler, handle);

     // XTmrCtr_SetOptions(handle->inst, handle->timer_ch, opts);
      XTmrCtr_WriteReg(handle->inst.BaseAddress, handle->timer_ch,XTC_TCSR_OFFSET,opts);
      if(handle->dir == TIMER_DIR_DOWN)  XTmrCtr_SetResetValue(&handle->inst, handle->timer_ch, handle->period);
      else  XTmrCtr_SetResetValue(&handle->inst, handle->timer_ch, (0xFFFFFFFF - handle->period));
      handle->initDone = 1;
}

/*This Function is used to start timer, has required two parameter
 * Param1 - handle of timer instance
 * param2 - timer_index refer which timer needs to start
 * */
void timer_start(timer_struct_t* handle,uint8_t timer_index) {
  //XTmrCtr_Start(handle->inst,timer_index);
	  XTmrCtr_Start(&handle->inst,handle->timer_ch);
}
#if 1
/*This Function is used to stop timer, has required two parameter
 * Param1 - handle of timer instance
 * param2 - timer_index refer which timer needs to stop
 * */
void timer_stop(timer_struct_t* handle,uint8_t timer_index) {
  XTmrCtr_Stop(&handle->inst,handle->timer_ch);
}

/*This Function is used to set timer period, has required two parameter
 * Param1 - handle of timer instance
 * param2 - time period
 * */

void timer_setPeriod(timer_struct_t* handle, uint32_t period) {

   if(handle->dir == TIMER_DIR_DOWN)
	   XTmrCtr_SetResetValue(&handle->inst, handle->timer_ch, period);
   else
	   XTmrCtr_SetResetValue(&handle->inst, handle->timer_ch, (0xFFFFFFFF - handle->period));
   handle->period = period;
}

/*This Function is used to get timer count, has required a parameter
 * Param1 - handle of timer instance
 * return - current count of timer
 * */
uint32_t timer_getCount(timer_struct_t* handle) {
  uint32_t data;
   data = XTmrCtr_GetValue(&handle->inst, handle->timer_ch);
  return data;
}

/*This function is used to check timer is expired or not*/
uint32_t timer_getStatus(timer_struct_t* handle) {
    uint32_t data;
  data =  XTmrCtr_IsExpired(&handle->inst, handle->timer_ch);
  return data;
}

/*This Function is used to clear timer status*/
void timer_clearStatus(timer_struct_t* handle) {
  uint32_t opts;
  opts = XTmrCtr_GetOptions(&handle->inst, handle->timer_ch);
  opts |= XTC_CSR_INT_OCCURED_MASK;

  XTmrCtr_SetOptions(&handle->inst, handle->timer_ch, opts);
}
#endif
