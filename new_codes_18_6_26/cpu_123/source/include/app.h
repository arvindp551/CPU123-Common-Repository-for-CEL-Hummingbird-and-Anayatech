/***************************************************************************//**
 * @file     app.h
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
#ifndef APP_H__
#define APP_H__

#include "common_header.h"
#include "drvr_spi.h"
#include "fifo.h"
#include "interlock.h"

#define CMD_READ_STATUS				0xA1U
#define CMD_READ_INRLY				0xA2U
#define CMD_READ_INTERLOCK			0xA4U
#define CMD_READ_REG				0xA5U
#define CMD_READ_NON_VITAL			0xA6U
#define CMD_READ_FLASH_CHECKSUM		0xA7U
#define CMD_READ_REMOTE_INRLY		0xA8U
#define CMD_READ_VO					0xA8U
#define CMD_READ_LOGICAL_RELAY		0xA9U
#define CMD_READ_REMOTE_LO			0xAAU
#define CMD_READ_REMOTE_VO			0xABU
#define CMD_READ_INRLY_UNSTABLE		0xACU
#define CMD_READ_INTERLOCK_MODE		0xADU

#define CMD_WRITE_REG				0xB1U
#define CMD_WRITE_CPU_CTRLS			0xB4U

#define STATUS_OK					0
#define STATUS_FAIL					1

#define APROM_BANK_BYTES 			0x40000
#define APROM_BANKS 				2
#define APROM_TOTAL 				APROM_BANKS*APROM_BANK_BYTES

#define INTERLOCK_MODE_DOUBLE_LINE 	0x03U
#define INTERLOCK_MODE_MUX 			0x02U
#define INTERLOCK_MODE_SINGLE_LINE 	0x01U
#define INTERLOCK_MODE_NONE 		0x00U

#define INTER_READ_GAP_ms 			50U //200
#define DEBOUNCE_ROUNDS 			15U
				
typedef enum {
INIT_STATE,
POST_STATE,
WAIT_READY_STATE,
OPERATIONAL_STATE,
WATCHDOG_RESET
} CPU_state_e;

//configuration database structure
typedef struct {
	uint8_t relay_mask[5];
	uint8_t type;
	uint32_t expected_chksum;
} cfg_t;

//SSC - system status code database structure
typedef enum {
	SSC_LINE_CLOSE = 2,
	SSC_TRAIN_ON_LINE = 4,
	SSC_TRAIN_GOING_TO = 3,
	SSC_LINE_CLEAR = 5,
	SSC_ERROR = 8,
	SSC_RESET = 1
} status_codes_e;

// vital input pair database structure
typedef struct {
	uint8_t state_on_change;
	uint8_t state_now;
	uint8_t state_after_dbounce;
	uint8_t mask;
	uint16_t dbounce_ticks;
	uint16_t unstable_check_count;
} vi_pair_t;

// relay database structure
typedef struct {
	uint8_t status;
	uint8_t vi_remote[NR_VITAL_INPUT/8U]; //40 bits vital input from remote system unused
	uint8_t vi_dbounced[NR_VITAL_INPUT/8U]; //40 bits vital input after debouncing
	uint8_t oe_vi[NR_VITAL_INPUT/8U]; //40 bits remote side vital input
	uint8_t oe_lo[NR_LOGIC_RELAYS/8U]; //48 bits logical relays
	uint8_t oe_vo[NR_VITAL_OUTPUT/8U]; //48 bits logical relays
	vi_pair_t vi_pair[NR_VITAL_INPUT/2U];
	uint8_t le_vo[NR_VITAL_OUTPUT/8U]; // m_inv,m....k_inv,k  vital output
	uint8_t le_nvo[NR_NON_VITAL_OUTPUT/8U]; // m_inv,m....k_inv,k non vital output
	uint8_t run_flash_check_status;
	uint32_t flash_cksum;
	uint8_t vi_is_unstable[NR_VITAL_INPUT/8U];
	//added
	uint8_t rly_fdbk_byte[4];
	uint8_t misc_byte[4];
} db_t;

typedef struct __attribute__((__packed__)) {
	uint8_t run_flash_check : 1;
	uint8_t enable_hang : 1;
	uint8_t com0_state : 1;
	uint8_t com1_state : 1;
	uint8_t interlock_mode : 2;
	uint8_t SSBPAC_OK : 1;
	uint8_t SSBPAC_FAIL : 1;
	uint8_t oe_state_avl : 1;
	uint8_t reset_state : 2;
	uint8_t spare : 5;
} ctrls_struct_t;

typedef union {
	ctrls_struct_t x;
	uint8_t bytes[sizeof(ctrls_struct_t)];
}ctrls_union_t;

extern ctrls_union_t ctrls_u;
extern bool onesec_pulse;

extern void watchdog_init(void);

void vi_process(void);
void cpu_fpga_xfer(void);
void cpu_sm(void);
void spi_read_cb(void);
#endif
