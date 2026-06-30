
 /**
 ******************************************************************************
 * @file    gpio.c - Driver for the GPIO.
 * @brief   A GPIO driver.
 * @author  Anayatech Application Team
 * @date    16-March-2018
 * @bug     No Known bugs.
 * @copyright Anayatech Systems Pvt. Ltd. 
 ******************************************************************************
 *
 **/

/******************************************************************************
 *
 *! \addtogroup GPIO_driver
 *! @{
 *
 **/


#include "drvr_gpio.h"

//*****************************************************************************
//
//! Constructure for the GPIO.
//! 
//! \param handle: where handle can be a select the GPIO peripheral
//!
//! \return None
//
//*****************************************************************************
uint32_t gpio_d[5];
void gpio_ctor(gpio_struct_t* handle) {
  //GPIO_InitTypeDef gpioStructure;
  //uint32_t gpioBase;
  uint8_t i;

//
// Determine the AHB Peripheral Clock for the gpio
//
  switch(handle->port) {
  case 0:
	 // RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
    handle->gpioBase = GPIOA_BASE;
    break;
  case 1:
	 // RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
    handle->gpioBase = GPIOB_BASE;
    break;
  case 2:
	 // RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);
    handle->gpioBase = GPIOC_BASE;
    break;
  case 3:
	 // RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
    handle->gpioBase = GPIOA_BASE+0x8;
    break;
  }

//
// Determine the PIN for the gpio
//
 // gpioStructure.GPIO_Pin = (0x1 << handle->pin);
  
//
// Determine the Direction for the gpio
//
/*
  if(handle->dir == GPIO_DIR_OUT) 
    gpioStructure.GPIO_Mode = GPIO_Mode_OUT;
  else if(handle->dir == GPIO_DIR_IN) 
    gpioStructure.GPIO_Mode = GPIO_Mode_IN;
	else if (handle->dir == GPIO_DIR_AF) 
		gpioStructure.GPIO_Mode = GPIO_Mode_AF;
	else if (handle->dir == GPIO_DIR_AN) 
		gpioStructure.GPIO_Mode = GPIO_Mode_AN;

//
// gpio config for alternate channel
//	

 if(handle->dir == GPIO_DIR_AF)
	GPIO_PinAFConfig((GPIO_TypeDef*)gpioBase,handle->pin,handle->af); 

//
// Determine the output Type for the gpio
//
	if(handle->outType == OPENDRAIN) 
    gpioStructure.GPIO_OType = GPIO_OType_OD;
  else if(handle->outType == PUSHPULL) 
    gpioStructure.GPIO_OType = GPIO_OType_PP; 
//
// Determine the pull-up/pull-down for the gpio
//
	if(handle->pullType == PULLUP) 
    gpioStructure.GPIO_PuPd = GPIO_PuPd_UP;
  else if(handle->pullType == PULLDOWN) 
    gpioStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
  else if(handle->pullType == PULLNONE) 
    gpioStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;

  gpioStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init((GPIO_TypeDef*)gpioBase, &gpioStructure);
*/	
	for(i = 0; i < MAX_PWM_ENTRIES; i++) {
		handle->pwmPattern[i].repeats = 0;
	}
	handle->sequence_repeat = 0;
	handle->pwm_index = 0;
	handle->event = 0;
  //gpio_pwm_clear(handle);	
}

//*****************************************************************************
//
//! set the oupput high for the selected gpio.
//! 
//! \param handle: where handle can be a select the GPIO peripheral
//! \param delay : privide delay for the gpio
//!
//! \return None
//
//*****************************************************************************
void gpio_set(gpio_struct_t* handle, uint16_t delay) {
  if(handle->delay != NULL && delay > 0) {
    handle->delay->after = delay;
    handle->delay->state = 1;
  }
  else {
   if(handle->polarity){ // inverted
		 GPIO_CLR(handle);
	 }
	 else {
		 GPIO_SET(handle);
	 }
  }
}

//*****************************************************************************
//
//! set the output low for the selected gpio.
//! 
//! \param handle: where handle can be a select the GPIO peripheral
//! \param delay : privide delay for the gpio
//!
//! \return None
//
//*****************************************************************************
void gpio_clr(gpio_struct_t* handle, uint16_t delay) {
  if(handle->delay != NULL && delay > 0) {
    handle->delay->after = delay;
    handle->delay->state = 0;
  }
  else {
   if(handle->polarity){ // inverted
		 GPIO_SET(handle);
	 }
	 else{
		 GPIO_CLR(handle);
	 }
  }
}

//*****************************************************************************
//
//! this function handles the delay/debounce time for the selected GPIO 
//! 
//! \param handle: where handle can be a select the GPIO peripheral
//!
//! \return None
//
//*****************************************************************************
void gpio_tick(gpio_struct_t* handle) {
  uint32_t gpioBase;
  uint16_t gpio_pin;
	uint8_t val;
	uint8_t i=0;
	gpio_pwm_struct_t* pPWM;
  gpio_delay_struct_t* pDELAY;

	gpioBase = handle->gpioBase;
  gpio_pin = (0x1 << handle->pin);
  pDELAY = handle->delay;

  // delay soluld work only when dir is OUT
  if(handle->delay != NULL) {
    if(handle->delay->after != 0) handle->delay->after--;
    else {
			if(handle->polarity){ // inverted
				if (handle->delay->state) gpio_clr(handle,0);
				else gpio_set(handle,0);
			}
			else{
				if (handle->delay->state) gpio_set(handle,0);
				else gpio_clr(handle,0);
			}
			if(handle->pwm_now.repeats == 0 && (handle->sequence_repeat > 0 || handle->sequence_repeat == REPEAT_INFINITE)) { // get next pattern
			  for(i = handle->pwm_index; i < MAX_PWM_ENTRIES; i++) {
			  	pPWM = &handle->pwmPattern[i];
					if(i == MAX_PWM_ENTRIES - 1)
						handle->pwm_index = 0;
					else
					  handle->pwm_index = i+1;
					if(pPWM->repeats == 0) continue;
			  	handle->pwm_now.ONticks = pPWM->ONticks;
			  	handle->pwm_now.OFFticks = pPWM->OFFticks;
			  	handle->pwm_now.repeats = pPWM->repeats;
					break;
			  }
			}
			if(handle->sequence_repeat != REPEAT_INFINITE && handle->sequence_repeat > 0 && i == MAX_PWM_ENTRIES)
				handle->sequence_repeat--;

			if(handle->pwm_now.repeats > 0 || handle->pwm_now.repeats == REPEAT_INFINITE) {
				// if ONticks zero then wait for OFFticks
        if(!pDELAY->state && handle->pwm_now.ONticks != 0) {// gpio low -> drive high and then keep high for delay 
					gpio_set(handle,0);
					gpio_set(handle,handle->pwm_now.ONticks);
			  }
				else {// gpio high -> drive low and keep low for delay 
					gpio_clr(handle,0);
					gpio_clr(handle,handle->pwm_now.OFFticks);
					if(handle->pwm_now.repeats > 0) handle->pwm_now.repeats--;
				}
			}
		}
	}

  if(handle->dir == GPIO_DIR_OUT) return;

	// debounce should work when dir is IN
  val = (GPIO_GET(handle) == Bit_SET);
	if(handle->polarity) val = !val;
  if(handle->dbounce != NULL) {
    if(val != handle->dbounce->last) 
      handle->dbounce->countDown = handle->dbounce->period;
      handle->dbounce->last = val;
    if(handle->dbounce->countDown != 0) handle->dbounce->countDown--;
    else {
			if(val && !handle->dbounce->state) handle->event = 1; //Lo -> Hi transition event latched
			handle->dbounce->state = val; 
		}
  }
}

//*****************************************************************************
//
//! toggle the current output with respect to delay for the selected GPIO. 
//! 
//! \param handle: where handle can be a select the GPIO peripheral
//! \param delay:  where delay can be a time for the selected GPIO.
//!
//! \return None
//
//*****************************************************************************

void gpio_toggle(gpio_struct_t* handle, uint16_t delay) {
  uint32_t gpioBase;
  uint16_t gpio_pin;
	uint8_t  val;
  gpioBase =handle->gpioBase;

  gpio_pin = (0x1 << handle->pin);
  val = GPIO_GET(handle);
	val = !val;
  if(handle->delay != NULL && delay > 0) {
    handle->delay->after = delay;
    handle->delay->state = val;
  }
  else{
	 if (val) {
		 GPIO_SET(handle);
	 }
	 else {
		 GPIO_CLR(handle);	
	 } 
	}
}

//*****************************************************************************
//
//! read the state for the selected GPIO. 
//! 
//! \param handle: where handle can be a select the GPIO peripheral
//!
//! \return None
//
//*****************************************************************************
uint8_t gpio_read(gpio_struct_t* handle) {
  uint32_t gpioBase;
  uint16_t gpio_pin;
	uint8_t val=0;

  gpioBase =handle->gpioBase;
  gpio_pin = (0x1 << handle->pin);
  if(handle->dir == GPIO_DIR_OUT)
    val = (GPIO_GET(handle) == Bit_SET);
  else if(handle->dir == GPIO_DIR_IN) {
    if(handle->dbounce != NULL) val = handle->dbounce->state; 
    else {
			val = (GPIO_GET(handle) == Bit_SET);
			if(handle->polarity) val = !val;
		}

  }
	
  return val;
}

//*****************************************************************************
//
//!  add PWM pattern 
//! 
//! \param handle: where handle can be a select the GPIO peripheral
//! \param index:    pattern number to be updated
//! \param ONticks:  High period
//! \param OFFticks: Low period
//! \param repeats:  number of times pattern to be repeated before going to next one
//!
//! \return None
//
//*****************************************************************************
void gpio_pwm_add(gpio_struct_t* handle,uint8_t index, uint16_t ONticks, uint16_t OFFticks, int8_t repeats) {
	if(index > MAX_PWM_ENTRIES-1) return; //error
  handle->pwmPattern[index].ONticks = ONticks; 
  handle->pwmPattern[index].OFFticks = OFFticks; 
  handle->pwmPattern[index].repeats = repeats; 
}

void gpio_pwm_clear(gpio_struct_t* handle){
	int i;
	for(i = 0; i < MAX_PWM_ENTRIES; i++) {
		handle->pwmPattern[i].repeats = 0;
	}
	handle->sequence_repeat = 0;
	handle->pwm_index = 0;
}
//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
