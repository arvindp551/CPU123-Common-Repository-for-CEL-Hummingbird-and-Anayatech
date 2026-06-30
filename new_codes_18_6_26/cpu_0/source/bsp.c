/***************************************************************************//**
 * @file     bsp.c
 * @brief    All application specific source files
 * @version  1.0.0
 * @details
 * Compiler : Keil uVision
 * Target : Nuvoton M263
 * Module : Central Controller Card
 * Date Created : Jan 09, 2026
 * @copyright Copyright (C) 2026 Anaya Tech Systems Pvt. Ltd. . All rights reserved.
 * @author    SG
 * @par Functions Included
 * void wait_ms(uint32_t w); <br>
 * void bsp_ctor(void); <br>
 * void sysTimer_Init(void); <br>
 * void sysClock_Init(void); <br>
 * void sysTimer_IRQHandler(void); <br>
 * void I2C0_IRQHandler(void);
 ******************************************************************************/

#include "common_header.h"
#include "bsp.h"
#include "app.h"
#include "isp_user.h"
#include "SH1106.h"

static uint8_t buf_rx_cli[BUFSZ_RX_CLI];
static uint8_t buf_rx_el[BUFSZ_RX_EL];
static uint8_t buf_rx_com0[BUFSZ_RX_COM0];
static uint8_t buf_rx_com1[BUFSZ_RX_COM1];
static uint8_t buf_rx_gps[BUFSZ_RX_GPS];

static uint8_t buf_tx_cli[BUFSZ_TX_CLI];
static uint8_t buf_tx_el[BUFSZ_TX_EL];
static uint8_t buf_tx_com0[BUFSZ_TX_COM0];
static uint8_t buf_tx_com1[BUFSZ_TX_COM1];
static uint8_t buf_tx_gps[BUFSZ_TX_GPS];

static fifo_t fifo_rx_cli;
static fifo_t fifo_rx_el;
static fifo_t fifo_rx_com0;
static fifo_t fifo_rx_com1;
static fifo_t fifo_rx_gps;

static fifo_t fifo_tx_cli;
static fifo_t fifo_tx_el;
static fifo_t fifo_tx_com0;
static fifo_t fifo_tx_com1;
static fifo_t fifo_tx_gps;

uart_struct_t uart_cli;
uart_struct_t uart_el;
uart_struct_t uart_com[2];
uart_struct_t uart_gps;
i2c_struct_t i2c0;

static uint8_t buf_rx_cpux[BUFSZ_RX_CPUx];
static uint8_t buf_tx_cpux[BUFSZ_TX_CPUx];
static uint8_t buf_rx_fpga[BUFSZ_RX_FPGA];
static uint8_t buf_tx_fpga[BUFSZ_TX_FPGA];

fifo_t fifo_rx_cpux;
fifo_t fifo_tx_cpux;
fifo_t fifo_rx_fpga;
fifo_t fifo_tx_fpga;
spi_struct_t spi_cpux;
spi_struct_t spi_fpga;

void sysClock_Init(void);

/**
 * @brief  Provides a blocking delay for the specified duration in milliseconds.
 *
 * @param  [in]  w   Delay duration in milliseconds.
 *
 * @return None
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note   This function converts the input time (in ms) into system tick counts
 *         based on TICK_PERIOD and performs rounding to ensure minimum delay.
 *         It blocks execution until the global variable 'wait_time' becomes zero.
 *         The variable 'wait_time' is expected to be decremented periodically
 *         (e.g., in a timer ISR).
 */

volatile uint32_t wait_time = 0;
void wait_ms(uint32_t w) {
	wait_time = (w+TICK_PERIOD-1U)/TICK_PERIOD; //round
	while(wait_time);
}

/**
 * @brief  Board Support Package (BSP) constructor.
 *         Initializes system clock, communication peripherals (UART, SPI, I2C),
 *         FIFO buffers, GPIO configurations, system timer, and flash/ISP settings.
 *         This function prepares all low-level hardware resources required for
 *         application execution.
 *
 * @param  None
 *
 * @return None
 *
 * @details
 * Requirement ID(s) -
 * PI_SSBPAC_SwRS_INTR_001 
 * PI_SSBPAC_SwRS_GENR_006
 * PI_SSBPAC_SwRS_GENR_008
 * PI_SSBPAC_SwRS_GENR_014
 * PI_SSBPAC_SwRS_GENR_015
 * PI_SSBPAC_SwRS_GENR_016
 * PI_SSBPAC_SwRS_GENR_018
 * PI_SSBPAC_SwRS_GENR_024
 * PI_SSBPAC_SwRS_GENR_028
 * PI_SSBPAC_SwRS_GENR_034
 * PI_SSBPAC_SwRS_GENR_035
 * PI_SSBPAC_SwRS_GENR_036
 * PI_SSBPAC_SwRS_GENR_037
 * PI_SSBPAC_SwRS_GENR_038
 * PI_SSBPAC_SwRS_GENR_039
 * PI_SSBPAC_SwRS_GENR_040
 * PI_SSBPAC_SwRS_GENR_047
 * PI_SSBPAC_SwRS_GENR_048
 *
 * @note   - Initializes multiple UART interfaces:
 *         - Configures SPI interfaces for CPUx and FPGA communication in master mode.
 *         - Initializes I2C interface for OLED display.
 *         - Sets up GPIO pins for system heartbeat, debug signals, and reset controls.
 *         - Initializes system timer via sysTimer_Init().
 *         - Enables ISP (In-System Programming) and retrieves APROM/Data Flash size.
 *         - Configures SysTick timer for ~300 ms timeout using CPU clock.
 *         - Must be called during system startup before using any peripherals.
 */

void bsp_ctor(void) {
	sysClock_Init();
	
	fifo_init(&fifo_rx_cli,buf_rx_cli,BUFSZ_RX_CLI);
	fifo_init(&fifo_rx_el,buf_rx_el,BUFSZ_RX_EL);
	fifo_init(&fifo_rx_com0,buf_rx_com0,BUFSZ_RX_COM0);
	fifo_init(&fifo_rx_com1,buf_rx_com1,BUFSZ_RX_COM1);
	fifo_init(&fifo_rx_gps,buf_rx_gps,BUFSZ_RX_GPS);

	fifo_init(&fifo_tx_cli,buf_tx_cli,BUFSZ_TX_CLI);
	fifo_init(&fifo_tx_el,buf_tx_el,BUFSZ_TX_EL);
	fifo_init(&fifo_tx_com0,buf_tx_com0,BUFSZ_TX_COM0);
	fifo_init(&fifo_tx_com1,buf_tx_com1,BUFSZ_TX_COM1);
	fifo_init(&fifo_tx_gps,buf_tx_gps,BUFSZ_TX_GPS);

	uart_cli.baudRate = 115200;	
	uart_cli.prx = &fifo_rx_cli;	
	uart_cli.ptx = NULL;	
	uart_cli.inten = 0;	
	uart_cli.port = 0;	
	uart_ctor(&uart_cli);

	uart_el.baudRate = 115200;//9600;	
	uart_el.prx = &fifo_rx_el;	
	uart_el.ptx = &fifo_tx_el;	
	uart_el.inten = 0;	
	uart_el.port = 3;	
	uart_ctor(&uart_el);

	uart_com[0].baudRate = 1200;	
	uart_com[0].prx = &fifo_rx_com0;	
	uart_com[0].ptx = &fifo_tx_com0;	
	uart_com[0].inten = 0;	
	uart_com[0].port = 2;	
	uart_ctor(&uart_com[0]);

	uart_com[1].baudRate = 1200;	
	uart_com[1].prx = &fifo_rx_com1;	
	uart_com[1].ptx = &fifo_tx_com1;	
	uart_com[1].inten = 0;	
	uart_com[1].port = 1;	
	uart_ctor(&uart_com[1]);

	uart_gps.baudRate = 9600;	
	uart_gps.prx = &fifo_rx_gps;	
	uart_gps.ptx = &fifo_tx_gps;	
	uart_gps.inten = 0;	
	uart_gps.port = 4;	
	uart_ctor(&uart_gps);

	fifo_init(&fifo_tx_cpux,buf_tx_cpux,BUFSZ_TX_CPUx);
	fifo_init(&fifo_rx_cpux,buf_rx_cpux,BUFSZ_RX_CPUx);
	fifo_init(&fifo_tx_fpga,buf_tx_fpga,BUFSZ_TX_FPGA);
	fifo_init(&fifo_rx_fpga,buf_rx_fpga,BUFSZ_RX_FPGA);

	spi_cpux.ptx = &fifo_tx_cpux;
	spi_cpux.prx = &fifo_rx_cpux;
	spi_cpux.inten = 0;
	spi_cpux.cb = NULL;
	spi_cpux.is_master = 1;
	spi_cpux.port = 2;
  spi_ctor(&spi_cpux);

	spi_fpga.ptx = &fifo_tx_fpga;
	spi_fpga.prx = &fifo_rx_fpga;
	spi_fpga.inten = 0;
	spi_fpga.cb = NULL;
	spi_fpga.is_master = 1;
	spi_fpga.port = 1;
	spi_ctor(&spi_fpga);

	i2c0.slave_addr = SH1106_I2C_ADDR;
	i2c0.inst = I2C0;
	i2c_ctor(&i2c0);

	//#define SYSTEM_HB PD1 Blinks @2Hz 
	GPIO_SetMode(PD, BIT1, GPIO_MODE_OUTPUT);  
	//#define CPU0_FORCED_RST PB9
	GPIO_SetMode(PB, BIT9, GPIO_MODE_INPUT);  
	//#define FPGA_FORCED_RST PA0
	GPIO_SetMode(PA, BIT0, GPIO_MODE_OUTPUT);  
	
	//#define DBG0
	GPIO_SetMode(PA, BIT8, GPIO_MODE_OUTPUT);  
	//#define DBG1
	GPIO_SetMode(PF, BIT6, GPIO_MODE_OUTPUT);  
	//#define DBG2
	GPIO_SetMode(PF, BIT5, GPIO_MODE_OUTPUT);  
	//#define DBG3
	GPIO_SetMode(PF, BIT4, GPIO_MODE_OUTPUT);  


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
 * @brief  Initializes system timer (TIMER3) for periodic interrupts.
 *         Configures TIMER3 to operate in periodic mode using HIRC clock
 *         source and generates interrupts at a frequency defined by TICK_FREQ.
 *
 * @param  None
 *
 * @return None
 *
 * @details
 * Requirement ID(s) -
 * PI_SSBPAC_SwRS_INTR_002
 * PI_SSBPAC_SwRS_INTR_008
 * PI_SSBPAC_SwRS_GENR_019
 *
 * @note   - TIMER3 clock is enabled via APBCLK0.
 *         - Clock source is set to HIRC (internal RC oscillator).
 *         - Timer operates in periodic mode with frequency = TICK_FREQ
 *           (typically 1 kHz for 1 ms system tick).
 *         - Timer interrupt is enabled and corresponding NVIC IRQ is configured.
 *         - Used as the system time base for delays, scheduling, and timekeeping.
 *         - Alternative configuration (commented) allows lower frequency (e.g., 100 Hz).
 *         - Protected registers lock is currently disabled (commented out).
 */
 
void sysTimer_Init(void)
{
	/* Enable IP clock */
	CLK->APBCLK0 |= CLK_APBCLK0_TMR3CKEN_Msk;
	/* Select IP clock source */
	CLK->CLKSEL1 = (CLK->CLKSEL1 & (~CLK_CLKSEL1_TMR3SEL_Msk)) | CLK_CLKSEL1_TMR3SEL_HIRC;
	// Set timer frequency to 1KHZ
	TIMER_Open(TIMER3, TIMER_PERIODIC_MODE,TICK_FREQ);
	
	//TIMER_Open(TIMER3, TIMER_PERIODIC_MODE,100); //100Hz or 10ms
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
 *         Enables HIRC, waits for it to stabilize, and configures it as the main
 *         HCLK source with no division.
 *
 * @param  None
 *
 * @return None
 * 
 * @details
 * Requirement ID(s) -
 * PI_SSBPAC_SwRS_GENR_013
 *
 * @note   - Uses internal HIRC (High-Speed Internal RC oscillator) as clock source.
 *         - Waits until HIRC clock is stable before switching.
 *         - Sets HCLK divider to 1 (no division).
 *         - Updates SystemCoreClock variable to reflect the configured frequency.
 *         - Should be called early during system initialization before peripheral setup.
 */
 
void sysClock_Init(void){
  	/* Enable HIRC clock */
  	CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);
  	/* Waiting for HIRC clock ready */
  	CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);
  	/* Select HCLK clock source as HIRC and and HCLK clock divider as 1 */
  	CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC, CLK_CLKDIV0_HCLK(1));
	/* Update System Core Clock */
	SystemCoreClockUpdate();
}

uint32_t ticks = 0;
uint32_t elapsed = 0;

#define decrTicks(v) if(v != 0U) v -= 1U

/**
 * @brief  System Timer Interrupt Service Routine (ISR).
 *         Handles periodic system tasks including UART communication,
 *         SPI state machines, software timers, heartbeat LED control,
 *         and system time updates.
 *
 * @param  None
 *
 * @return None
 *
 * @details
 * Requirement ID(s) -
 * PI_SSBPAC_SwRS_INTR_003
 * PI_SSBPAC_SwRS_INTR_004
 * PI_SSBPAC_SwRS_INTR_005
 * PI_SSBPAC_SwRS_INTR_006
 * PI_SSBPAC_SwRS_INTR_009
 *
 * @note   - Executed on TIMER3 interrupt at a fixed interval (e.g., 1 ms tick).
 *         - Disables global interrupts at entry and re-enables before exit.
 *         - Clears TIMER3 interrupt flag at the beginning of ISR.
 *
 *         UART Handling:
 *         - Reads incoming data from CLI, COM0, COM1, and GPS UART interfaces.
 *         - Handles UART transmission for EL, COM0, and COM1 interfaces.
 *
 *         Timing & Scheduling:
 *         - Updates wait_time and multiple software timers using decrTicks().
 *         - Maintains global system timestamp (sys.global_ts) at 1-second intervals.
 *         - Updates global tick counters (ticks, elapsed).
 *
 *         LED Indication:
 *         - Controls heartbeat LED (LED_HB) with ON/OFF timing pattern.
 *
 *         SPI Handling:
 *         - Executes SPI master state machines for CPUx and FPGA at 1 ms intervals.
 *
 *         I2C Timeout Handling:
 *         - Manages I2C busy timeout and recovery mechanism.
 *
 *         - Uses ELAPSED_ms macros for time-sliced execution of periodic tasks.
 *         - ISR should be kept efficient to avoid timing issues.
 */
 
uint32_t g_i2c_timeout = 0;

void sysTimer_IRQHandler(void){
	__disable_irq();

	TIMER_ClearIntFlag(TIMER3);

	uart_read_isr(&uart_cli);
	//uart_read(&uart_el); event logger is write only interface
	uart_read_isr(&uart_com[0]);
	uart_read_isr(&uart_com[1]);
	uart_read_isr(&uart_gps);

	//uart_write(&uart_cli); CLI write done through printf as it is stddev
	uart_write_isr(&uart_el);
	uart_write_isr(&uart_com[0]);
	uart_write_isr(&uart_com[1]);

	decrTicks(wait_time);
	ELAPSED_ms(20U) {
		LED_HB = 1; //led OFF
	}
	ELAPSED_ms(2000U) {
		LED_HB = 0; //led ON
	}
	ELAPSED_ms(1000U) {
		if(sys.global_ts!=0U){
			sys.global_ts++;
		}
	}

	ELAPSED_ms(1U) {
	spi_master_sm(&spi_cpux);
	spi_master_sm(&spi_fpga);
	}

	//software timers	
	decrTicks(g_i2c_timeout);
	decrTicks(tmr.com_send_pkt);
	decrTicks(tmr.primary_com_ok);
	decrTicks(tmr.secondary_com_ok);
	decrTicks(tmr.check_flash);
	decrTicks(tmr.update_oled);
	decrTicks(tmr.check_faults);
	decrTicks(tmr.btn_reset);
	decrTicks(tmr.wait_reset);
	decrTicks(tmr.wait_reset_ack);
	decrTicks(tmr.wait_C_pkt);

	ELAPSED_ms(100U) {	
		decrTicks(fdbk_fault.ascr);
		decrTicks(fdbk_fault.tcfr_d);
		decrTicks(fdbk_fault.tcfr_p);
		decrTicks(fdbk_fault.tgtr_d);
		decrTicks(fdbk_fault.tgtr_p);
	}

	if(i2c0.is_busy && g_i2c_timeout == 0) {
		g_i2c_timeout = 100;
	}
	else if(i2c0.is_busy && g_i2c_timeout == 10) {
		i2c0.is_busy = 0;
		g_i2c_timeout = 0;
	}
	else if (i2c0.is_busy == 0) {
		g_i2c_timeout = 0;
	}
	
	
	ticks++;
  	elapsed++;
	__enable_irq();
}

/**
 * @brief  I2C0 Interrupt Service Routine (ISR).
 *         Handles I2C0 interrupt events including timeout detection and
 *         delegation to the registered I2C handler function.
 *
 * @param  None
 *
 * @return None
 *
 * @details
 * Requirement ID(s) -
 * PI_SSBPAC_SwRS_INTR_007
 *
 * @note   - Reads current I2C status using I2C_GET_STATUS().
 *         - Checks for timeout condition using I2C_GET_TIMEOUT_FLAG().
 *         - If timeout occurs:
 *             - Clears the timeout flag to recover the I2C bus.
 *         - If no timeout:
 *             - Calls the registered callback handler (s_I2C0HandlerFn)
 *               with I2C instance and status, if handler is not NULL.
 *         - Enables flexible handling of I2C events via function pointer callback.
 *         - ISR should execute quickly to maintain I2C communication reliability.
 */
 
volatile I2C_FUNC s_I2C0HandlerFn = NULL;

void I2C0_IRQHandler(void) {
    uint32_t u32Status;

    u32Status = I2C_GET_STATUS(I2C0);

    if(I2C_GET_TIMEOUT_FLAG(I2C0)) {
        /* Clear I2C0 Timeout Flag */
        I2C_ClearTimeoutFlag(I2C0);
        //g_u8TimeoutFlag = 1;
    }
    else {
        if(s_I2C0HandlerFn != NULL)
				{
            s_I2C0HandlerFn(&i2c0, u32Status);
				}
    }
}
