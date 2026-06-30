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
 
#ifndef __UART_FIFO_H__
#define __UART_FIFO_H__

#include <stdlib.h>
#include <stdint.h>

//fifo database structure
typedef struct {
	uint16_t rdptr;
	uint16_t wrptr;
	uint8_t  *buf;
	uint16_t cksum_wr;
	uint16_t cksum_rd;
	uint8_t is_empty;
	uint8_t is_full;
	uint8_t err_overflow;
	uint8_t err_underrun;
	uint16_t fill;
	uint16_t bufsize;
} fifo_t;

uint8_t fifo_init(fifo_t* pf, uint8_t* pbuf, int bufsize);
uint8_t fifo_reset(fifo_t* pf);
uint8_t fifo_read(fifo_t* pf, uint8_t* data);
uint8_t fifo_write(fifo_t* pf, uint8_t data);
void fifo_write_bytes(fifo_t* f, uint8_t* buf, uint8_t nr_bytes);
uint8_t fifo_read_isr(fifo_t* pf, uint8_t* data);
uint8_t fifo_write_isr(fifo_t* pf, uint8_t data);

#endif
