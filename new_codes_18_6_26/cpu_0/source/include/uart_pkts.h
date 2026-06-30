/***************************************************************************//**
 * @file     uart_pkts.h
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
 
#ifndef __UART_PKTS_H__
#define __UART_PKTS_H__
#include "common_header.h"
#include "fifo.h"
#include "interlock.h"

uint8_t wr_sys_status_pkt(fifo_t* p);
uint8_t wr_card_status_pkt(fifo_t* p);
uint8_t wr_relay_status_pkt(fifo_t* p,uint8_t card, uint8_t channelNo);

uint8_t LINE_COM_read_packet(fifo_t* p, uint8_t com_id);
uint8_t LINE_COM_write_packet(fifo_t* p, uint8_t com_id);

#endif
