/***************************************************************************//**
 * @file     interlock.c
 * @brief    All application specific source files
 * @version  1.0.0
 * @details
 * Compiler : Keil uVision
 * Target : Nuvoton M263
 * Module : Central Controller Card
 * Date Created : Jan 16, 2026
 * @copyright Copyright (C) 2026 Anaya Tech Systems Pvt. Ltd. . All rights reserved.
 * @author    DG 
 * @par Functions Included
 * void interlock(void); <br>
 * void interlock_set_defaults(void);
 ******************************************************************************/
 
#include "NuMicro.h"
#include "interlock.h"
#include "app.h"

uint8_t TGTR_1, TGTR_2, TGTR_3;	
uint8_t TCFR_1, TCFR_2, TCFR_3;	
uint8_t ASCR_1, ASCR_2;	

vi_union_t vi_u;
vi_union_t vi_oe_u; //remote side vital inputs
nvo_union_t nvo_u;

lo_union_t lo_u; 
lo_union_t lo_oe_u; 
lo_struct_t lo_in_s;

vo_union_t vo_u;
vo_union_t vo_oe_u;
vo_union_t vo_in_u;

extern uint16_t g_SSBPAC_FAIL_ticks;

/**
* @brief based on the interlock mode (single line, double line and mux) call
* all releted function like interlock, interlock_tgt_sm, interlock_tcf_sm, interlock_buzzer_sm_tcf and interlock_buzzer_sm_tgt 
* if mode is not selected than call single line related function
* 
* @param [in] ctrls_u.x.interlock_mode
* Determines which interlock logic mode is executed:
* * INTERLOCK_MODE_DOUBLE_LINE
* * INTERLOCK_MODE_SINGLE_LINE
* * INTERLOCK_MODE_MUX
*
* @param [in] TGTR_1, TGTR_2, TGTR_3
* Track circuit trigger relay inputs used to determine TGTR output state.
*
* @param [in] TCFR_1, TCFR_2, TCFR_3
* Track circuit failure relay inputs used to determine TCFR output state.
*
* @param [in] ASCR_1, ASCR_2
* Auxiliary signal condition relay inputs used to determine ASCR output state.
*
* @param [in] ctrls_u.x.com0_state
* Communication channel 0 health status used to update COM0 status output.
*
* @param [in] ctrls_u.x.com1_state
* Communication channel 1 health status used to update COM1 status output.
*
* @param [in] ctrls_u.x.SSBPAC_OK
* Indicates that the SSBPAC subsystem is operating normally.
*
* @param [in] ctrls_u.x.SSBPAC_FAIL
* Indicates that the SSBPAC subsystem has reported a failure condition.
*
* @param [out] vo_u.x
* Structure containing relay output states including TGTR, TCFR, and ASCR.
*
* @param [out] nvo_u.x
* Structure containing non-vital output signals including communication status and SSBPAC status indicators.
*
* @return
* 		None.
*
* @details
* Requirement ID(s) -
* PI_SSBPAC_SwRS_SFTY_002
* PI_SSBPAC_SwRS_SDESG_003
*
* @note
* 		Output relays are forced to their default safe state when an SSBPAC fault condition is detected.
* 		The function uses global timing counters (e.g., g_120s_ticks, g_SSBPAC_FAIL_ticks) to control delayed state transitions.
* 		Communication status outputs are inverted before being written to the non-vital output structure.
* 		The function assumes that relay input macros (PICK/DROP) and timing counters are updated externally before execution.
* 
*/

void interlock(void) {	
		vo_struct_t* vo = &vo_u.x;
		nvo_struct_t* nvo = &nvo_u.x;

		if(ctrls_u.x.interlock_mode == INTERLOCK_MODE_DOUBLE_LINE) {
				interlock_DL();
				interlock_TGT_SM_DL();
				interlock_TCF_SM_DL();
				interlock_Buzzer_SM_TCF_DL();
				interlock_Buzzer_SM_TGT_DL();
		}
		else if(ctrls_u.x.interlock_mode == INTERLOCK_MODE_SINGLE_LINE) {
				interlock_SL();
				interlock_TGT_SM();
				interlock_TCF_SM();
				interlock_Buzzer_SM();
		}
		else if(ctrls_u.x.interlock_mode == INTERLOCK_MODE_MUX) {
				interlock_Bypass();
		}
		else {
			//do nothing
		}

		if(TGTR_1 == PICK) {
				vo_u.x.TGTR = PICK;
				vo_u.x.TGTR_D = DROP;
		}	
		else if(((TGTR_2 == PICK) || (TGTR_3 == PICK)) && (g_120s_ticks == 0U)) {
				vo_u.x.TGTR = DROP;
				vo_u.x.TGTR_D = PICK;
		}
		else {
				//do nothing
		}

		if(TCFR_1 == PICK) {
				vo_u.x.TCFR = PICK;
				vo_u.x.TCFR_D = DROP;
		}	
		else if(((TCFR_2 == PICK) || (TCFR_3 == PICK)) && (g_120s_ticks == 0U)) {
				vo_u.x.TCFR = DROP;
				vo_u.x.TCFR_D = PICK;
		}
		else {
				//do nothing
		}

		if((ASCR_1 == PICK) || (ASCR_2 == PICK)) {
				vo_u.x.ASCR = PICK;
		}
		else {
				vo_u.x.ASCR = DROP;
		}

		nvo->COM0_STA = (ctrls_u.x.com0_state == 1U) ? 0 : 1;
		nvo->COM1_STA = (ctrls_u.x.com1_state == 1U) ? 0 : 1;

				if(ctrls_u.x.reset_state == 3) {
					nvo->RESET_CNTR = 1;
				}
				else {
					nvo->RESET_CNTR = 0;			
				}
		
		// Check Fault - Set output relays to default state
		if(((ctrls_u.x.SSBPAC_OK == 0U) && (ctrls_u.x.SSBPAC_FAIL == 1U))||(ctrls_u.x.reset_state == 3)) {
				vo->TGTR = DROP;
				vo->TGTR_D = PICK;
				vo->TCFR = DROP;
				vo->TCFR_D = PICK;
				vo->ASCR = DROP;
				
				nvo->CAN_CNTR = 0;
				nvo->TGT_R_TOL = 1;
				nvo->TGT_RA = 0;
				nvo->TGT_G = 0;
				nvo->TGT_GA = 0;
				nvo->TCF_R_TOL = 1;
				nvo->TCF_RA = 0;
				nvo->TCF_G = 0;
				nvo->TCF_GA = 0;
				nvo->CANCEL = 0;
				nvo->CAN_COOP = 0;
				nvo->SMKEY = 0;
				nvo->SNOEK = 0;
				nvo->SNK = 0;
				nvo->LC = 0;
				nvo->ACKN_R = 0;
				nvo->COM0_STA = 0;
				nvo->SHK_G = 0;
				nvo->SHK_R = 0;
				nvo->ACKN_D = 0;
				nvo->SNK_R = 0;
				nvo->COM1_STA = 0;
//				nvo->RESET_CNTR = 0;
				nvo->LSS_R = 0;
				nvo->LSS_G = 0;
				nvo->LINE_F = 0;
				nvo->LINE_O = 0;
				nvo->BELL = 0;
				nvo->SECTIONAL_BELL = 0;
				nvo->LC_DL = 0;
				nvo->LINE_TCF_F = 0;
				nvo->LINE_TCF_O = 0;
				nvo->LCB_KEY = 0;
				nvo->SECTIONAL_BELL_DL = 0;
				nvo->LINE_STATUS_TCF = 0;
				nvo->LINE_STATUS_TGT = 0;
				nvo->SSBPAC_FAIL = 1;
				nvo->SSBPAC_OK = 0;
				g_SSBPAC_FAIL_ticks = 2000;
		}
		else if((ctrls_u.x.SSBPAC_OK == 1U) && (ctrls_u.x.SSBPAC_FAIL == 1U)) {
				nvo->SSBPAC_OK = 1;

				if(g_SSBPAC_FAIL_ticks == 0U) {
					g_SSBPAC_FAIL_ticks = 2000;
					nvo->SSBPAC_FAIL = 0;
				}
				else if(g_SSBPAC_FAIL_ticks < 1000U) {
					nvo->SSBPAC_FAIL = 1;
				}
		}
		else if((ctrls_u.x.SSBPAC_OK == 1U) && (ctrls_u.x.SSBPAC_FAIL == 0U)) {
				g_SSBPAC_FAIL_ticks = 2000;
				nvo->SSBPAC_OK = 1;
				nvo->SSBPAC_FAIL = 0;
		}
		else {
				g_SSBPAC_FAIL_ticks = 2000;
				nvo->SSBPAC_OK = 0;
				nvo->SSBPAC_FAIL = 1;
		}
}

/**
* @brief
* Initializes all interlocking system outputs to their default states during system startup or reset.
*
* This function sets default values for Vital Outputs (VO), Non-Vital Outputs (NVO), and Logical Outputs (LO). It ensures the system starts in a known safe state. Additionally, several NVO indicators (LEDs) are turned ON during power-up for diagnostic and visual verification purposes.
*
* @param [out] vo_u.x
* 	Vital Output structure containing relay output states such as TGTR, TCFR, and ASCR. These outputs are initialized to their safe default relay states.
*
* @param [out] nvo_u.x
* 	Non-Vital Output structure controlling LEDs, indicators, counters, and status outputs. Many indicators are enabled during power-up to verify proper hardware operation.
*
* @param [out] lo_u.x
* 	Logical Output structure containing internal logical control signals used by the interlocking system logic. All logical outputs are initialized to an inactive state.
*
* @return
* None.
*
* @details
* Requirement ID(s) -
* PI_SSBPAC_SwRS_OPER_061
*
* @note
* 		This function is typically called during system initialization or power-up reset.
* 		Vital relay outputs are forced to their default safe states to ensure fail-safe operation.
* 		Many NVO LED indicators are intentionally turned ON during startup to perform a lamp test.
* 		Logical outputs are cleared to prevent unintended control actions before the main interlocking logic begins execution.
*/

void interlock_set_defaults(void) {
	vo_struct_t* vo = &vo_u.x;
	nvo_struct_t* nvo = &nvo_u.x;
	lo_struct_t* lo = &lo_u.x;
	// VO defaults
	vo->TGTR = DROP;
	vo->TGTR_D = PICK;
	vo->TCFR = DROP;
	vo->TCFR_D = PICK;
	vo->ASCR = DROP;

	//NVO leds switched ON for test at powerup
	nvo->CAN_CNTR = 0;
	nvo->TGT_R_TOL = 1;
	nvo->TGT_RA = 1;
	nvo->TGT_G = 1;
	nvo->TGT_GA = 1;
	nvo->TCF_R_TOL = 1;
	nvo->TCF_RA = 1;
	nvo->TCF_G = 1;
	nvo->TCF_GA = 1;
	nvo->CANCEL = 0;
	nvo->CAN_COOP = 0;
	nvo->SMKEY = 1;
	nvo->SNOEK = 1;
	nvo->SNK = 1;
	nvo->LC = 1;
	nvo->ACKN_R = 1;
	nvo->COM0_STA = 1;
	nvo->SHK_G = 1;
	nvo->SHK_R = 1;
	nvo->ACKN_D = 1;
	nvo->SNK_R = 1;
	nvo->COM1_STA = 1;
	nvo->RESET_CNTR = 0;
	nvo->SSBPAC_OK = 1;
	nvo->SSBPAC_FAIL = 1;
	nvo->LSS_R = 1;
	nvo->LSS_G = 1;
	nvo->LINE_F = 1;
	nvo->LINE_O = 1;
	nvo->BELL = 0;
	nvo->SECTIONAL_BELL = 1;
	nvo->LC_DL = 1;
	nvo->LINE_TCF_F = 1;
	nvo->LINE_TCF_O = 1;
	nvo->LCB_KEY = 1;
	nvo->SECTIONAL_BELL_DL = 1;
	nvo->LINE_STATUS_TCF = 1;
	nvo->LINE_STATUS_TGT = 1;

	// Logical output defaults
	lo->TGTXR = 0;
	lo->TGTYR = 0;
	lo->TCFRY = 0;
	lo->TGTZR = 0;
	lo->TCFXR = 0;
	lo->ASGNCPR = 0;
	lo->TGTYP = 0;
	lo->UNR = 0;
	lo->EGNR = 0;
	lo->TCFRY_1 = 0;
	lo->CAR = 0;
	lo->BTSR = 0;
	lo->FR1 = 0;
	lo->FR2 = 0;
	lo->TAR2 = 0;
	lo->TAR1 = 0;
	lo->EJ120 = 0;
	lo->JPR120 = 0;
	lo->TCFZR = 0;
	lo->TGTPR = 0;
	lo->TCFCR = 0;
	lo->SPARE = 0;
}
