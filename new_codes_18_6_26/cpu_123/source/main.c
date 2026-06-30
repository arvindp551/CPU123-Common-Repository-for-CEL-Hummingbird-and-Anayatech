/***************************************************************************//**
 * @file     main.c
 * @brief    All application specific source files
 * @version  1.0.0
 * @details
 * Compiler : Keil uVision
 * Target : Nuvoton M263
 * Module : Central Controller Card
 * Date Created : Dec 16, 2025
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

/**

* @brief
* Entry point of the application.
*
* This function performs system initialization and executes the main application loop. It initializes hardware and board support components, then continuously runs the CPU state machine.
*
* @param
* None.
*
* @return
* int32_t
*
* @details
* Requirement ID(s) - ?
*
* @note
* System initialization:
* Unlocks protected system registers.
* Calls bsp_ctor() to initialize board-specific peripherals and configurations.
*
* Main loop:
* Executes cpu_sm() continuously for system operation.
*
* TEST_MODE (if enabled):
* input_relay_bfm() is executed for simulation/testing of relay inputs.
*
* The function is designed to run indefinitely.
*
* Returning from main() is not expected in embedded systems; 
* return statement is provided for compliance.
*/
  
//entry point of the sowtware
int32_t main(void) {

	// inits
	SYS_UnlockReg();
	bsp_ctor();

	//printf("SSPBAC Booting ...\r\n");

	while(true) {
		cpu_sm();
		
		#ifdef TEST_MODE
		input_relay_bfm();
		#endif
	}

	return 0; // should never reach here
}
