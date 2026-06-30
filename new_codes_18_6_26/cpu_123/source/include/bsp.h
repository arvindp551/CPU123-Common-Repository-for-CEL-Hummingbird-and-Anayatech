/***************************************************************************//**
 * @file     bsp.h
 * @brief    All application specific source files
 * @version  1.0.0
 * @details
 * Compiler : Keil uVision
 * Target : Nuvoton M263
 * Module : Central Controller Card
 * Date Created : Dec 30, 2025
 * @copyright Copyright (C) 2026 Anaya Tech Systems Pvt. Ltd. . All rights reserved.
 * @author    SG 
 ******************************************************************************/
#ifndef BSP_H__
#define BSP_H__

#include "common_header.h"
#include "drvr_spi.h"
#include "drvr_uart.h"

#define sysTimer_IRQHandler TMR3_IRQHandler

#define BUFSZ_RX_CPUx 		100
#define BUFSZ_TX_CPUx 		100

#define CPU_HB 				PF5
#define RELAY_CLK  			PA2
#define RELAY_DATA 			PA3
#define RELAY_RST  			PA4
#define INPUT_CARD_PRESENT PA12
#define LED_HB 				PA5

#define FPGA2CPU_CLK  		PD0
#define FPGA2CPU_RST  		PD1
#define CPU2FPGA_DATA 		PD2

#define FPGA2CPU_DATA 		PD3
#define CPU_FORCED_RST 		PB13


#define ELAPSED_ms(x) if((ticks%(x)) == 0U) 
#define ELAPSED_sec(x) if((ticks%((x)*1000U)) == 0U) 

extern spi_struct_t spi_cpux;

void bsp_ctor(void);
void wait_ms(uint32_t w);

#endif
