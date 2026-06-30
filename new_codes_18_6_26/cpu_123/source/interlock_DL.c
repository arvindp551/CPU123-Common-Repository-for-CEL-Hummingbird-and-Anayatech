/***************************************************************************//**
 * @file     interlock_DL.c
 * @brief    All application specific source files
 * @version  1.0.0
 * @details
 * Compiler : Keil uVision
 * Target : Nuvoton M263
 * Module : Central Controller Card
 * Date Created : Jan 02, 2026
 * @copyright Copyright (C) 2026 Anaya Tech Systems Pvt. Ltd. . All rights reserved.
 * @author    DG 
 * @par Functions Included
 * void interlock_DL(void); <br>
 * void interlock_TGT_SM_DL(void); <br>
 * void interlock_TCF_SM_DL(void); <br>
 * void interlock_Buzzer_SM_TGT_DL(void); <br>
 * void interlock_Buzzer_SM_TCF_DL(void);
 ******************************************************************************/
 
#include "NuMicro.h"
#include "stdbool.h"
#include "interlock.h"
#include "app.h"

#define TRAIN_ERR 0
#define LINE_CLEAR 1
#define TRAIN_MOVING 2
#define TRAIN_EXIT 3 

static uint8_t g_tcf_120jpr_state = 0;

/**
* @brief
* 		Executes Double Line (DL) interlocking logic using a state-machine approach.
*
* 		This function implements the interlocking logic for Double Line mode.
* 		It operates in multiple states to initialize default values, copy previous states, and compute new logical, vital output (VO), and non-vital output (NVO) signals based on local and remote inputs.
*
* @param [in] vi_u.dl
* Structure containing local-side vital input signals used for interlocking decisions.
*
* @param [in] vi_oe_u.dl
* Structure containing remote-side vital input signals from the other end.
*
* @param [in,out] lo_u.x
* Logical output structure updated based on interlocking conditions.
*
* @param [in,out] vo_u.x
* Vital output structure updated with relay control signals such as TGTR, TCFR, and ASCR.
*
* @param [out] nvo_u.x
* Non-vital output structure used for status indication, LEDs, and counters.
*
* @param [in] lo_oe_u.x
* Logical signals received from the remote end used in decision making.
*
* @param [in] vo_oe_u.x
* Vital output signals from the remote end used for interlocking coordination.
*
* @param [in] g_120s_ticks
* Global timing counter used for generating timed signals (e.g., JPR120).
*
* @param [in,out] g_tcf_120jpr_state
* Global state variable used for managing timing-related transitions.
*
* @return
* 		None.
*
* @details
* Requirement ID(s) -
* PI_SSBPAC_SwRS_OPER_062
* PI_SSBPAC_SwRS_SFTY_001
*
* @note
* The function operates as a cyclic state machine with three main states:
* 		0: Initialize default logical and output states.
* 		1: Copy current states for feedback/reference.
* 		2: Compute interlocking logic and update outputs.
* Logical decisions are based on combinations of vital inputs, feedback signals, and remote-side states.
* Timing-dependent logic is implemented using global tick counters.
* The function must be called periodically to maintain correct interlocking behavior.
* Care must be taken to ensure all global/shared variables are properly synchronized, especially in interrupt-driven systems.
* This logic follows version 1.4 update (dated 27-12-2024).
*/

void interlock_DL(void) {
	static uint8_t lstate_DL = 0;
	uint8_t i;
	bool x;

	// Local side states. vi:VITAL Input, lo:Logical signals, vo:VITAL output
	vo_struct_t* vo = &vo_u.x;
	vo_struct_t* vo_in = &vo_in_u.x;
	nvo_struct_t* nvo = &nvo_u.x;
	lo_struct_t* lo_in = &lo_in_s;
	lo_struct_t* lo = &lo_u.x; 
	vi_dl_struct_t* vi = &vi_u.dl;

	// Remote or other end states. vi:VITAL Input, lo:Logical signals, vo:VITAL output
	vi_dl_struct_t* vi_oe = &vi_oe_u.dl;
	lo_struct_t* lo_oe = &lo_oe_u.x;
	vo_struct_t* vo_oe = &vo_oe_u.x;

	switch(lstate_DL) {
		case 0: //set default states
			lo->TGTXR = DROP;
			lo->TGTYR = DROP;
			lo->TCFRY = DROP;
			lo->TGTZR = DROP;
			lo->TCFXR = DROP;
			lo->ASGNCPR = DROP;
			lo->TGTYP = DROP;
			lo->UNR = DROP;
			lo->EGNR = DROP;
			lo->TCFRY_1 = DROP;
			lo->CAR = DROP;
			lo->BTSR = PICK;
			lo->FR1 = DROP;
			lo->FR2 = DROP;
			lo->TAR2 = PICK;
			lo->TAR1 = PICK;
			lo->EJ120 = DROP;
			lo->JPR120 = DROP;

			vo->TGTR = DROP;
			vo->TGTR_D = PICK;
			vo->TCFR = DROP;
			vo->TCFR_D = PICK;
			vo->ASCR = DROP;

			lstate_DL = 1;
			break;
		case 1: // copy states
			lo_in->TGTXR = lo->TGTXR;
			lo_in->TGTYR = lo->TGTYR;
			lo_in->TCFRY = lo->TCFRY;
			lo_in->TGTZR = lo->TGTZR;
			lo_in->TCFXR = lo->TCFXR;
			lo_in->ASGNCPR = lo->ASGNCPR;
			lo_in->TGTYP = lo->TGTYP;
			lo_in->UNR = lo->UNR;
			lo_in->EGNR = lo->EGNR;
			lo_in->TCFRY_1 = lo->TCFRY_1;
			lo_in->CAR = lo->CAR;
			lo_in->BTSR = lo->BTSR;
			lo_in->FR1 = lo->FR1;
			lo_in->FR2 = lo->FR2;
			lo_in->TAR2 = lo->TAR2;
			lo_in->TAR1 = lo->TAR1;
			lo_in->EJ120 = lo->EJ120;
			lo_in->JPR120 = lo->JPR120;

			lo_in->TCFZR = lo->TCFZR;
			lo_in->TGTPR = lo->TGTPR;
			lo_in->TCFCR = lo->TCFCR;

			vo_in->TGTR = vo->TGTR;
			vo_in->TCFR = vo->TCFR;
			vo_in->ASCR = vo->ASCR;

			lstate_DL = 2;
			break;
		case 2: // compute logical, vo and nvo states // update as per Version 1.4 dated 27-12-2024
			TGTR_1 = ((vi->AZTR_D==PICK) && (vi->ASGNCR==PICK) && (vi->TGTN==PICK) && (vi->BPN==PICK) && (lo_in->TGTYR==PICK) && (lo_in->TGTXR==PICK) && (vi->SM_KEY==PICK))?PICK:DROP;
			TGTR_2 = ((vi->AZTR_D==PICK) && (vi->ASGNCR==PICK)  && (vi->TGTN==DROP) && (vi->BPN==DROP) &&  (lo_in->TGTZR==DROP) && (lo_in->TGTYR==DROP))?PICK:DROP;
			TGTR_3 = 	DROP;
			// Replace AZTR_D to AZTR_R
			TCFR_1 = ((vi->AZTR_R==PICK) && (vi->HSGNCR==PICK) && (vi->HSBTPR==PICK) && (vi->BPN==DROP) && (vi->CNR==DROP) &&  (lo_in->TCFXR==PICK) && (lo_in->BTSR==PICK) && (lo_in->CAR==DROP) && (lo_in->ASGNCPR==PICK) && (lo_in->JPR120==DROP) && (lo_in->TAR1==DROP) && (lo_in->TAR2==DROP) && (vi->LCB_KEY==PICK))?PICK:DROP;
			TCFR_2 = ((vi->AZTR_R==PICK) && (vi->HSGNCR==PICK)  &&  (vi->CNR==DROP) && (lo_in->TCFXR==DROP) && (lo_in->CAR==PICK) && (lo_in->ASGNCPR==PICK) && (lo_in->JPR120==PICK))?PICK:DROP;
			TCFR_3 = ((vi->AZTR_R==PICK) && (vi->HSGNCR==PICK)  && (vi->CNR==DROP) &&  (lo_in->TCFXR==DROP) && (lo_in->CAR==DROP) && (lo_in->ASGNCPR==PICK) && (lo_in->TAR2==PICK))?PICK:DROP;

			ASCR_1  = ((vi->AZTR_D==PICK) && (vi->TGTR_FDBK==PICK) && (vo_in->TGTR==PICK) && (lo_in->TGTYR==PICK))?PICK:DROP;
			ASCR_2  = ((vi->AZTR_D==PICK) && (vi->TGTR_FDBK==PICK) && (vi->ASCR ==PICK) && (vo_in->TGTR==PICK))?PICK:DROP;

			lo->TGTXR = ((vi->AZTR_D==PICK) && (vi->ASGNCR==PICK) && (vi->TGTN==PICK) && (vi->BPN==PICK) && (vo_in->TGTR==DROP) && (vi->SM_KEY==PICK))?PICK:DROP;
			lo->TGTYR = ((vo_oe->TCFR==PICK) && (lo_oe->BTSR==PICK) && (vi->SM_KEY==PICK))?PICK:DROP;
			lo->TGTZR = ((vo_oe->TCFR==PICK) && (vi_oe->HSGNCR==PICK))?PICK:DROP;
			lo->TCFXR = ((lo_oe->TGTXR==PICK))?PICK:DROP;
			lo->ASGNCPR = ((vi_oe->ASGNCR==PICK))?PICK:DROP;
			x = ((vi->AZTR_R==PICK) && (vi->HSGNCR==PICK) && (vi->BPN==PICK) && (vi->CNR==PICK) &&  (lo_in->ASGNCPR==PICK) && (lo_in->TCFCR==PICK) && (vo_in->TCFR==PICK) && (vi->SM_KEY==PICK));
			x = x || ((vi->AZTR_R==PICK) && (lo_in->CAR==PICK) && (vo_in->TCFR==PICK));
			lo->CAR = x?PICK:DROP;
			x = ((vi->AZTR_R==PICK) && (lo_in->CAR==DROP) && (vo_in->TCFR==DROP));
			x = x || ((vi->AZTR_R==PICK) && (lo_in->BTSR==PICK) && (lo_in->CAR==DROP));
			lo->BTSR = x?PICK:DROP;
			x = ((vi->AZTR_R==PICK) && (lo_in->BTSR==DROP) && (lo_in->FR2==DROP) && (vo_in->TCFR==PICK));
			x = x || ((vi->AZTR_D==PICK) && (lo_in->TGTYR==DROP) && (lo_in->FR2==DROP) && (vo_in->TGTR==PICK));
			lo->FR1 = x?PICK:DROP;
			x = ((vi->AZTR_R==PICK) && (lo_in->BTSR==DROP) && (lo_in->FR1==PICK) && (vo_in->TCFR==PICK));
			x = x || ((vi->AZTR_D==PICK) && (lo_in->TGTYR==DROP) && (lo_in->FR1==PICK) && (vo_in->TGTR==PICK));
			lo->FR2 = x?PICK:DROP;
			x = ((vi->HSATPR==PICK) && (vi->HSBTPR==DROP) && (lo_in->BTSR==DROP) && (lo_in->TAR1==PICK));
			x = x || ((lo_in->BTSR==DROP) && (lo_in->TAR2==PICK));
			lo->TAR2 = x?PICK:DROP;
			x = ((vi->HSBTPR==PICK) && (lo_in->TAR2==DROP) && (vo_in->TCFR==PICK));
			x = x || ((vi->HSATPR==DROP) && (lo_in->TAR1==PICK));
			lo->TAR1 = (x)?PICK:DROP;

			lo->JPR120 = ((g_120s_ticks <= 10U) && (g_120s_ticks > 0U))?PICK:DROP;
			if(g_tcf_120jpr_state == 2U) {g_tcf_120jpr_state = 0;}
			else {g_tcf_120jpr_state = g_tcf_120jpr_state;}
			
			lo->TCFZR = ((vo_in->TCFR==DROP))?PICK:DROP;
			lo->TCFCR = ((vi_oe->ASGNCR==PICK))?PICK:DROP;
			lo->TGTPR = ((vo_in->TGTR==DROP))?PICK:DROP;

			nvo->CAN_CNTR = (lo_in->CAR==PICK)?1:0;
			nvo->TGT_R_TOL = ((vi->AZTR_D==DROP) && (vo_in->TGTR==PICK))?1:0;
			nvo->TGT_RA = ((vi->AZTR_D==DROP) && (vo_in->TGTR==PICK))?1:0;
			nvo->TCF_R_TOL = ((vi->AZTR_R==DROP) && (vo_in->TCFR==PICK))?1:0;
			nvo->TCF_RA = ((vi->AZTR_R==DROP) && (vo_in->TCFR==PICK))?1:0;

			nvo->SMKEY = (vi->SM_KEY==PICK)?1:0;
			nvo->SNOEK = (vo_oe->ASCR==DROP)?1:0;
			nvo->SNK = (vi->ASCR ==DROP)?1:0;
			nvo->LC = ((vi->AZTR_D==PICK) && (vi->ASGNCR==PICK) && (vi_oe->HSGNCR==PICK) && (vi_oe->HSATPR==PICK) && (vi_oe->HSBTPR==PICK) && (vi->ASCR ==DROP) && (vo_oe->TCFR==DROP) && (vo_in->TGTR==DROP))?1:0;
			nvo->SHK_G = ((vi->EKT_KEY==PICK) && (vi->SM_KEY==DROP))?1:0;
			nvo->SHK_R = (vi->SM_KEY==PICK)?1:0;
			//nvo->RESET_CNTR = ((vi->BPN==PICK) && (vi->RST_BTN==PICK))?1:0;
			//nvo->RESET_CNTR = (vi->RST_BTN==PICK)?1:0;
			nvo->LSS_R = ((vi->ASCR ==DROP)?1:0);
			nvo->LSS_G = ((vi->ASCR ==PICK)?1:0);
			nvo->LINE_F = ((vi->AZTR_D==PICK)?1:0);
			nvo->LINE_O = ((vi->AZTR_D==DROP)?1:0);
			nvo->BELL = ((vi_oe->BPN==PICK) && (vi_oe->SM_KEY==PICK))?1:0;
			nvo->LC_DL = ((vi->AZTR_R==PICK) && (vi_oe->ASGNCR==PICK) && (vi->HSGNCR==PICK) && (vi->HSATPR==PICK) && (vi->HSBTPR==PICK) && (vi_oe->ASCR ==DROP) && (vo_in->TCFR==DROP) && (vo_oe->TGTR==DROP))?1:0;
			
			nvo->LINE_TCF_F = (vi->AZTR_R == PICK)?1:0;
			nvo->LINE_TCF_O = (vi->AZTR_R == DROP)?1:0;
			
			nvo->SNK_R = ((vi->HSGNCR == PICK) && (vi_oe->ASCR == DROP))?1:0;
			nvo->LCB_KEY = (vi->LCB_KEY == 1U )? 0 : 1; 
			lstate_DL = 1;
			break;
		default: {
			//shouldn't be here
			break;
		}
	}
}


/**
 * @brief Interlock State Machine for TGF
 * This function implements the interlocking logic for the TGF section.
 * It manages the state transitions based on the inputs and controls TGF_G and TGF_GA outputs accordingly.
 * The state machine is designed to PICK, DROP or Pulsate TGF_G(A) on train movements, line closures.
 *
 * @param [in] vo_in_u.x
 * Vital input structure containing feedback states of TGTR used for state transitions.
 *
 * @param [in] vi_u.dl
 * Local-side vital input signals used to determine track conditions
 *
 * @param [in] vi_oe_u.dl
 * Remote-side vital input signals used for cooperative cancellation and interlocking coordination.
 *
 * @param [in,out] nvo_u.x
 * Non-vital output structure used to control LED indicators, status flags, and display encoding (e.g., LINE_STATUS_TGT).
 *
 * @param [in] g_120s_ticks
 * Global timer used for 120-second blinking and delay operations.
 *
 * @param [in,out] g_TGT_ticks
 * Timer used for generating 1-second blinking intervals for TGT indicators.
 *
 * @return
 * None.
 *
 * @details
 * Requirement ID(s) -
 * PI_SSBPAC_SwRS_OPER_066
 * PI_SSBPAC_SwRS_OPER_067
 * PI_SSBPAC_SwRS_OPER_068
 * PI_SSBPAC_SwRS_OPER_069
 *
 * @note
 * TGT_G and TGT_GA LEDs are toggled periodically to indicate different operational states such as blinking during initialization and train movement.
 * The state machine is bypassed if SHKR transition is detected (shkr_state != 0).
 *
 */

void interlock_TGT_SM_DL(void) {
	static uint8_t lstate_TGT_SM_DL = 0;
	static uint8_t shkr_state = 0;

	vo_struct_t* vo = &vo_u.x;
	vo_struct_t* vo_in = &vo_in_u.x;
	nvo_struct_t* nvo = &nvo_u.x;
	lo_struct_t* lo_in = &lo_in_s;
	lo_struct_t* lo = &lo_u.x; 

	vi_dl_struct_t* vi = &vi_u.dl;
	vi_dl_struct_t* vi_oe = &vi_oe_u.dl; //remote side vital inputs

	//If SHKR transitions TGTR should be ignored so TGTR SM state is set to 0	
	// break commented intentionally to avoid confusion with the switch case structure
	// maintained switch-case for easy expansion in future
	if(shkr_state == 0U) {
		switch (lstate_TGT_SM_DL)
		{
			case 0:
				// Let TGT_G and TGT_GA blink for 120 seconds
				if(g_120s_ticks != 0U) {
					if(g_TGT_ticks == 0U) { // one second elapsed
						nvo->TGT_G  = (nvo->TGT_G != 0U) ? 0 : 1; // Toggle TGT_G
						nvo->TGT_GA = (nvo->TGT_GA != 0U) ? 0 : 1; // Toggle TGT_GA
						g_TGT_ticks = 1000; // set for one second
					}
				}
				else if(vo_in->TGTR == DROP) { // Line Close
					nvo->TGT_G = 0;
					nvo->TGT_GA = 0;
					lstate_TGT_SM_DL = 1;
				}
				else {
					//do nothing
				}
				break; 

			case 1:
				if((vo_in->TGTR == PICK) && (vi->AZTR_D == PICK)) { // Clear For TGT
					nvo->TGT_G = 1;
					nvo->TGT_GA = 1;
					
					//Line clear encoding for CPU0 OLED status
					nvo->LINE_STATUS_TGT = LINE_CLEAR;

					lstate_TGT_SM_DL = 2;
				}
				break;

			case 2:
				if((vo_in->TGTR == PICK) && (vi->AZTR_D == DROP)) { // Train Enter at section
					nvo->TGT_G = 0;
					nvo->TGT_GA = 0;


					lstate_TGT_SM_DL = 3;
				}
				else { // Cancellation when line clear and train awaited
					if((vi_oe->CNR == PICK) && (vi_oe->BPN == PICK)) {
						nvo->CAN_COOP = 1;
					}		
					if((vi->CNL_COOP == PICK) && (nvo->CAN_COOP == 1U)) {  			
						g_120s_ticks = 120;
						nvo->CAN_COOP = 0;
						lstate_TGT_SM_DL = 0;
					}
				}
				break;

			case 3:
				if((vo_in->TGTR == PICK) && (vi->AZTR_D == PICK)) { // Tain on line
					//Train Online  encoding for CPU0 OLED status
					// check TOL. If set ignore SPARE8 SPARE9

					if(g_TGT_ticks == 0U) { // one second elapsed. Toggle TGT_G and GA while train online
						nvo->TGT_G  = (nvo->TGT_G != 0U) ? 0 : 1; // Toggle TGT_G
						nvo->TGT_GA = (nvo->TGT_GA != 0U) ? 0 : 1; // Toggle TGT_GA
						g_TGT_ticks = 1000; // set for one second
					}
					if((vi_oe->CNR == PICK) && (vi_oe->BPN == PICK)) { // cancellation when train on line
						nvo->CAN_COOP = 1;
					}		
					if((vi->CNL_COOP == PICK) && (nvo->CAN_COOP == 1U)) {  			
						g_120s_ticks = 120;
						nvo->CAN_COOP = 0;
						lstate_TGT_SM_DL = 0;
					}
					//Train Going To encoding for CPU0 OLED status
					nvo->LINE_STATUS_TGT = TRAIN_MOVING;
				}
				else if((vo_in->TGTR == DROP) && (vi->AZTR_D == PICK)) { // Train Exit from Sending station - Line Close
					lstate_TGT_SM_DL = 4;
				}
				else {
					//do nothing
				}
				break;

			case 4:
				if((vo_in->TGTR == DROP) && (vi->AZTR_D == PICK)) { // Train Exit from Sending station - Line Close
					nvo->TGT_G = 0;
					nvo->TGT_GA = 0;
					nvo->CAN_COOP = 0;
					lstate_TGT_SM_DL = 0; // Reset to initial state
							   
					//Train Exit encoding for CPU0 OLED status
					nvo->LINE_STATUS_TGT = TRAIN_EXIT;
				}
				else {
					//do nothing
				}
				break;

			default:
				//do nothing
				break;
		}
	}
}

/**
 * @brief Interlock State Machine for TCF
 * This function implements the interlocking logic for the TCF section.
 * It manages the state transitions based on the inputs and controls TCF_G and TCF_GA outputs accordingly.
 * The state machine is designed to PICK, DROP or Pulsate TCF_G(A) on train movements, line closures.
 *
 * @param [in] vo_in_u.x
 * Vital input structure containing TCFR feedback used for determining state transitions.
 *
 * @param [in] vi_u.dl
 * Local-side vital input signals (e.g., AZTR_R, cancellation inputs) used for interlocking decisions.
 *
 * @param [in] vi_oe_u.dl
 * Remote-side vital input signals used for cooperative cancellation handling and synchronization.
 *
 * @param [in,out] nvo_u.x
 * Non-vital output structure used to control LED indicators, cancellation signals, and status outputs (e.g., LINE_STATUS_TCF).
 *
 * @param [in] g_120s_ticks
 * Global timer used for 120-second blinking and delay operations.
 *
 * @param [in,out] g_TCF_ticks
 * Timer used to generate periodic (1-second) blinking for TCF indicators.
 *
 * @return
 * None.
 *
 * @details
 * Requirement ID(s) - ?
 * PI_SSBPAC_SwRS_OPER_066
 * PI_SSBPAC_SwRS_OPER_067
 * PI_SSBPAC_SwRS_OPER_068
 * PI_SSBPAC_SwRS_OPER_069
 * @note
 * The function relies on global timing counters and must be executed periodically for correct operation.
 */

void interlock_TCF_SM_DL(void) {
	static uint8_t cancel_req = 0;
	static uint8_t lstate_TCF_SM_DL = 0;
	vo_struct_t* vo = &vo_u.x;
	vo_struct_t* vo_in = &vo_in_u.x;
	nvo_struct_t* nvo = &nvo_u.x;
	lo_struct_t* lo_in = &lo_in_s;
	lo_struct_t* lo = &lo_u.x; 

	vi_dl_struct_t* vi = &vi_u.dl;
	vi_dl_struct_t* vi_oe = &vi_oe_u.dl; //remote side vital inputs

	// break commented intentionally to avoid confusion with the switch case structure
	// maintained switch-case for easy expansion in future
	switch (lstate_TCF_SM_DL)
	{
		case 0:
			if(g_120s_ticks != 0U) {
				if(g_TCF_ticks == 0U) { // one second elapsed
					nvo->TCF_G  = (nvo->TCF_G != 0U) ? 0 : 1; // Toggle TCF_G
					nvo->TCF_GA = (nvo->TCF_GA != 0U) ? 0 : 1; // Toggle TCF_GA
					nvo->CANCEL = (nvo->CANCEL != 0U) ? 0 : 1; // Toggle CANCEL LED
					g_TCF_ticks = 1000; // set for one second
				}
			}
			else if(vo_in->TCFR == DROP) { // Line Close
				nvo->TCF_G = 0;
				nvo->TCF_GA = 0;
				nvo->CANCEL = 0;
				lstate_TCF_SM_DL = 1;
			}
			else {
				// do nothing
			}
			break;

		case 1:
			if((vo_in->TCFR == PICK) && (vi->AZTR_R == PICK)) { // Clear For TCF
				nvo->TCF_G = 1;
				nvo->TCF_GA = 1;
				lstate_TCF_SM_DL = 2;

				// clear for CPU0 OLED
				nvo->LINE_STATUS_TCF = LINE_CLEAR;
			}
			break;

		case 2:
			if((vo_in->TCFR == PICK) && (vi->AZTR_R == DROP)) { // Train Enter at section
				nvo->TCF_G = 0;
				nvo->TCF_GA = 0;

				lstate_TCF_SM_DL = 3;
			}
			if((vi->CNR == PICK) && (vi->BPN == PICK)) {
				cancel_req = 1;
			}
			if((vi_oe->CNL_COOP == PICK) && (cancel_req != 0U)) {
				g_120s_ticks = 120;
				cancel_req = 0;
				lstate_TCF_SM_DL = 0;
			}
			break;

		case 3:
			if((vo_in->TCFR == PICK) && (vi->AZTR_R == PICK)) { // After Train movement
				//if TOL_D set ignore SPARE8 and SPARE9 
				if(g_TCF_ticks == 0U) { // one second elapsed
					nvo->TCF_G  = (nvo->TCF_G != 0U) ? 0 : 1; // Toggle TCF_G
					nvo->TCF_GA = (nvo->TCF_GA != 0U) ? 0 : 1; // Toggle TCF_GA
					g_TCF_ticks = 1000; // set for one second
				}
				if((vi->CNR == PICK) && (vi->BPN == PICK)) {
					cancel_req = 1;
				}
				if((vi_oe->CNL_COOP == PICK) && (cancel_req != 0U)) { 
					g_120s_ticks = 120;
					cancel_req = 0;
					lstate_TCF_SM_DL = 0;
				}
				// Train coming From for CPU0 OLED
				nvo->LINE_STATUS_TCF = TRAIN_MOVING;
			}
			else if((vo_in->TCFR == DROP) && (vi->AZTR_R == PICK)) { // Train Exit at section - Line Close
				lstate_TCF_SM_DL = 4;
			}
			else {
				// do nothing
			}
			break;

		case 4:
			if((vo_in->TCFR == DROP) && (vi->AZTR_R == PICK)) { // Train Exit at section - Line Close
				nvo->TCF_G = 0;
				nvo->TCF_GA = 0;
				lstate_TCF_SM_DL = 0; // Reset to initial state
						   
				// Train exit From for CPU0 OLED
				nvo->LINE_STATUS_TCF = TRAIN_EXIT;
			}
			break;

		default:
			// shouldnt be here
			break;
	}
}

/** 
 * @brief Interlock State Machine for Buzzer of TGT side
 * It switches ON BELL on AZTR_D transitions.
 * On receiving ACKN, it will switch OFF the BELL.
 *
 * @param [in] vi_u.dl
 * Local-side vital input structure containing signals such as AZTR_D (line status) and ACKN_TGT (acknowledgement input).
 *
 * @param [in,out] nvo_u.x
 * Non-vital output structure used to control buzzer (SECTIONAL_BELL) and acknowledgement indicator (ACKN_D).
 *
 * @param [in,out] g_ACKN_D_ticks
 * Timer used to control duration of buzzer and acknowledgement signal.
 *
 * @return
 * None.
 *
 * @details
 * Requirement ID(s) -
 * PI_SSBPAC_SwRS_OPER_070
 *
 * @note
 * The function must be executed periodically to ensure proper timing and state transitions.
 *
 */

void interlock_Buzzer_SM_TGT_DL(void) {
	static uint8_t lstate_Buzzer_SM_TGT_DL = 0;

	vo_struct_t* vo = &vo_u.x;
	vo_struct_t* vo_in = &vo_in_u.x;
	nvo_struct_t* nvo = &nvo_u.x;
	lo_struct_t* lo_in = &lo_in_s;
	lo_struct_t* lo = &lo_u.x; 

	vi_dl_struct_t* vi = &vi_u.dl;

	if((vi->ACKN_TGT == PICK) && (g_ACKN_D_ticks == 0U)) {
		nvo->ACKN_D = 0;
		nvo->SECTIONAL_BELL = 0;
		nvo->ACKN_D = nvo->SECTIONAL_BELL; //TODO Not required
	}
	else {
		//do nothing	
	}

	// break commented intentionally to avoid confusion with the switch case structure
	// maintained switch-case for easy expansion in future
	switch (lstate_Buzzer_SM_TGT_DL)
	{
		case 0:
			if(vi->AZTR_D == DROP) {
				lstate_Buzzer_SM_TGT_DL = 2;
			}
			else if(vi->AZTR_D == PICK) {
				lstate_Buzzer_SM_TGT_DL = 1;
			}
			else {
				//do nothing	
			}
			nvo->ACKN_D = 0;
			nvo->SECTIONAL_BELL = 0;
			nvo->ACKN_D = nvo->SECTIONAL_BELL;
			break;

		case 1:
			if(vi->AZTR_D == DROP) {
				nvo->ACKN_D = 1;
				nvo->SECTIONAL_BELL = 1;
				g_ACKN_D_ticks = 2000;
				lstate_Buzzer_SM_TGT_DL = 2;
			}
			else {
				//do nothing	
			}
			break;
		case 2:
			if(vi->AZTR_D == PICK) {
				nvo->ACKN_D = 1;
				nvo->SECTIONAL_BELL = 1;
				g_ACKN_D_ticks = 2000;
				lstate_Buzzer_SM_TGT_DL = 1;
			}
			else {
				//do nothing	
			}			
			break;

		default:
			// do nothing
			break;
	}
}

/**
 * @brief Interlock State Machine for Buzzer of TCF side
 * It switches ON BELL on AZTR_R transitions.
 * On receiving ACKN, it will switch OFF the BELL.
 *
 * @param [in] vi_u.dl
 * Local-side vital input structure containing signals such as AZTR_R
 * (line status) and TCFR_FDBK (used as acknowledgement input).
 *
 * @param [in,out] nvo_u.x
 * Non-vital output structure used to control acknowledgement indicator
 * (ACKN_R) and sectional bell (SECTIONAL_BELL_DL).
 *
 * @param [in,out] g_ACKN_R_ticks
 * Timer used to control the duration of buzzer and acknowledgement signals.
 *
 * @return
 * None.
 *
 * @details
 * Requirement ID(s) -
 * PI_SSBPAC_SwRS_OPER_071
 *
 * @note
 * The function must be executed periodically for correct timing and state transitions.
 */

void interlock_Buzzer_SM_TCF_DL(void) {
	static uint8_t lstate_Buzzer_SM_TCF_DL = 0;
	vo_struct_t* vo = &vo_u.x;
	vo_struct_t* vo_in = &vo_in_u.x;
	nvo_struct_t* nvo = &nvo_u.x;
	lo_struct_t* lo_in = &lo_in_s;
	lo_struct_t* lo = &lo_u.x; 

	vi_dl_struct_t* vi = &vi_u.dl;

	//tcfr_fdbk used at place of ackn_tcf	
	if((vi->TCFR_FDBK == PICK) && (g_ACKN_R_ticks == 0U)) {
		nvo->ACKN_R = 0;
		nvo->SECTIONAL_BELL_DL = 0;
		nvo->ACKN_R = nvo->SECTIONAL_BELL_DL;
	}
	else {
		// do nothing
	}


	// break commented intentionally to avoid confusion with the switch case structure
	// maintained switch-case for easy expansion in future
	switch (lstate_Buzzer_SM_TCF_DL)
	{
		case 0:
			if(vi->AZTR_R == DROP) {
				lstate_Buzzer_SM_TCF_DL = 2;
			}
			else if(vi->AZTR_R == PICK) {
				lstate_Buzzer_SM_TCF_DL = 1;
			}
			else {
				// do nothing
			}
			nvo->ACKN_R = 0;
			nvo->SECTIONAL_BELL_DL = 0;
			nvo->ACKN_R = nvo->SECTIONAL_BELL_DL;
			break;

		case 1:
			if(vi->AZTR_R == DROP) {
				nvo->ACKN_R = 1;
				nvo->SECTIONAL_BELL_DL = 1;
				g_ACKN_R_ticks = 2000;
				lstate_Buzzer_SM_TCF_DL = 2;
			}
			else {
				// do nothing
			}
			break;
		case 2:
			if(vi->AZTR_R == PICK) {
				nvo->ACKN_R = 1;
				nvo->SECTIONAL_BELL_DL = 1;
				g_ACKN_R_ticks = 2000;
				lstate_Buzzer_SM_TCF_DL = 1;
			}
			else {
				// do nothing
			}
			break;

		default:
			// shouldnt be here
			break;
	}
}
