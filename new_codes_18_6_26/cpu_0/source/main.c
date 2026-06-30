/***************************************************************************//**
 * @file     main.c
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
 * int32_t main(void);
 ******************************************************************************/

#include "NuMicro.h"
#include "targetdev.h"
#include "app.h"
#include "uart.h"
#include "bsp.h"

void cliUART(void);
void cpu_sm(void);
uint32_t g_cnt = 0;

/*
 * @brief entry point of the program
 */

/**
 * @brief Entry point of the application.
 *
 * Initializes system-level resources, including unlocking protected registers,
 * board support package (BSP) initialization, and peripheral setup. After
 * initialization, the function enters an infinite loop executing the main
 * application tasks such as CLI handling and CPU state machine processing.
 *
 * @param [in] None
 * @param [out] None
 *
 * @return int32_t Returns 0 (this point should never be reached under normal operation).
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note This function runs indefinitely. All periodic and application-specific
 *       processing is handled inside the main loop (e.g., cliUART(), cpu_sm()).
 *       Any return from this function indicates an unexpected condition.
 */
 
int32_t main(void) {
	//uint8_t ch
	// inits
	SYS_UnlockReg();
	bsp_ctor();

	printf("SSPBAC CPU0 BOOTING.....\r\n");

	while(1) {
		g_cnt++;
		cliUART();
		
		#if 0
		if(UART_GET_RX_EMPTY(UART1) == 0) {
			UART_Read(UART1, &ch, 1);
			UART_Write(UART1, &ch, 1);
		}
		#endif
		cpu_sm();
	}

	return 0; // should neverb reach here
}
