/**
******************************************************************************
* @file    gpio.h - Driver for the GPIO.
* @brief   A gpio driver.
* @author  Anayatech Application Team
* @date    16-March-2018
* @bug     No Known bugs.
* @copyright Anayatech Systems Pvt. Ltd. 
******************************************************************************
*
**/

#ifndef DRVR_GPIO_H__
#define DRVR_GPIO_H__

 #include "stdint.h"
 #include "xparameters.h"
 #include "hal.h"
 #include "stdio.h"

//*****************************************************************************
//
// Values that can be passed to gpio ctor parameter.
//
//*****************************************************************************
// GPIO set, get and clear macros
// Effective operation, pin offset bit set to (d & mask)
#define GPIO_WRITE(h,d)  gpio_d[h->port] &= ~(1 << h->pin);\
                         gpio_d[h->port] |= ((d & 1) << h->pin);\
                                reg_write(h->gpioBase + 0x8,gpio_d[h->port]);

// Effective operation, pin offset bit set to (mask)
#define GPIO_SET(h)  gpio_d[h->port] |= (1 << h->pin);\
                       reg_write(h->gpioBase + 0x0,gpio_d[h->port]);

// Effective operation, pin offset bit set to (~mask)
#define GPIO_CLR(h)  gpio_d[h->port] &= ~(1 << h->pin);\
                       reg_write(h->gpioBase + 0x0,gpio_d[h->port]);

// Effective operation, pin offset bit toggle if mask=1 else no effect
#define GPIO_TOGGLE(h)  gpio_d[h->port] &= ~(1 << h->pin);\
                       reg_write(h->gpioBase + 0x0,gpio_d[h->port]);

#define GPIO_GET(h)  (uint8_t)((reg_read(h->gpioBase) >> h->pin) & 1)



#define GPIO_DIR_IN   0       /*!< GPIO Direction as Input               */
#define GPIO_DIR_OUT  1       /*!< GPIO Direction as output              */
#define GPIO_DIR_AF   2       /*!< GPIO Direction as Alternate function  */
#define GPIO_DIR_AN   3       /*!< GPIO Direction as Analog Input/Output */
//#define GPIO_ISBUSY(handle) (handle->delay->after != 0)
#define MAX_PWM_ENTRIES 3
#define GPIO_NON_INVERTED 0
#define GPIO_INVERTED 1
#define REPEAT_INFINITE -1 
#define Bit_SET 1
#define GPIOA_BASE	XPAR_GPIO_0_BASEADDR
#define GPIOB_BASE	XPAR_GPIO_1_BASEADDR	//LED
#define GPIOC_BASE	XPAR_GPIO_1_BASEADDR	

//*****************************************************************************
//
// GPIO_Alternate_function_selection_define
//
//*****************************************************************************
 
typedef enum {
AF_0,  /*!< GPIO AF 0 */
AF_1,  /*!< GPIO AF 1 */
AF_2,  /*!< GPIO AF 2 */
AF_3,  /*!< GPIO AF 3 */
AF_4,  /*!< GPIO AF 4 */
AF_5,  /*!< GPIO AF 5 */
AF_6,  /*!< GPIO AF 6 */
AF_7	 /*!< GPIO AF 7 */
} gpio_af_e;

//*****************************************************************************
//
// Configuration_Pull-Up_Pull-Down_enumeration
//
//*****************************************************************************

typedef enum {
PULLDOWN,  /*!< Pull-down  */
PULLUP,    /*!< Pull-up    */
PULLNONE   /*!< Pull-none  */
} gpio_pull_e;


//*****************************************************************************
//
// Output_type_enumeration
//
//*****************************************************************************

typedef enum {
OPENDRAIN,
PUSHPULL
} gpio_outtype_e;
	
//***************************************************************************
//
// gpio debounce structure definition
//
//***************************************************************************

typedef struct {
uint8_t period;      /*!< specify the period for the debounce */
uint8_t countDown;
uint8_t last;		
uint8_t state;
} gpio_dbounce_struct_t;

//***************************************************************************
//
// gpio delay structure definition
//
//***************************************************************************

typedef struct {
uint16_t after;
uint8_t state;
} gpio_delay_struct_t;

//***************************************************************************
//
// gpio PWM structure definition
//
//***************************************************************************
typedef struct {
	uint16_t ONticks;
  uint16_t OFFticks;
	int8_t repeats; // -1:infinite, 0:no pwm
} gpio_pwm_struct_t;

//***************************************************************************
//
// gpio init structure definition
//
//***************************************************************************

typedef struct {
uint8_t                port;     /*!< Specifies the GPIO port to be configured.                         */                 
uint8_t                pin;      /*!< Specifies the GPIO pins to be configured.                         */
uint8_t                dir;      /*!< Specifies the operating direction type for the selected pin       */
gpio_outtype_e         outType;  /*!< Specifies the operating output type for the selected pins         */
gpio_pull_e            pullType; /*!< Specifies the operating Pull-up/Pull down for the selected pin    */   
uint8_t                af;       /*!< Alternate Function                                                */
gpio_dbounce_struct_t* dbounce;  /*!< Specifies the input debounce period for the selected Pin          */
uint8_t                polarity; /*!< Specifies the polarity for the Selected Pins 0 - Non-Inv 1 - Inv  */
gpio_delay_struct_t*   delay;    /*!< Specifies the Output delay for the Selected Pin                   */
gpio_pwm_struct_t      pwm_now;  /*!< copy of currently selected pattern                                */
gpio_pwm_struct_t      pwmPattern[MAX_PWM_ENTRIES];  /*!< Specifies the PWM sequence for Selected Pin                 */
int8_t                 sequence_repeat;           /*!< Specifies number of rounds of PWM sequence for Selected Pin */
uint8_t                pwm_index;
uint8_t                event;
uint32_t                gpioBase;
} gpio_struct_t;


extern uint32_t gpio_d[5];

//****************************************************************************
//
//Prototypes for the APIs.
//
//****************************************************************************

void gpio_ctor(gpio_struct_t* handle);
void gpio_tick(gpio_struct_t* handle);
void gpio_set(gpio_struct_t* handle, uint16_t delay);
void gpio_clr(gpio_struct_t* handle, uint16_t delay);
void gpio_toggle(gpio_struct_t* handle, uint16_t delay);
uint8_t gpio_read(gpio_struct_t* handle);
void gpio_pwm_add(gpio_struct_t* handle,uint8_t index, uint16_t ONticks, uint16_t OFFticks, int8_t repeats);
void gpio_pwm_clear(gpio_struct_t* handle);

#endif
/************************ (C) COPYRIGHT Anayatech Systems Pvt Ltd *****END OF FILE****/
