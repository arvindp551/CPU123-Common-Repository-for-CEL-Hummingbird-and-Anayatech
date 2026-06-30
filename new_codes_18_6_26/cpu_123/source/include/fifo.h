/***************************************************************************//**
 * @file     fifo.h
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
#ifndef UART_FIFO_H__
#define UART_FIFO_H__

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

//fifo database structure
typedef struct {
	uint16_t rdptr;
	uint16_t wrptr;
	uint8_t  *buf;
	bool is_empty;
	bool is_full;
	uint8_t err_overflow;
	uint8_t err_underrun;
	uint16_t fill;
	uint16_t bufsize;
} fifo_t;

void fifo_init(fifo_t* pf, uint8_t* pbuf, uint16_t bufsize);
void fifo_reset(fifo_t* pf);
void fifo_read(fifo_t* pf, uint8_t* data);
void fifo_write(fifo_t* pf, uint8_t data);
//void fifo_write_bytes(fifo_t* f, uint8_t* buf, uint8_t nr_bytes);

#endif
