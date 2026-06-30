/***************************************************************************//**
 * @file     cli.c
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
 * void send_el_pkt(void); <br>
 * static uint16_t get_sys_error(void); <br>
 * static void get_line_state_sl(void); <br>
 * static void get_line_state_dl(void); <br>
 * char* my_strtok(char* str, char delim); <br>
 * static void conv_byte2event(uint8_t nr_bytes, uint8_t* p_byte, event_t* p_event); <br>
 * static void conv_byte2event_nvo(uint8_t nr_bytes, uint8_t* p_byte, event_t* p_event);
 ******************************************************************************/

#include "app.h"
#include "bsp.h"
#include "uart_pkts.h"
#include "SH1106.h"

#define TRAIN_ERR 0U
#define LINE_CLEAR 1U
#define TRAIN_MOVING 2U 
#define TRAIN_EXIT 3U 

static void conv_byte2event(uint8_t nr_bytes, uint8_t* p_byte, event_t* p_event);
static void conv_byte2event_nvo(uint8_t nr_bytes, uint8_t* p_byte, event_t* p_event);
static void conv_byte2event_misc(uint8_t nr_bytes, uint8_t* p_byte, event_t* p_event);

static void get_line_state_sl(void);
static void get_line_state_dl(void);
static uint16_t get_sys_error(void);

uint8_t oe_relay_state[OE_RELAY_STATE_WIDTH];
static uint8_t oe_relay_state_c[OE_RELAY_STATE_WIDTH];

static uint8_t en_write_oe_relay_state = 1;

/**
 * @brief Processes system events and sends event logger (EL) packets.
 *
 * This function evaluates system inputs/outputs, detects changes and faults,
 * generates corresponding events, and transmits event packets via the event
 * logger UART interface. It performs state conversion, error detection,
 * event generation, and prioritized packet transmission.
 *
 * @param [in,out] None
 *
 * @return void
 *
 * @details
 * Requirement ID(s) -
 * PI_SSBPAC_SwRS_OPER_072
 *
 * @note
 * - Converts raw VI, VO, LO, NVO, and feedback bytes into event structures.
 * - Detects multiple fault conditions including:
 *   - CPU/FPGA no response
 *   - Flash/ROM checksum errors
 *   - VI instability and mismatch (2oo3 voting check)
 *   - VO mismatch and relay feedback mismatch
 *   - Non-complimentary relay inputs
 * - Assigns error codes based on priority and generates events on change.
 * - Monitors system-level changes such as:
 *   - Line state (single/double line mode)
 *   - Communication state
 *   - System error code
 * - Updates OLED display when line state changes.
 * - Event packets are transmitted only when TX FIFO is empty, in priority:
 *   1. Relay events (VI/VO)
 *   2. Card-level events
 *   3. System-level events
 * - Ensures one packet at a time to avoid FIFO congestion.
 */

void send_el_pkt(void) {
	uint8_t idx;
	uint8_t i,j,k,v,d;
	event_t* s;

	/*
	 * Steps for error code and event generation
	 * 1. Convert VI,VO,NVO,
	 *
	 * Describe error code mechanism
	 */

	/********* updating states and generate input and output level events ***********/
	// vital input
	for(i=0U;i<4U;i++){
		conv_byte2event(NR_VITAL_INPUT/8U, pe[i].vi_byte, pe[i].vi);
	}

	//vital output
	for(i=0U;i<4U;i++) {
		conv_byte2event(NR_VITAL_OUTPUT/8U, pe[i].vo_byte, pe[i].vo);
	}
	
	//Logical output
	for(i=0U;i<4U;i++) {
		conv_byte2event(NR_LOGIC_RELAYS/8U, pe[i].lo_byte, pe[i].lo);
	}
	//non vital output
	conv_byte2event_nvo(NR_NON_VITAL_OUTPUT/8U, pe[3].nvo_byte, pe[3].nvo);

	//vital output feedback fault as detected by FPGA
	conv_byte2event_misc(NR_VITAL_OUTPUT/8U, pe[3].vo_fdbk_fault_byte, pe[3].vo_fdbk_fault);
	conv_byte2event(NR_VITAL_OUTPUT/8U, pe[3].vo_fdbk_byte, pe[3].vo_fdbk);

	// other end (oe) vital input
	for(i=0U;i<3U;i++) {
		conv_byte2event(NR_VITAL_INPUT/8U, pe[i].oe_vi_byte, pe[i].oe_vi);
	}
	// other end (oe) vital output
	for(i=0U;i<3U;i++) {
		conv_byte2event(NR_VITAL_OUTPUT/8U, pe[i].oe_vo_byte, pe[i].oe_vo);
	}
	// other end (oe) logical output
	for(i=0U;i<3U;i++) {
		conv_byte2event(NR_LOGIC_RELAYS/8U, pe[i].oe_lo_byte, pe[i].oe_lo);
	}

	// vital input stabity state
	for(i=0U;i<4U;i++) {
		conv_byte2event_misc(NR_VITAL_INPUT/8U, pe[i].vi_unstable_byte, pe[i].vi_unstable);
	}

	for(i=0U;i<3U;i++) {
		// check if vital input of CPU 1,2,3 matches with each other
		// Compare VI of each CPU with FPGA 2oo3 majority voter result
		pe[i].err_vi_mismatch = 0;
		if(pe[i].vi_byte != pe[3].vi_byte) {
			pe[i].err_vi_mismatch = 1;
			break;
		}
	}

	for(i=0U;i<3U;i++) {
		// check if vital output from interlock logic of CPU 1,2,3 matches with each other
		// Compare VO of each CPU with FPGA 2oo3 majority voter result
		pe[i].err_vo_mismatch = false;
		if(pe[i].vo_byte != pe[3].vo_byte) {
			pe[i].err_vo_mismatch = true;
			break;
		}
	}

	for(i=0U;i<3U;i++) {
		// check if both the signals of vital input pair of CPU 1,2,3 are complimentary
		pe[i].err_vi_non_complimentary = false;
		for(k=0U;k<(NR_VITAL_INPUT/2U);k++) {
			if(pe[i].vi[k].err_code == ERR_RELAY_NON_COMPLIMENTARY) {
				pe[i].err_vi_non_complimentary = true;
				break;
			}
		}
	}

	for(i=0U;i<3U;i++) {
		// check if both the signals of vital input pair of CPU 1,2,3 are stable as detected by debounce logic
		pe[i].err_vi_unstable = false;
		for(k=0U;k<(NR_VITAL_INPUT/2U);k++) {
			if(pe[i].vi_unstable[k].state != 0U) {
				pe[i].err_vi_unstable = true;
				break;
			}
		}
	}

	// check if feedback from relays driven by FPGA matches with driven state
	// only first 5 relays are used
	for(i=0U;i<4U;i++) {
		// Not valid for CPU1,2,3 so keep mismatch 0 and reset FPGA mismatch state to 0 before computing
		pe[i].err_vo_fdbk_mismatch = false;
	}
	for(i=0U;i<5U;i++) {
		if(pe[3].vo_fdbk_fault[i].state != 0U) {
			pe[3].err_vo_fdbk_mismatch = true;
			break;
		}
	}

	// CPU and FPGA device level errors in priority order from highest to lowest
	for(i=0U;i<4U;i++) {
		if(pe[i].err_no_response) {
			pe[i].err_code = ERR_CARD_NO_RESPONSE;
		}
		else if(pe[i].err_rom_faulty) {
			pe[i].err_code = ERR_CARD_ROM_FAULTY;
		}
		else if(pe[i].err_vi_unstable) {
			pe[i].err_code = ERR_CARD_VI_UNSTABLE;
		}
		else if(pe[i].err_vi_mismatch) {
			pe[i].err_code = ERR_CARD_VI_MISMATCH;
		}
		else if(pe[i].err_vo_mismatch) {
			pe[i].err_code = ERR_CARD_VO_MISMATCH;
		}
		else if(pe[i].err_vi_non_complimentary) {
			pe[i].err_code = ERR_CARD_VI_NON_COMPLIMENTARY;
		}
		else if(pe[i].err_vo_fdbk_mismatch) {
			pe[i].err_code = ERR_CARD_VO_FDBK_MISMATCH;
		}
		else {
			pe[i].err_code = ERR_CARD_NONE;
		}

		// Generate event on err code change
		if(pe[i].err_code != pe[i].err_code_c) {
			pe[i].ev_pending = true;
		}
		pe[i].err_code_c = pe[i].err_code;
	}

	// Check track line state error
	if(cpu_ctrls_u.x.interlock_mode == INTERLOCK_MODE_SINGLE_LINE) {
		get_line_state_sl();
		if(strcmp(sys.line_state_TGT,sys.line_state_TGT_c) != 0U) {
			strcpy(sys.line_state_TGT_c,sys.line_state_TGT);
			//Clear line
			SH1106_GotoXY(0,16);
			SH1106_Puts("                ",&Font_7x10,SH1106_COLOR_WHITE);
			
			//Print line state
			SH1106_GotoXY(0,16);
			SH1106_Puts(sys.oled,&Font_7x10,SH1106_COLOR_WHITE);

			sys.ev_pending = true;
			ev.update_oled = true;
		}
	}
	else if(cpu_ctrls_u.x.interlock_mode == INTERLOCK_MODE_DOUBLE_LINE) {
		get_line_state_dl();

		if (strcmp(sys.line_state_TGT,sys.line_state_TGT_c)) {
			strcpy(sys.line_state_TGT_c,sys.line_state_TGT);
			sys.ev_pending = true;
			ev.update_oled = true;
		}

		if(strcmp(sys.line_state_TCF,sys.line_state_TCF_c) != 0U) {
			strcpy(sys.line_state_TCF_c,sys.line_state_TCF);
			sys.ev_pending = true;
			ev.update_oled = true;
		}
	}
	else{
		// do nothing
	}

	if(sys.com_state != sys.com_state_c) {
		sys.com_state_c = sys.com_state;
		sys.ev_pending = true;
	}

	sys.error = get_sys_error();
	if(sys.error != sys.error_c) {
		sys.error_c = sys.error;
		sys.ev_pending = true;
	}

	// trigger event packet
	fifo_t* f = uart_el.ptx;

	/******** Relay Events *********/
	// send packet when event is pending and event fifo has no packet pending

	if(f->is_empty == 1U) {
		for(k=0U;k<(NR_VITAL_INPUT/2U);k++) {
			if(pe[3].vi[k].ev_pending) { 
				wr_relay_status_pkt(f,IN_RLY_CARD,k);
				pe[3].vi[k].ev_pending = false;
			}
		}

		for(k=0U;k<(NR_VITAL_OUTPUT/2U);k++) {
			if(pe[3].vo[k].ev_pending) {
				wr_relay_status_pkt(f,OUT_RLY_CARD,k);
				pe[3].vo[k].ev_pending = false;
			}
		}

		/******** Card Events *********/
		for(k=0U;k<4U;k++) {
			if(pe[k].ev_pending) { 
				wr_card_status_pkt(f);
				pe[0].ev_pending = false;
				pe[1].ev_pending = false;
				pe[2].ev_pending = false;
				pe[3].ev_pending = false;
				break; // no more events as all card events sent in one packet
			}
		}

		/******** System Events *********/
		if(sys.ev_pending) { 
			wr_sys_status_pkt(f);
			sys.ev_pending = false;
		}
	}
}

/**
 * @brief Computes the system-level error code based on priority of detected faults.
 *
 * This function evaluates various error conditions across CPUs and FPGA in a 
 * predefined priority order (highest to lowest). It decrements an internal 
 * error code counter and returns immediately when a matching fault condition 
 * is detected. If no errors are found, it returns 0.
 *
 * @param[in]  None
 *
 * @return uint16_t 
 *         - Non-zero error code (lower value indicates higher priority fault)
 *         - 0 if no error is present in the system
 *
 * @details
 * Requirement ID(s) -
 * PI_SSBPAC_SwRS_OPER_073
 *
 * @note
 * - Error priority sequence:
 *   1. No response from CPU/FPGA
 *   2. ROM/Flash fault in CPUs
 *   3. Vital input non-complimentary error
 *   4. Vital input instability
 *   5. Vital input mismatch
 *   6. Vital output mismatch
 *   7. Vital output feedback mismatch
 * - Error code starts from 9999 and decrements for each check.
 * - Function exits immediately on first detected error (highest priority).
 * - Designed for deterministic fault reporting in safety-critical systems.
 */

static uint16_t get_sys_error(void) {
	uint16_t err_code = 9999;
	uint8_t idx,i;

	//Check no response error from CPU1,2,3, FPGA
	for(idx=0U;idx<4U;idx++) {
		err_code--;
		if(pe[idx].err_no_response) {
			return err_code;
		}
	}

	//Check Flash check status from CPU1,2,3 - Not required for FPGA
	for(idx=0U;idx<3U;idx++) {
		err_code--;
		if(pe[idx].err_rom_faulty) {
			return err_code;
		}
	}

	//Check Vital Input pair state (should be complimentary)
	for(idx=0U;idx<3U;idx++) {
		for(i=0U;i<(NR_VITAL_INPUT/2U);i++) {
			err_code--;
			if(pe[idx].err_vi_non_complimentary) {
				if(pe[idx].vi[i].err_code == ERR_RELAY_NON_COMPLIMENTARY) {
					return err_code;
				}
			}
		}	
	}

	//Check Vital Input stablity
	for(idx=0U;idx<3U;idx++) {
		for(i=0U;i<(NR_VITAL_INPUT/2U);i++) {
			err_code--;
			if(pe[idx].err_vi_unstable) {
				if(pe[idx].err_vi_unstable) {
					return err_code;
				}
			}		
		}
	}

	//Check Vital Inputs to CPU1,2,3 should match with output of 2oo3 in FPGA
	for(idx=0U;idx<3U;idx++) {
		for(i=0U;i<(NR_VITAL_INPUT/2U);i++) {
			err_code--;
			if(pe[idx].err_vi_mismatch) {
				if(pe[idx].vi[i].state != pe[3].vi[i].state) {
					return err_code;
				}
			}
		}
	}

	//Check Vital Output after 2oo3 should match CPU1,2,3 interlock vital output
	for(idx=0U;idx<3U;idx++) {
		for(i=0U;i<(NR_VITAL_OUTPUT/2U);i++) {
			err_code--;
			if(pe[idx].err_vo_mismatch) {
				if(pe[idx].vo[i].state != pe[3].vo[i].state) {
					return err_code;
				}
			}
		}	
	}

	//Check Relay feedback should match with Vital relay drive state
	//First 5 relays are being driven
	for(i=0U;i<4U;i++) {
		err_code--;
		if(pe[3].err_vo_fdbk_mismatch) {
			if(pe[3].vo[i].state != pe[3].vo_fdbk[i].state) {
				return err_code;
			}
		}
	}	

	return 0;
}

/**
 * @brief Determines and updates line state for single-line interlocking mode.
 *
 * This function evaluates Non-Vital Output (NVO) signals from FPGA to determine
 * the current track line state in single-line mode. Based on the decoded state,
 * it updates system line status strings and corresponding OLED display message.
 *
 * @param[in,out] None
 *
 * @return void
 *
 * @details
 * Requirement ID(s) -
 * PI_SSBPAC_SwRS_OPER_074
 *
 * @note
 * - Uses NVO data (pe[3].nvo_byte) to derive line state.
 * - Updates:
 *      - sys.line_state_TGT (short code representation)
 *      - sys.oled (human-readable display string)
 * - Possible states include:
 *      - "LC" : Line Close
 *      - "OU" : Train On Line
 *      - "CC" : Line Clear
 *      - "TG" : Train Going To
 *      - "EU" : Train Exit
 *      - "RU" : Line Error (default/fallback)
 * - sys.line_state_TCF is marked as "NV" (Not Valid) in single-line mode.
 */

static void get_line_state_sl(void){
	nvo_union_t* nvo_lcl = (nvo_union_t*)pe[3].nvo_byte;
	if(nvo_lcl->x.LC) {
			strcpy(sys.line_state_TGT,"LC");
			strcpy(sys.oled,"Line Close");
		}
		else if(nvo_lcl->x.TGT_R_TOL) {
			strcpy(sys.line_state_TGT,"OU");
			strcpy(sys.oled,"Train On Line");
		}
		else if(nvo_lcl->x.LINE_STATUS_TGT == LINE_CLEAR) {
			strcpy(sys.line_state_TGT,"CC");
			strcpy(sys.oled,"Line Clear");
		}
		else if(nvo_lcl->x.LINE_STATUS_TGT == TRAIN_MOVING) {
			strcpy(sys.line_state_TGT,"TG");
			strcpy(sys.oled,"Going To");
		}
		else if(nvo_lcl->x.LINE_STATUS_TGT == TRAIN_EXIT) {
			strcpy(sys.line_state_TGT,"EU");
			strcpy(sys.oled,"Train Exit");
		}
		else  {
			strcpy(sys.line_state_TGT,"RU");
			strcpy(sys.oled,"Line Error");
		}

		strcpy(sys.line_state_TCF,"NV"); //nat valid
}

/**
 * @brief Determines and updates line state for double-line interlocking mode.
 *
 * This function evaluates Non-Vital Output (NVO) signals from FPGA to determine
 * the current track line states for both directions:
 * - TCF (Train Coming From)
 * - TGT (Train Going To)
 * 
 * Based on decoded NVO signals, it updates system status strings and
 * corresponding OLED display messages for both directions.
 *
 * @param[in,out] None
 *
 * @return void
 *
 * @details
 * Requirement ID(s) -
 * PI_SSBPAC_SwRS_OPER_075
 *
 * @note
 * - Uses NVO data (pe[3].nvo_byte) to derive line states.
 * - Updates:
 *      - sys.line_state_TCF and sys.oled_TCF (TCF direction)
 *      - sys.line_state_TGT and sys.oled_TGT (TGT direction)
 *
 * - TCF (Down direction) states:
 *      - "LD" : Line Close
 *      - "OD" : Train On Line
 *      - "CD" : Line Clear
 *      - "TG" : Train Coming From
 *      - "ED" : Train Exit
 *      - "RD" : Line Error (default)
 *
 * - TGT (Up direction) states:
 *      - "LU" : Line Close
 *      - "OU" : Train On Line
 *      - "CU" : Line Clear
 *      - "TF" : Train Going To
 *      - "EU" : Train Exit
 *      - "RU" : Line Error (default)
 *
 * - Designed for dual-line operation where both directions are independently monitored.
 */

static void get_line_state_dl(void) {

	nvo_union_t* nvo_lcl = (nvo_union_t*)pe[3].nvo_byte;

	//TCF
	if(nvo_lcl->x.LC_DL) {
		strcpy(sys.line_state_TCF,"LD");
		strcpy(sys.oled_TCF,"TC: Line Close");
	}
	else if(nvo_lcl->x.TCF_R_TOL) {
		strcpy(sys.line_state_TCF,"OD");
		strcpy(sys.oled_TCF,"TC: On Line");
	}
	else if(nvo_lcl->x.LINE_STATUS_TCF == LINE_CLEAR) {
		strcpy(sys.line_state_TCF,"CD");
		strcpy(sys.oled_TCF,"TC: Line Clear");
	}
	else if(nvo_lcl->x.LINE_STATUS_TCF == TRAIN_MOVING) {
		strcpy(sys.line_state_TCF,"TG");
		strcpy(sys.oled_TCF,"TC: Coming From");
	}
	else if(nvo_lcl->x.LINE_STATUS_TCF == TRAIN_EXIT) {
		strcpy(sys.line_state_TCF,"ED");
		strcpy(sys.oled_TCF,"TC: Train Exit");
	}
	else  {
		strcpy(sys.line_state_TCF,"RD");
		strcpy(sys.oled_TCF,"TC: Line Error");
	}
	

	//TGT
	if(nvo_lcl->x.LC) {
		strcpy(sys.line_state_TGT,"LU");
		strcpy(sys.oled_TGT,"TG: Line Close");
	}
	else if(nvo_lcl->x.TGT_R_TOL) {
		strcpy(sys.line_state_TGT,"OU");
		strcpy(sys.oled_TGT,"TG: Train On Line");
	}
	else if(nvo_lcl->x.LINE_STATUS_TGT == LINE_CLEAR) {
		strcpy(sys.line_state_TGT,"CU");
		strcpy(sys.oled_TGT,"TG: Line Clear");
	}
	else if(nvo_lcl->x.LINE_STATUS_TGT == TRAIN_MOVING) {
		strcpy(sys.line_state_TGT,"TF");
		strcpy(sys.oled_TGT,"TG: Going To");
	}
	else if(nvo_lcl->x.LINE_STATUS_TGT == TRAIN_EXIT) {
		strcpy(sys.line_state_TGT,"EU");
		strcpy(sys.oled_TGT,"TG: Train Exit");
	}
	else  {
		strcpy(sys.line_state_TGT,"RU");
		strcpy(sys.oled_TGT,"TG: Line Error");
	}
}

/**
 * @brief Converts packed byte data into relay event structures.
 *
 * This function decodes packed relay data (2 bits per relay) from the input
 * byte array and updates the corresponding event structures. Each 2-bit field
 * represents relay state and its complementary status. It also detects state
 * changes and marks events as pending.
 *
 * @param [in]  nr_bytes Number of bytes in the input buffer.
 * @param [in]  p_byte Pointer to input byte array containing packed relay data.
 * @param [in,out] p_event Pointer to array of event structures to be updated.
 *
 * @return void
 *
 * @details
 * Requirement ID(s) -
 * PI_SSBPAC_SwRS_OPER_076
 *
 * @note
 * - Each byte contains 4 relays (2 bits per relay).
 * - Decoding of 2-bit values:
 *      0x1 → state = 0 (valid, de-energized)
 *      0x2 → state = 1 (valid, energized)
 *      0x0 or 0x3 → invalid (non-complimentary), error set
 * - Sets ERR_RELAY_NON_COMPLIMENTARY for invalid states.
 * - Detects state changes by comparing with last_state and sets ev_pending.
 * - Assumes p_event array is sized for (nr_bytes * 4) relay events.
 */

static void conv_byte2event(uint8_t nr_bytes, uint8_t* p_byte, event_t* p_event) {
	uint8_t i,d,v,k,j;
	event_t* s;

	for(i=0;i<nr_bytes;i++) {
		d = p_byte[i];  
		for(k=0U;k<4U;k++) {
			// get 2b relay status and its complimentary status for each relay
			v = d & 0x03U;  
			d >>= 2U;

			j = (i*4U)+k;
			s = &p_event[j];

			switch(v) {
				case 0x1:
					s->state = 0;
					s->err_code = ERR_RELAY_NONE;
					break;
				case 0x2:
					s->state = 1;
					s->err_code = ERR_RELAY_NONE;
					break;
				case 0x0:
					s->state = 3;
					s->err_code = ERR_RELAY_NON_COMPLIMENTARY;
					break;
				case 0x3:
					s->state = 3;
					s->err_code = ERR_RELAY_NON_COMPLIMENTARY;
					break;
				default:
					//Do nothing
					break;
			}

			if (s->state != s->last_state) {
				s->ev_pending = true;
				s->last_state = s->state;
			}
		}
	}
}

/**
 * @brief Converts packed byte data into relay event structures.
 *
 * This function decodes packed relay data (2 bits per relay) from the input
 * byte array and updates the corresponding event structures. Each 2-bit field
 * represents relay state and its complementary status. It also detects state
 * changes and marks events as pending.
 *
 * @param [in]  nr_bytes Number of bytes in the input buffer.
 * @param [in]  p_byte Pointer to input byte array containing packed relay data.
 * @param [in,out] p_event Pointer to array of event structures to be updated.
 *
 * @return void
 *
 * @details
 * Requirement ID(s) -
 * PI_SSBPAC_SwRS_OPER_076
 *
 * @note
 * - Each byte contains 4 relays (2 bits per relay).
 * - Decoding of 2-bit values:
 *      0x1 → state = 0 (valid, de-energized)
 *      0x2 → state = 1 (valid, energized)
 *      0x0 or 0x3 → invalid (non-complimentary), error set
 * - Sets ERR_RELAY_NON_COMPLIMENTARY for invalid states.
 * - Detects state changes by comparing with last_state and sets ev_pending.
 * - Assumes p_event array is sized for (nr_bytes * 4) relay events.
 */

static void conv_byte2event_misc(uint8_t nr_bytes, uint8_t* p_byte, event_t* p_event) {
	uint8_t i,d,v,k,j;
	event_t* s;

	for(i=0;i<nr_bytes;i++) {
		d = p_byte[i];  
		for(k=0U;k<4U;k++) {
			// get 2b relay status and its complimentary status for each relay
			v = d & 0x03U;  
			d >>= 2U;

			j = (i*4U)+k;
			s = &p_event[j];

			switch(v) {
				case 0x0:
					s->state = 0;
					s->err_code = ERR_RELAY_NONE;
					break;
				default:	
					s->state = 3;
					s->err_code = ERR_RELAY_NON_COMPLIMENTARY;
					break;
			}

			if (s->state != s->last_state) {
				s->ev_pending = true;
				s->last_state = s->state;
			}
		}
	}
}
/**
 * @brief Converts packed NVO byte data into event structures.
 *
 * This function decodes Non-Vital Output (NVO) data where each bit represents
 * the state of a single output. It updates the corresponding event structures
 * with the extracted state values.
 *
 * @param [in]  nr_bytes Number of bytes in the input buffer.
 * @param [in]  p_byte Pointer to input byte array containing packed NVO data.
 * @param [in,out] p_event Pointer to array of event structures to be updated.
 *
 * @return void
 *
 * @details
 * Requirement ID(s) -
 * PI_SSBPAC_SwRS_OPER_077
 *
 * @note
 * - Each byte contains 8 NVO states (1 bit per output).
 * - Extracts each bit and assigns directly to event state.
 * - Does not perform error checking or event change detection.
 * - Assumes p_event array is sized for (nr_bytes * 8) events.
 */

static void conv_byte2event_nvo(uint8_t nr_bytes, uint8_t* p_byte, event_t* p_event) {
	uint8_t i,d,v,k,j;
	event_t* s;

	for(i=0;i<nr_bytes;i++) {
		d = p_byte[i];  
		for(k=0U;k<8U;k++) {
			v = d & 0x01U;  
			d >>= 1;

			j = (i*8U)+k;
			s = &p_event[j];
			s->state = v;
		}
	}
}
