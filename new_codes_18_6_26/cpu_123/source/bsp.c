/***************************************************************************//**
 * @file     bsp.c
 * @brief    All application specific source files
 * @version  1.0.0
 * @details
 * Compiler : Keil uVision
 * Target : Nuvoton M263
 * Module : Central Controller Card
 * Date Created : Jan 16, 2026
 * @copyright Copyright (C) 2026 Anaya Tech Systems Pvt. Ltd. . All rights reserved.
 * @author    SG
 * @par Functions Included
 * void wait_ms(uint32_t w); <br>
 * void bsp_ctor(void); <br>
 * static void sysTimer_Init(void); <br>
 * static void sysClock_Init(void); <br>
 * void sysTimer_IRQHandler(void);
 ******************************************************************************/

#include <stdbool.h>
#include "common_header.h"
#include "bsp.h"
#include "app.h"
#include "isp_user.h"

static uint8_t buf_rx_cpux[BUFSZ_RX_CPUx];
static uint8_t buf_tx_cpux[BUFSZ_TX_CPUx];

static fifo_t fifo_rx_cpux;
static fifo_t fifo_tx_cpux;
spi_struct_t spi_cpux = {.todo = SPI_NONE, .complete = 1, .state = SPI_IDLE};

static void sysClock_Init(void);
static void sysTimer_Init(void);


volatile static uint32_t wait_time = 0;

/**
 * @brief  Blocking delay function in milliseconds.
 *         Converts the input delay time into internal tick units
 *         and waits until the delay expires.
 *
 * @param  [in]  w   Delay duration in milliseconds.
 *
 * @return None
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note
 * - Delay is implemented using a global variable `wait_time`.
 * - Conversion uses rounding: (w + 9) / 10, assuming 10 ms tick resolution.
 */

void wait_ms(uint32_t w) {
	wait_time = (w+9U)/10U; //round
	while(wait_time!=0U) {}
}

/**
 * @brief  initilize all the rx and tx buffer for CPU.
 *         Initilize mode for GPIO
 *
 * @param  None
 *
 * @return None
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note
 * - Initializes system clock and system timer.
 * - Sets up SPI communication with CPUx including TX/RX FIFOs and callback.
 * - Configures GPIO pins for heartbeat, relay interface, interlock signals, and test mode.
 * - Enables ISP (In-System Programming) and configures flash memory access.
 * - Configures SysTick timer for periodic timing (300 ms timeout).
 * - TEST_MODE macro changes DATA pin direction (input/output).
 */
 
void bsp_ctor(void) {
	uint32_t g_apromSize, g_dataFlashAddr, g_dataFlashSize;

	sysClock_Init();
	fifo_init(&fifo_tx_cpux,buf_tx_cpux,BUFSZ_TX_CPUx);
	fifo_init(&fifo_rx_cpux,buf_rx_cpux,BUFSZ_RX_CPUx);

	spi_cpux.ptx = &fifo_tx_cpux;
	spi_cpux.prx = &fifo_rx_cpux;
	spi_cpux.inten = 0;
	spi_cpux.cb = spi_read_cb;
	spi_cpux.is_master = 0;
	spi_cpux.port = 1;
  	spi_ctor(&spi_cpux);

	GPIO_SetMode(PF,BIT5,GPIO_MODE_OUTPUT); // PF5 CPU Heartbeat 
	GPIO_SetMode(PA,BIT2,GPIO_MODE_OUTPUT); // PA2 CLK
	
	#ifdef TEST_MODE
	GPIO_SetMode(PA,BIT3,GPIO_MODE_OUTPUT);  // PA3 DATA [changed as output earlier was input]
	#else
	GPIO_SetMode(PA,BIT3,GPIO_MODE_INPUT);  // PA3 DATA [changed as output earlier was input]
	#endif
	GPIO_SetMode(PA,BIT4,GPIO_MODE_OUTPUT); // PA4  RST|SYNC
	GPIO_SetMode(PA,BIT5,GPIO_MODE_OUTPUT); // PA5  HB LED
	GPIO_SetMode(PA,BIT12,GPIO_MODE_INPUT); // PA12  Input card present TODO: dont knwo the pin. Added for completeness
	
	GPIO_SetMode(PD,BIT0,GPIO_MODE_INPUT); // PD0  INTERLOCK CLK 
	GPIO_SetMode(PD,BIT1,GPIO_MODE_INPUT); // PD1  INTERLOCK RST|SYNC 
	GPIO_SetMode(PD,BIT2,GPIO_MODE_OUTPUT); // PD2  INTERLOCK DATA 
	
	sysTimer_Init();

	/* Enable ISP */
	CLK->AHBCLK |= CLK_AHBCLK_ISPCKEN_Msk;
	FMC->ISPCTL |= (FMC_ISPCTL_ISPEN_Msk | FMC_ISPCTL_APUEN_Msk);

	/* Get APROM Flash size */
	g_apromSize = BL_EnableFMC();
	g_dataFlashAddr = SCU->FNSADDR;

	if(g_dataFlashAddr < g_apromSize)
	{
		g_dataFlashSize = (g_apromSize - g_dataFlashAddr);
	}
	else
	{
		g_dataFlashSize = 0;
	}

	/* Set Systick time-out for 300ms */
	SysTick->LOAD = 300000 * CyclesPerUs;
	SysTick->VAL  = (0x00);
	SysTick->CTRL = SysTick->CTRL | SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;   /* Use CPU clock */
}


/**
 * @brief  Initializes system timer (Timer3) for periodic interrupts.
 *         Configures Timer3 to generate interrupts at 1 ms interval (1 kHz).
 *
 * @param  None
 *
 * @return None
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note
 * - Enables Timer3 peripheral clock.
 * - Enables Timer3 interrupt and starts the timer.
 * - NVIC interrupt for Timer3 is enabled.
 */
 
static void sysTimer_Init(void)
{
	/* Enable IP clock */
	CLK->APBCLK0 |= CLK_APBCLK0_TMR3CKEN_Msk;
	/* Select IP clock source */
	CLK->CLKSEL1 = (CLK->CLKSEL1 & (~CLK_CLKSEL1_TMR3SEL_Msk)) | CLK_CLKSEL1_TMR3SEL_HIRC;
	// Set timer frequency to 1KHZ
	TIMER_Open(TIMER3, TIMER_PERIODIC_MODE,1000); //1000Hz or 1ms
	// Enable timer interrupt
	TIMER_EnableInt(TIMER3);
	// Start Timer 3
	TIMER_Start(TIMER3);
	NVIC_EnableIRQ(TMR3_IRQn);

 /* Lock protected registers */
  //  SYS_LockReg();
}

/**
 * @brief  Initializes the system clock using internal high-speed RC oscillator (HIRC).
 *         Configures HIRC as the main clock source and updates system core clock settings.
 *
 * @param  None
 *
 * @return None
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note
 * - Enables HIRC (internal RC oscillator) and waits until it stabilizes.
 * - Updates SystemCoreClock variable to reflect current clock configuration.
 * - External clock sources are not used in this configuration.
 */

static void sysClock_Init(void){
  	/* Enable HIRC clock */
  	CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);
  	/* Waiting for HIRC clock ready */
  	CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);
  	/* Select HCLK clock source as HIRC and and HCLK clock divider as 1 */
  	CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC, CLK_CLKDIV0_HCLK(1));
	/* Update System Core Clock */
	SystemCoreClockUpdate();
}

static uint32_t ticks = 0;
bool onesec_pulse = false;
uint16_t g_TGT_ticks = 0;
uint16_t g_TCF_ticks = 0;
uint16_t g_120s_ticks = 0;
uint16_t g_ACKN_D_ticks = 0;
uint16_t g_ACKN_R_ticks = 0;
uint16_t g_SSBPAC_FAIL_ticks = 0;

#define decrTicks(v) if(v != 0U) v -= 1U

/**
 * @brief  IRQ handler for system.
 *		   Decrease the time by 1 MS.
 *
 * @param  None
 *
 * @return None
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note
 * - Interrupts are disabled at entry and re-enabled before exit to ensure atomic execution.
 * - Clears Timer3 interrupt flag at the beginning of ISR.
 * - Executes SPI state machine for communication handling.
 * - Decrements delay counters (wait_time, various global tick counters).
 * - Calls vi_process() for relay input sampling and debouncing.
 * - Calls cpu_fpga_xfer() for serial data exchange between CPU and FPGA.
 * - Increments global system tick counter.
 */

void sysTimer_IRQHandler(void) {
	__disable_irq();

	TIMER_ClearIntFlag(TIMER3);

	spi_agent_sm(&spi_cpux);

	if(wait_time!=0U) {wait_time--;}

	ELAPSED_ms(1U) {
		decrTicks(g_TGT_ticks);
		decrTicks(g_TCF_ticks);
		decrTicks(g_ACKN_D_ticks);
		decrTicks(g_ACKN_R_ticks);
		decrTicks(g_SSBPAC_FAIL_ticks);
	}

	ELAPSED_ms(500U) {
		onesec_pulse = true;
	}
 
	ELAPSED_sec(1U) {
		if(g_120s_ticks != 0U) {g_120s_ticks--;}
	}
	
	ELAPSED_ms(200U) {
		LED_HB = 1; //led OFF
	}
	ELAPSED_ms(2000U) {
		LED_HB = 0; //led ON
	}

	vi_process(); // read and debounce relay inputs
  	cpu_fpga_xfer(); //serial transfer of vi,nvo and oe vi between FPGA and CPU. 
	ticks++;
	__enable_irq();
}
