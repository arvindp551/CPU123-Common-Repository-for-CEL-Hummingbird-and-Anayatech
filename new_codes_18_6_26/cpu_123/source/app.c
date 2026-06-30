/***************************************************************************//**
 * @file     app.c
 * @brief    All application specific source files
 * @version  1.0.0
 * @details
 * Compiler : Keil uVision
 * Target : Nuvoton M263
 * Module : Central Controller Card
 * Date Created : Jan 23, 2026
 * @copyright Copyright (C) 2026 Anaya Tech Systems Pvt. Ltd. . All rights reserved.
 * @author    SG
 * @par Functions Included
 * void cpu_sm(void); <br>
 * static uint8_t get_flash_checksum(void); <br>
 * void cpu_fpga_xfer(void); <br>
 * void vi_process(void); <br>
 * void spi_read_cb(void); <br>
 * static void set_ctrl_defaults(void);
 ******************************************************************************/

//#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "NuMicro.h"
#include <wdt.h>
#include <fmc.h>
#include "common_header.h"
#include "app.h"
#include "bsp.h"
#include "drvr_spi.h"
#include "interlock.h"

// volatile added as the variable update may not be used immediately by other function
// which results in MISRAC:2025 R2.2 violation showing dead code.
static bool enable_watchdog = 0;
static volatile db_t db;
static volatile CPU_state_e cpu_state;
ctrls_union_t ctrls_u;

static uint8_t get_flash_checksum(void);
static void set_ctrl_defaults(void);

/**
 * @brief State machine for CPU123
 * There are three state INIT state, POST STATE, and OPERATIONAL state
 * At first state INIT state control all the bsp control
 * At OPERATIONAL state read all vital input, other end vitl input, other end logical output and other end vital output and perform all interlock operation
 * Check flashchecksum and inable the watchdog
 *
 * @param
 * None.
 *
 * @return
 * None.
 *
 * @details
 * Requirement ID(s) - 
 * PI_SSBPAC_SwRS_POST_001
 * PI_SSBPAC_SwRS_POST_003
 * 
 * @note
 * States:
 * INIT_STATE
 * POST_STATE (Power-On Self-Test)
 * WAIT_READY_STATE
 * OPERATIONAL_STATE
 *
 */

void cpu_sm(void) {

	uint8_t i;
	bool b;

	switch(cpu_state){
		case INIT_STATE:
			//initialize all interfaces, timers
			bsp_ctor();
			//set defaults
			interlock_set_defaults();
			set_ctrl_defaults();
			/*
			 * Once started watchdog can't be stopped
			 * watchdog runs on 10KHz (100us) tick from internal RC oscillator
			 * watchdog timeout set to 2pow14 = 16384 ticks = 16384*0.1ms = 1.638 sec
			 */
			//watchdog_init();

			wait_ms(2000);
			//go to Power ON Self Test state
			cpu_state = POST_STATE;
			break;
		case POST_STATE:  // POWER ON SELF TEST
			db.run_flash_check_status = get_flash_checksum();

			if(db.run_flash_check_status) {
				// flash checksum calculation failed
				for(;;){/*do nothing*/}
			}

			cpu_state = WAIT_READY_STATE;
			break;
		case WAIT_READY_STATE:

			//wait for CPU0 COM ready
			//wait for interlock mode setting
			//wait for other end state info
			b = ((ctrls_u.x.com0_state == 1) || (ctrls_u.x.com1_state == 1));
			b &= (ctrls_u.x.interlock_mode != INTERLOCK_MODE_NONE);
			b &= (ctrls_u.x.oe_state_avl != 0);

			WDT_RESET_COUNTER();
			if (b) {
				cpu_state = OPERATIONAL_STATE;
			}
			else {
				cpu_state = WAIT_READY_STATE;
			}

		case OPERATIONAL_STATE:
			for(i=0;(i<(NR_VITAL_INPUT/8U));i++) {
				vi_u.bytes[i] = db.vi_dbounced[i];
			}
			for(i=0;(i<(NR_VITAL_INPUT/8U));i++) {
				vi_oe_u.bytes[i] = db.oe_vi[i];
			}
			for(i=0;(i<(NR_LOGIC_RELAYS/8U));i++) {
				lo_oe_u.bytes[i] = db.oe_lo[i];
			}
			for(i=0;(i<(NR_VITAL_OUTPUT/8U));i++) {
				vo_oe_u.bytes[i] = db.oe_vo[i];
			}
			interlock();

			/*
			 * TODO: check how much time flash checksum takes
			 */
			if(ctrls_u.x.run_flash_check == 1) {
				db.run_flash_check_status = get_flash_checksum();
				ctrls_u.x.run_flash_check = 0;
			}

			// reset watchdog even when not enabled
			// Program may hang in a loop so not clearing in Interrupt service routine
			WDT_RESET_COUNTER();

			if(ctrls_u.x.enable_hang){ // @NOTE: FOR TESTING ONLY. enter into hang state for watchdog testing.
				while(true) {}
			}

			if(onesec_pulse) {
				onesec_pulse = false;
				CPU_HB = (CPU_HB != 0U)?0:1;
			}
			break;
		default:
			//let watchdog expire and reset CPU
			break;
	}
}

/**
 * @brief calculate the flash checksum
 *
 * @param
 * None.
 *
 * @return
 * uint8_t
 * 0 : Checksum calculation successful (no error)
 * 1 : Checksum calculation failed (error detected)
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note
 * Uses FMC (Flash Memory Controller) to compute CRC32 checksum.
 *
 */
static uint8_t get_flash_checksum(void) {
	uint8_t ret = 0;
	static uint32_t flash_addr = FMC_APROM_BASE;
	bool b;

	FMC_Open();	/* Enable FMC ISP function */

	// Request FMC hardware to run CRC32 calculation on APROM bank x.
	db.flash_cksum = FMC_GetChkSum(FMC_APROM_BASE, CODE_SIZE_ALIGNED);

	b = (g_FMC_i32ErrCode == 0) ? false : true;
	if(b) {
		ret = 1;
	}
	else {
		// do nothing
	}

	FMC_Close();

	return ret;
}

/**
* @brief
* Handles bidirectional data transfer between CPU and FPGA using
* clock-driven serialization and deserialization.
*
* This function implements a communication interface where:
* CPU serializes and transmits data to FPGA on clock rising edge.
* CPU receives and deserializes data from FPGA on clock falling edge.
*
* @param
* None.
*
* @return
* None.
*
* @details
* Requirement ID(s) - 
* PI_SSBPAC_SwRS_POST_018
*
*
* @note
* Communication signals:
* FPGA2CPU_CLK : Clock signal from FPGA
* FPGA2CPU_RST : Reset signal from FPGA
* FPGA2CPU_DATA : Serial data input from FPGA
* CPU2FPGA_DATA : Serial data output to FPGA
*
*/

void cpu_fpga_xfer(void) {
	uint32_t mask;
	uint8_t v;
	bool clk_rising;
	bool clk_falling;
	uint8_t clk_now = FPGA2CPU_CLK;
	uint8_t rst = FPGA2CPU_RST;
	static uint8_t st_i = 0;
	static uint8_t clk_last = 0;
	static uint8_t sm_state;
	static uint8_t st_k;
	static uint8_t i_1 = 0;
	static uint8_t sm_state_1;
	static uint8_t k_1;
	static uint8_t data_1;
	static uint8_t clks;

	clk_rising = (clk_now==1U) && (clk_last == 0U);
	clk_falling = (clk_now==0U) && (clk_last == 1U);
	clk_last = clk_now;

	if(rst == 0U) {
		st_i = 0;
		CPU2FPGA_DATA = 1;
		sm_state = 0;
		st_k = 0;
		sm_state_1 = 0;
		i_1 = 0;
		k_1 = 0;
		clks=0;
	}
	else if (clk_rising) {
		// serialize VI VO NVO LO
		clks++;
		switch(sm_state) {
			case 0: // send  vital inputs
				mask = ((uint32_t)0x01U << st_i);
				v = ((mask & db.vi_dbounced[st_k]) != 0U) ? 1 : 0;
				CPU2FPGA_DATA = v;
				st_i++;
				if(st_i==8U) {
					st_i = 0;
					st_k++;
					if(st_k == (NR_VITAL_INPUT/8U)) {
						sm_state = 1;
						st_k = 0;
					}
				}
				break;
			case 1: // send vital outputs
				mask = ((uint32_t)0x01U << st_i);
				v = ((mask & vo_u.bytes[st_k]) != 0U) ? 1 : 0;
				CPU2FPGA_DATA = v;
				st_i++;
				if(st_i==8U) {
					st_i = 0;
					st_k++;
					if(st_k==(NR_VITAL_OUTPUT/8U)) {
						st_k = 0;
						sm_state = 2;
					}
				}
				break;
			case 2: // send logical outputs
				mask = ((uint32_t)0x01U << st_i);
				v = ((mask & lo_u.bytes[st_k]) != 0U) ? 1 : 0;
				CPU2FPGA_DATA = v;
				st_i++;
				if(st_i==8U) {
					st_i = 0;
					st_k++;
					if(st_k==(NR_LOGIC_RELAYS/8U)) {
						st_k = 0;
						sm_state = 3;
					}
				}
				break;
			case 3: // send non vital outputs
				mask = ((uint32_t)0x01U << st_i);
				v = ((mask & nvo_u.bytes[st_k]) != 0U) ? 1 : 0;
				CPU2FPGA_DATA = v;
				st_i++;
				if(st_i==8U) {
					st_i = 0;
					st_k++;
					if(st_k==(NR_NON_VITAL_OUTPUT/8U)) {
						st_k = 0;
						sm_state = 0;
					}
				}
				break;

			default:
				// if here it's an error
				break;
		}
	}
	else if (clk_falling) {
		switch(sm_state_1) {
			// deserialize OE_VI OE_LO
			case 0: // receive remote end vital inputs
				data_1 >>= 1U;	//shift right
				data_1 &= (~(uint8_t)0x80);//clear msb
				data_1 |= ((FPGA2CPU_DATA << 7U) & 0x80U); //FPGA2CPU_DATA is lsb first so right shift
				i_1++;
				if(i_1==8U) {
					i_1 = 0;
					db.oe_vi[k_1] = data_1;
					data_1 = 0;
					k_1++;
					if(k_1 == (NR_VITAL_INPUT/8U)) {
						sm_state_1 = 1;
						k_1 = 0;
					}
				}
				break;
			case 1: // vital output of other end
				data_1 >>= 1U;	//shift right
				data_1 &= (~(uint8_t)0x80);//clear msb
				data_1 |= ((FPGA2CPU_DATA << 7U) & 0x80U); //FPGA2CPU_DATA is lsb first so right shift
				i_1++;
				if(i_1==8U) {
					i_1 = 0;
					db.oe_vo[k_1] = data_1;
					data_1 = 0;
					k_1++;
					if(k_1 == (NR_VITAL_OUTPUT/8U)) {
						sm_state_1 = 2;
						k_1 = 0;
					}
				}
				break;
			case 2: // logic input from other end
				data_1 >>= 1U;	//shift right
				data_1 &= (~(uint8_t)0x80);//clear msb
				data_1 |= (uint8_t)((FPGA2CPU_DATA << 7U) & 0x80U); //FPGA2CPU_DATA is lsb first so right shift
				i_1++;
				if(i_1==8U) {
					i_1 = 0;
					db.oe_lo[k_1] = data_1;
					data_1 = 0;
					k_1++;
					if(k_1 == (NR_LOGIC_RELAYS/8U)) {
						sm_state_1 = 3;
						k_1 = 0;
					}
				}
				break;
			case 3: break; //do nothing
			default:
					// if here it's an error
					break;
		}
	}
	else {
		//do nothing
	}
}

/**
 * @brief
 *  Read and Process Vital Inputs from Input Card
 *	Read Vital Inputs serially
 *	Control RST and CLK timing to read VI serially through DATA input
 *	Debounce each VI pair to read a stable value
 *	Detect errors 1. Non complimentary 2. Unstable Input
 *
 * @param
 * None.
 *
 * @return
 * None.
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note
 * * State machine operation:
 * State 0
 * State 1
 * State 2
 * State 3
 * State 4
 */

void vi_process(void) {
	static uint8_t lstate = 0;
	static uint8_t relayidx = 0;
	static uint16_t read_gap = INTER_READ_GAP_ms;
	uint8_t i,d;
	uint8_t byteidx,bitidx;

	//wait for 2s before next relay read
	switch (lstate) {
		case 0:  // Assert RST to input card for sampling relay state
			if(read_gap != 0U) {read_gap--;}
			else {lstate = 1;}
			RELAY_RST = 0;
			break;
		case 1: // Deassert RST
			RELAY_RST = 1;
			lstate = 2;
			break;
		case 2: // Get ready for serial read
			relayidx = 0;
			lstate = 3;
			break;
		case 3: // Set clock high
			RELAY_CLK = 1;
			lstate = 4;
			break;
		case 4: // Set clock low and process serial data.
				// Debounce relay state
			RELAY_CLK = 0;
			i = (relayidx/2U);
			d = RELAY_DATA;

			if((relayidx%2U) == 0U) {
				if(d != 0U) {db.vi_pair[i].state_now |= (uint8_t)0x01U;}
				else  {db.vi_pair[i].state_now &= (~(uint8_t)0x01U);}
			}
			else {
				if(d != 0U) {db.vi_pair[i].state_now |= (uint8_t)0x02U;}
				else  {db.vi_pair[i].state_now &= (~(uint8_t)0x02U);}
			}

			if((relayidx%2U) == 1U) { // debounce once both bits of pair rcvd
				byteidx = (relayidx/8U);
				bitidx  = (relayidx/2U);
				bitidx %= 4U;
				bitidx *= 2U;
				// latch debounced state when no change seen for DEBOUNCE PERIOD
				// load debounce counter and relay changed state
				if(db.vi_pair[i].state_on_change != db.vi_pair[i].state_now) {
					db.vi_pair[i].dbounce_ticks = DEBOUNCE_ROUNDS;
					db.vi_pair[i].state_on_change = db.vi_pair[i].state_now;
					db.vi_pair[i].unstable_check_count++;
					if(db.vi_pair[i].unstable_check_count > DEBOUNCE_ROUNDS) {
						db.vi_is_unstable[byteidx] &= (~((uint8_t)0x03U << bitidx)); //clear unstable bits for vi relay pair
						db.vi_is_unstable[byteidx] |= ((uint8_t)0x03U << bitidx);  //set unstable bits for vi relay pair
					}
				}
				else if(db.vi_pair[i].dbounce_ticks != 0U){
					db.vi_pair[i].dbounce_ticks -= 1U;
				}
				else { // input pair stable now
					db.vi_pair[i].state_after_dbounce = db.vi_pair[i].state_on_change;
					db.vi_pair[i].unstable_check_count = 0;
					db.vi_is_unstable[byteidx] &= (~((uint8_t)0x03U << bitidx)); //clear unstable bits for vi relay pair
																				 // clear 2 bits based on relay index
					db.vi_dbounced[byteidx] &= (~((uint8_t)0x03U << bitidx));
					db.vi_dbounced[byteidx] |= (db.vi_pair[i].state_after_dbounce << bitidx);
				}
			}
			relayidx++;

			if(relayidx == 40U) {
				lstate = 0;
				read_gap = INTER_READ_GAP_ms;
			}
			else {
				lstate = 3;
			}
			break;

		default: {
					 // Shouldn't be here
					 break;
				 }
	}
}

/**
 * @brief  Handles SPI read callback and processes incoming commands from CPU0.
 *         This function reads commands from RX FIFO, validates checksum, executes
 *         the requested operation, and writes the response into TX FIFO.
 *         It supports multiple commands such as reading status, flash checksum,
 *         relay states, interlock data, and writing control/configuration values.
 *
 * @param  [in,out]  spi_cpux.prx   Pointer to RX FIFO containing received SPI data.
 * @param  [in,out]  spi_cpux.ptx   Pointer to TX FIFO used to send response data.
 *
 * @return None
 *
 * @details
 * Requirement ID(s) - 
 * PI_SSBPAC_SwRS_POST_015
 * PI_SSBPAC_SwRS_POST_017
 *
 * @note
 * - Uses checksum and inverted checksum for data integrity validation.
 * - RX FIFO is fully processed and then reset at the end of execution.
 * - TX FIFO is cleared at the start before writing response.
 * - Supports both read and write commands (CMD_READ_*, CMD_WRITE_*).
 * - Some commands (e.g., CMD_WRITE_CPU_CTRLS) modify global control structures.
 * - MISRA deviations are handled by explicit "do nothing" branches.
 * - Assumes FIFO operations (fifo_read/write/reset) are safe and validated externally.
 * - Break/return is used when checksum validation fails to prevent invalid processing.
 */

void spi_read_cb(void) {
	// command(1B),Data(0-20B)
	// response data depends on command
	// Commands

	uint8_t ch;
	uint8_t reg;
	uint8_t i;
	uint8_t chksum = 0;
	uint8_t c;
	uint8_t invc;
	uint8_t user_cmd_flag;
	uint8_t b[4];

	// Deviation MISRA C:2025 R2.2
	// frx and ftx are local variables which hold fifo pointers
	// that is ok and can't cause any issue
	fifo_t* frx = spi_cpux.prx;
	fifo_t* ftx = spi_cpux.ptx;

	fifo_reset(ftx); //empty tx fifo
	while(!frx->is_empty) {
		// read data and process
		fifo_read(frx,&ch); //command
		chksum += ch;
		if((ch != CMD_WRITE_REG) && (ch != CMD_READ_REG) && (ch!= CMD_WRITE_CPU_CTRLS)) {
			fifo_read(frx,&c); //chksum
			fifo_read(frx,&invc); //inverted cheksum
			invc = ~invc;
			if((chksum != c) || (chksum != invc)) {break;}
			else {
				// do nothing
				// Added to avoid MISRA C violation
			}
		}
		else {
			// do nothing
			// Added to avoid MISRA C violation
		}

		if(ch == CMD_READ_STATUS) {
			db.status = (uint8_t)cpu_state;
			fifo_write(ftx,db.status);
			chksum = db.status;
			fifo_write(ftx,chksum); //checksum
			chksum = ~chksum;
			fifo_write(ftx,chksum); //inverted checksum
		}
		else if(ch == CMD_READ_FLASH_CHECKSUM) { // 4B flash checksum (LSB first), last flash check run status
			uint8_t p_data;
			chksum = 0;
			for(i=0;i<4U;i++) {
				p_data = (uint8_t)((db.flash_cksum >> (i*8U)) & 0xFFU);
				fifo_write(ftx,p_data);
				chksum += p_data;
			}
			fifo_write(ftx,db.run_flash_check_status);
			chksum += db.run_flash_check_status;
			fifo_write(ftx,chksum); //checksum
			chksum = ~chksum;
			fifo_write(ftx,chksum); //inverted checksum
		}
		else if(ch == CMD_READ_INRLY) {
			chksum = 0;
			for(i=0;i<(NR_VITAL_INPUT/8U);i++) {
				fifo_write(ftx,db.vi_dbounced[i]);
				chksum += db.vi_dbounced[i];
			}
			fifo_write(ftx,chksum); //checksum
			chksum = ~chksum;
			fifo_write(ftx,chksum); //inverted checksum
		}
		else if(ch == CMD_READ_REMOTE_INRLY) {
			chksum = 0;
			for(i=0;i<(NR_VITAL_INPUT/8U);i++) {
				fifo_write(ftx,db.oe_vi[i]);
				chksum += db.oe_vi[i];
			}
			fifo_write(ftx,chksum); //checksum
			chksum = ~chksum;
			fifo_write(ftx,chksum); //inverted checksum
		}
		else if(ch == CMD_READ_REMOTE_LO) {
			chksum = 0;
			for(i=0;i<(NR_LOGIC_RELAYS/8U);i++) {
				fifo_write(ftx,db.oe_lo[i]);
				chksum += db.oe_lo[i];
			}
			fifo_write(ftx,chksum); //checksum
			chksum = ~chksum;
			fifo_write(ftx,chksum); //inverted checksum
		}
		else if(ch == CMD_READ_REMOTE_VO) {
			chksum = 0;
			for(i=0;i<(NR_VITAL_OUTPUT/8U);i++) {
				fifo_write(ftx,db.oe_vo[i]);
				chksum += db.oe_vo[i];
			}
			fifo_write(ftx,chksum); //checksum
			chksum = ~chksum;
			fifo_write(ftx,chksum); //inverted checksum
		}
		else if(ch == CMD_READ_INTERLOCK) {
			chksum = 0;
			for(i=0;i<(NR_VITAL_OUTPUT/8U);i++) {
				fifo_write(ftx,vo_u.bytes[i]);
				chksum += vo_u.bytes[i];
			}
			fifo_write(ftx,chksum); //checksum
			chksum = ~chksum;
			fifo_write(ftx,chksum); //inverted checksum
		}
		else if(ch == CMD_READ_LOGICAL_RELAY) { //not used may be used for testing only
			chksum = 0;
			for(i=0;i<(NR_LOGIC_RELAYS/8U);i++) {
				fifo_write(ftx,lo_u.bytes[i]);
				chksum += lo_u.bytes[i];
			}
			fifo_write(ftx,chksum); //checksum
			chksum = ~chksum;
			fifo_write(ftx,chksum); //inverted checksum
		}
		else if(ch == CMD_READ_INRLY_UNSTABLE) {
			chksum = 0;
			for(i=0;i<(NR_VITAL_INPUT/8U);i++) {
				fifo_write(ftx,db.vi_is_unstable[i]);
				chksum += db.vi_is_unstable[i];
			}
			fifo_write(ftx,chksum); //checksum
			chksum = ~chksum;
			fifo_write(ftx,chksum); //inverted checksum
		}
		else if(ch == CMD_READ_INTERLOCK_MODE) {
			fifo_write(ftx,ctrls_u.x.interlock_mode);
			chksum = ctrls_u.x.interlock_mode;
			fifo_write(ftx,chksum); //checksum
			chksum = ~chksum;
			fifo_write(ftx,chksum); //inverted checksum
		}
		else if(ch == CMD_WRITE_CPU_CTRLS) { //CPU0 sends this command to run flash checksum calculation
			for(i=0U;i<sizeof(ctrls_struct_t);i++) {
				fifo_read(frx,&b[i]);
				chksum+=b[i];
			}

			fifo_read(frx,&c);
			fifo_read(frx,&invc);
			invc = ~invc;
			if((chksum == c) && (chksum == invc)) {
				fifo_write(ftx,0xAA); //response OK
				chksum = 0xAA;
				fifo_write(ftx,chksum); //checksum
				fifo_write(ftx,~chksum); //inverted checksum

				for(i=0U;i<sizeof(ctrls_struct_t);i++) {
					ctrls_u.bytes[i] = b[i];
				}
			}
			else {
				// do nothing
				// Added to avoid MISRA C violation
			}
		}
		else if(ch == CMD_READ_REG) {
			fifo_read(frx,&reg); // register#
			chksum +=reg;
			if(reg == 0x00U) { //Relay Enable Setting
				fifo_read(frx,&ch); // Relay#
				chksum += ch;
				fifo_read(frx,&c); //chksum
				fifo_read(frx,&invc); //inverted cheksum
				if((chksum != c) || (chksum != (~invc))) {break;}
				else {
					// do nothing
					// Added to avoid MISRA C violation
				}

				if ((ch%2U) == 0U) {//even relay
					i = db.vi_pair[ch/2U].mask & 0x01U; //read status at bit0
				}
				else {//odd relay
					i = db.vi_pair[ch/2U].mask & 0x02U; //read status at bit1
				}
				ch = (i!=0U) ? 1U : 0U; // status
				fifo_write(ftx,ch);
				chksum = ch;
				fifo_write(ftx,chksum); //checksum
				fifo_write(ftx,~chksum); //checksum
			}
			else {
				// do nothing
				// Added to avoid MISRA C violation
			}
		}
		else if(ch == CMD_WRITE_REG) {
			fifo_read(frx,&ch); // register#
			chksum += ch;
			if(ch == 0x00U) { //Relay Enable Setting
				fifo_read(frx,&ch); // Relay#
				chksum += ch;
				fifo_read(frx,&i); // setting 1-Enabled, 0-Disabled
				chksum += i;
				fifo_read(frx,&c); // checksum
				fifo_read(frx,&invc); // inverted checksum
				if((chksum != c) || (chksum != (~invc))) {break;}
				else {
					// do nothing
					// Added to avoid MISRA C violation
				}

				if ((ch%2U) == 0U) {//even relay
					db.vi_pair[ch/2U].mask &= (~(uint8_t)0x01U); //clear status at bit0
					db.vi_pair[ch/2U].mask |= (uint8_t)((i!=0U)?0x01U:0x00U); //write setting
				}
				else {//odd relay
					db.vi_pair[ch/2U].mask &= (~(uint8_t)0x2U); //clear status at bit1
					db.vi_pair[ch/2U].mask |= (uint8_t)((i!=0U)?0x02U:0x00U); //write setting
				}
				fifo_write(ftx,0xAA); //response OK
				chksum = 0xAA;
				fifo_write(ftx,chksum); //checksum
				fifo_write(ftx,~chksum); //checksum
			}
			else if(ch == 0x01U) { //type of interlock logic. DL, SL or Testmode
				fifo_read(frx,&i); // setting
				chksum += i;
				fifo_read(frx,&c); // checksum
				fifo_read(frx,&invc); // inverted checksum
				if((chksum != c) || (chksum != (~invc))) {return;}
				else {
					// do nothing
					// Added to avoid MISRA C violation
				}

				fifo_write(ftx,0xAA); //response OK
				chksum = 0xAA;
				fifo_write(ftx,chksum); //checksum
				fifo_write(ftx,~chksum); //checksum
			}
			else {
				// do nothing
				// Added to avoid MISRA C violation
			}
		}

		else {
			// do nothing
			// Added to avoid MISRA C violation
		}
	}
	fifo_reset(frx); //empty rx fifo
}

/**
 * @brief  Initializes CPU control structure with default values.
 *         This function resets all control flags and status indicators
 *         in the control structure to a known safe state during startup.
 *
 * @param  None
 *
 * @return None
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note
 * - All control flags are cleared (set to 0).
 * - Ensures system starts in a deterministic and safe configuration.
 * - Typically called during initialization phase (e.g., INIT_STATE).
 * - Must be executed before using ctrl signals in operational logic.
 */

static void set_ctrl_defaults(void) {
	ctrls_u.x.run_flash_check = 0;
	ctrls_u.x.enable_hang = 0;
	ctrls_u.x.com0_state = 0;
	ctrls_u.x.com1_state = 0;
	ctrls_u.x.interlock_mode = 0;
	ctrls_u.x.SSBPAC_OK = 0;
	ctrls_u.x.SSBPAC_FAIL = 0;
	ctrls_u.x.oe_state_avl = 0;
	ctrls_u.x.spare = 0;
	ctrls_u.x.reset_state = 3;
}
