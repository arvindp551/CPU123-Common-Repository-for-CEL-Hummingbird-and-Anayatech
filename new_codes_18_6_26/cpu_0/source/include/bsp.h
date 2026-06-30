/***************************************************************************//**
 * @file     bsp.h
 * @brief    All application specific source files
 * @version  1.0.0
 * @details
 * Compiler : Keil uVision
 * Target : Nuvoton M263
 * Module : Central Controller Card
 * Date Created : Dec 24, 2025
 * @copyright Copyright (C) 2026 Anaya Tech Systems Pvt. Ltd. . All rights reserved.
 * @author    SG 
 ******************************************************************************/
 
#ifndef __BSP_H__
#define __BSP_H__

#include "common_header.h"
#include "drvr_spi.h"
#include "drvr_uart.h"
#include "drvr_uuart.h"
#include "drvr_iic.h"

#define sysTimer_IRQHandler TMR3_IRQHandler
#define LED_HB PD1
#define LED_DBG0 PA8
#define LED_DBG1 PF6
#define LED_DBG2 PF5
#define LED_DBG3 PF4

#define BUFSZ_RX_CLI  100
#define BUFSZ_RX_EL   100
#define BUFSZ_RX_COM0 100
#define BUFSZ_RX_COM1 100
#define BUFSZ_RX_GPS  100

#define BUFSZ_TX_CLI  100
#define BUFSZ_TX_EL   100
#define BUFSZ_TX_COM0 100
#define BUFSZ_TX_COM1 100
#define BUFSZ_TX_GPS  100

#define BUFSZ_RX_CPUx 100
#define BUFSZ_TX_CPUx 100
#define BUFSZ_RX_FPGA 100
#define BUFSZ_TX_FPGA 100

/***********  PIN MAPPING ******************
CLIRX	PB12 UART
CLITX	PB13 UART
ELTX	PC3 UART
ELRX	PC2 UART
SMTX	PA5 UART
SMRX	PA4 UART
COM1TX	PB1 UART
COM1RX	PB0 UART
COM2TX	PB3 UART
COM2RX	PB2 UART
DISPLAYTX	PA3 UART
DISPLAYRX	PA2 UART
GPSTX	PD1	UART
GPSRX	PD2	UART

SPI2_MOSI	PA15 SPI
SPI2_MISO	PA14 SPI
SPI2_CLK	PA13 SPI
SPI2_SS1	PA12 SPI
SPI2_SS2	PC4 SPI
SPI3_SS3	PC5 SPI
SPI1_SS	PA6 SPI
SPI1_CLK	PA7 SPI
SPI1_MOSI	PC6 SPI
SPI1_MISO	PC7 SPI

CANTX	PB11	CAN
CANRX	PB10	CAN

CPU0_FORCED_RST	PB9 GPIO
FPGA_FORCED_RST	PA0 GPIO
Heartbeat LED	PD1 GPIO

NRST	nRESET  "DEDICATED RESET"
********************************/

#define SYSTEM_HB PD1
#define CPU0_FORCED_RST PB9
#define FPGA_FORCED_RST PA0

#define TICK_PERIOD 10U //ms
#define TICK_FREQ 1000U/TICK_PERIOD //Hz, period in ms

#define ELAPSED_ms(x) if((ticks*TICK_PERIOD)%x == 0) 

#define I2C_SLAVEADDRESS 0x3C // try 0x3D if 0x3C doesn't work
extern i2c_struct_t i2c0;
extern spi_struct_t spi_cpux;
extern spi_struct_t spi_fpga;
extern uart_struct_t uart_cli;
extern uart_struct_t uart_el;
extern uart_struct_t uart_sm;
extern uart_struct_t uart_com[2];
extern uuart_struct_t uart_display;
extern uart_struct_t uart_gps;

void sysTimer_Init(void);
void bsp_ctor(void);
void wait_ms(uint32_t w);

#endif
