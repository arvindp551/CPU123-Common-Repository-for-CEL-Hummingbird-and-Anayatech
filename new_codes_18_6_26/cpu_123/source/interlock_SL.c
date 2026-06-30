/***************************************************************************//**
 * @file     interlock_SL.c
 * @brief    All application specific source files
 * @version  1.0.0
 * @details
 * Compiler : Keil uVision
 * Target : Nuvoton M263
 * Module : Central Controller Card
 * Date Created : Feb 16, 2026
 * @copyright Copyright (C) 2026 Anaya Tech Systems Pvt. Ltd. . All rights reserved.
 * @author    DG
 * @par Functions Included
 * void interlock_SL(void); <br>
 * void interlock_TGT_SM(void); <br>
 * void interlock_TCF_SM(void); <br>
 * void interlock_Buzzer_SM(void);
 ******************************************************************************/
 
#include "NuMicro.h"
#include "stdbool.h"
#include "interlock.h"
#include "app.h"

/**
* @brief
* Executes Single Line (SL) interlocking logic using a state-machine approach.
*
* This function implements the interlocking logic for Single Line mode.
* It initializes default states, copies previous cycle states, and computes
* logical outputs, vital outputs (VO), and non-vital outputs (NVO) based on
* local and remote input conditions.
*
* @param [in] vi_u.sl
* Structure containing local-side vital input signals used for interlocking
* decision-making (e.g., AZTR_R, ASGNCR, HSGNCR, SHKR).
*
* @param [in] vi_oe_u.sl
* Structure containing remote-side vital input signals for coordination
* with the other station.
*
* @param [in,out] lo_u.x
* Logical output structure updated based on computed interlocking conditions.
*
* @param [in,out] vo_u.x
* Vital output structure controlling relay states such as TGTR, TCFR, and ASCR.
*
* @param [out] nvo_u.x
* Non-vital output structure used for status indicators, LEDs, and counters.
*
* @param [in] lo_oe_u.x
* Logical outputs from the remote end used for interlocking coordination.
*
* @param [in] vo_oe_u.x
* Vital outputs from the remote end used for synchronization and decision logic.
*
* @return
* None.
*
* @details
* Requirement ID(s) -
* PI_SSBPAC_SwRS_OPER_063
*
* @note
*
* Logical decisions are based on combinations of local inputs, remote inputs, feedback signals, and internal states.
*
* TGTR, TCFR, and ASCR outputs are derived using multiple conditional paths to ensure safe interlocking operation.
*
* Non-vital outputs (NVO) provide system status such as line condition, signal indications, and operator feedback.
*
* The function must be executed periodically to maintain proper interlocking behavior.
*
* Care must be taken to ensure synchronization of shared/global variables when used in interrupt-driven or multi-context systems.
*/
	
void interlock_SL(void) {
	static uint8_t lstate_SL = 0;
	uint8_t i;
	bool x;
	
	vo_struct_t* vo = &vo_u.x;
	vo_struct_t* vo_in = &vo_in_u.x;
	nvo_struct_t* nvo = &nvo_u.x;
	lo_struct_t* lo_in = &lo_in_s;
	lo_struct_t* lo = &lo_u.x; 

	vi_sl_struct_t* vi = &vi_u.sl;
	vi_sl_struct_t* vi_oe = &vi_oe_u.sl; //remote side vital inputs
	lo_struct_t* lo_oe = &lo_oe_u.x; //remote side logical relays
	vo_struct_t* vo_oe = &vo_oe_u.x;

	switch(lstate_SL) {
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

			lstate_SL = 1;
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

			lstate_SL = 2;
			break;
		case 2: // compute logical, vo and nvo states // update as per Version 1.4 dated 27-12-2024
			TGTR_1 = ((vi->AZTR_R==PICK) && (vi->ASGNCR==PICK) && (vi->HSGNCR==PICK) && (vi->SHKR==PICK) && (vi->TGTN==PICK) && (vi->BPN==PICK) && (vi->CNR==DROP) && (lo_in->TGTYR==PICK) && (lo_in->TGTXR==PICK) && (lo_in->TCFXR==DROP) && (lo_in->BTSR==PICK) && (vo_in->TCFR==DROP) && (vi->SM_KEY==PICK))?PICK:DROP;
			TGTR_2 = ((vi->AZTR_R==PICK) && (vi->ASGNCR==PICK) && (vi->HSGNCR==PICK) && (vi->SHKR==PICK) && (vi->TGTN==DROP) && (vi->BPN==DROP) && (vi->CNR==DROP) && (lo_in->TGTZR==PICK) && (lo_in->TGTYR==DROP) && (lo_in->TCFXR==DROP) && (lo_in->CAR==DROP) && (lo_in->ASGNCPR==PICK) && (lo_in->JPR120==DROP) && (lo_in->TAR1==DROP) && (lo_in->TAR2==DROP) && (vi->SM_KEY==PICK))?PICK:DROP;
			TGTR_3 = DROP;

			TCFR_1 = ((vi->AZTR_R==PICK) && (vi->ASGNCR==PICK) && (vi->HSGNCR==PICK) && (vi->HSATPR==PICK) && (vi->HSBTPR==PICK) && (vi->SHKR==PICK) && (vi->TGTN==DROP) && (vi->BPN==DROP) && (vi->CNR==DROP) && (lo_in->TGTZR==PICK) && (lo_in->TGTYR==DROP) && (lo_in->TCFXR==PICK) && (lo_in->BTSR==PICK) && (lo_in->CAR==DROP) && (lo_in->ASGNCPR==PICK) && (lo_in->JPR120==DROP) && (lo_in->TAR1==DROP) && (lo_in->TAR2==DROP) && (vo_in->TGTR==DROP) && (vi->SM_KEY==PICK))?PICK:DROP;
			TCFR_2 = ((vi->AZTR_R==PICK) && (vi->ASGNCR==PICK) && (vi->HSGNCR==PICK) && (vi->SHKR==PICK) && (vi->TGTN==DROP) && (vi->BPN==DROP) && (vi->CNR==DROP) && (lo_in->TGTZR==PICK) && (lo_in->TGTYR==DROP) && (lo_in->TCFXR==DROP) && (lo_in->CAR==PICK) && (lo_in->ASGNCPR==PICK) && (lo_in->JPR120==PICK) && (vi->SM_KEY==PICK))?PICK:DROP;
			TCFR_3 = ((vi->AZTR_R==PICK) && (vi->ASGNCR==PICK) && (vi->HSGNCR==PICK) && (vi->SHKR==PICK) && (vi->TGTN==DROP) && (vi->BPN==DROP) && (vi->CNR==DROP) && (lo_in->TGTZR==PICK) && (lo_in->TGTYR==DROP) && (lo_in->TCFXR==DROP) && (lo_in->CAR==DROP) && (lo_in->ASGNCPR==PICK) && (lo_in->TAR2==PICK) && (vi->SM_KEY==PICK))?PICK:DROP;

			ASCR_1  = ((vi->AZTR_R==PICK) && (vi->SHKR==PICK) && (lo_in->TGTYR==PICK) && (vo_in->TGTR==PICK) && (vi->TGTR_FDBK==PICK))?PICK:DROP;
			ASCR_2  = ((vi->AZTR_R==PICK) && (vi->ASCR ==PICK) && (vo_in->TGTR==PICK))?PICK:DROP;

			lo->TGTXR = ((vi->AZTR_R==PICK) && (vi->ASGNCR==PICK) && (vi->HSGNCR==PICK) && (vi->SHKR==PICK) && (vi->TGTN==PICK) && (vi->BPN==PICK) && (vi->CNR==DROP) && (lo_in->TCFXR==DROP) && (lo_in->BTSR==PICK) && (vo_in->TCFR==DROP) && (vo_in->TGTR==DROP) && (vi->SM_KEY==PICK))?PICK:DROP;
			lo->TGTYR = ((vo_oe->TCFR==PICK) && (vo_oe->TGTR==DROP) && (vi_oe->ASGNCR==PICK) && (vi_oe->SHKR==PICK) && (lo_oe->BTSR==PICK) && (lo_oe->TGTYR==DROP) && (vo_oe->TCFR==PICK) && (vo_oe->TGTR==DROP))?PICK:DROP;
			lo->TGTZR = ((vo_oe->TCFR==DROP) && (vi_oe->HSGNCR==PICK) && (vo_oe->TCFR==DROP))?PICK:DROP;
			lo->TCFXR = (lo_oe->TGTXR==PICK)?PICK:DROP;
			lo->ASGNCPR = ((vi_oe->ASGNCR==PICK) && (vi_oe->SHKR==PICK))?PICK:DROP;
			x = ((vi->AZTR_R==PICK) && (vi->ASGNCR==PICK) && (vi->HSGNCR==PICK) && (vi->BPN==PICK) && (vi->CNR==PICK) && (lo_in->TGTZR==PICK) && (lo_in->ASGNCPR==PICK) && (lo_in->TCFCR==PICK) && (vo_in->TCFR==PICK) && (vi->SM_KEY==PICK));
			x = x || ((vi->AZTR_R==PICK) && (lo_in->CAR==PICK) && (vo_in->TCFR==PICK) && (vi->SM_KEY==PICK));
			lo->CAR = x?PICK:DROP;
			x = ((vi->AZTR_R==PICK) && (lo_in->CAR==DROP) && (lo_in->TGTPR==PICK) && (lo_in->TCFZR==PICK));
			x = x || ((vi->AZTR_R==PICK) && (lo_in->BTSR==PICK) && (lo_in->CAR==DROP));
			lo->BTSR = x?PICK:DROP;
			x = ((vi->AZTR_R==PICK) && (lo_in->BTSR==DROP) && (lo_in->FR2==DROP) && (vo_in->TCFR==PICK));
			x = x || ((vi->AZTR_R==PICK) && (lo_in->TGTYR==DROP) && (lo_in->FR2==DROP) && (vo_in->TGTR==PICK));
			lo->FR1 = x?PICK:DROP;
			x = ((vi->AZTR_R==PICK) && (lo_in->BTSR==DROP) && (lo_in->FR1==PICK) && (vo_in->TCFR==PICK));
			x = x || ((vi->AZTR_R==PICK) && (lo_in->TGTYR==DROP) && (lo_in->FR1==PICK) && (vo_in->TGTR==PICK));
			lo->FR2 = x?PICK:DROP;
			x = ((vi->HSATPR==PICK) && (vi->HSBTPR==DROP) && (lo_in->BTSR==DROP) && (lo_in->TAR1==PICK));
			x = x || ((lo_in->BTSR==DROP) && (lo_in->TAR2==PICK));
			lo->TAR2 = x?PICK:DROP;
			x = ((vi->HSBTPR==PICK) && (lo_in->TAR2==DROP) && (vo_in->TCFR==PICK));
			x = x || ((vi->HSATPR==DROP) && (lo_in->TAR1==PICK));
			lo->TAR1 = (x)?PICK:DROP;
			lo->JPR120 = (lo_in->CAR==PICK)?PICK:DROP;
			lo->TCFZR = (vo_in->TCFR==DROP)?PICK:DROP;
			lo->TCFCR = ((vi_oe->ASGNCR==PICK) && (vi_oe->SHKR==PICK))?PICK:DROP;
			lo->TGTPR = (vo_in->TGTR==DROP)?PICK:DROP;

			nvo->CAN_CNTR = (lo_in->CAR==PICK)?1:0;
			nvo->TGT_R_TOL = (((vi->AZTR_R==DROP) && (vo_in->TGTR==PICK)) || ((vi->SHKR == DROP) && (vi->AZTR_R==DROP)))?1:0;
			nvo->TGT_RA = (((vi->AZTR_R==DROP) && (vo_in->TGTR==PICK)) || ((vi->SHKR == DROP) && (vi->AZTR_R==DROP)))?1:0;
			nvo->TCF_R_TOL = ((vi->AZTR_R==DROP) && (vo_in->TCFR==PICK))?1:0;
			nvo->TCF_RA = (vi_oe->AZTR_R==DROP)?1:0;

			nvo->SMKEY = (vi->SM_KEY==PICK)?1:0;
			nvo->SNOEK = (vo_oe->ASCR==DROP)?1:0;
			nvo->SNK = (vi->ASCR==DROP)?1:0;
			nvo->LC = ((vi->AZTR_R==PICK) && (vi->ASGNCR==PICK) && (vi->HSGNCR==PICK) && (vi->HSATPR==PICK) && (vi->HSBTPR==PICK) && (vi->ASCR==DROP) && (vo_in->TCFR==DROP) && (vi->SHKR==PICK) && (vo_in->TGTR==DROP))?1:0;
			nvo->SHK_G = ((vi->EKT_KEY==PICK) && (vi->SR_KEY==DROP))?1:0;
			nvo->SHK_R = (vi->SR_KEY==PICK)?1:0;
			nvo->SNK_R = 0;
			nvo->RESET_CNTR = ((vi->SM_KEY==PICK) && (vi->RST_BTN==PICK))?1:0;
			nvo->LSS_R = (vi->ASCR==DROP)?1:0;
			nvo->LSS_G = (vi->ASCR==PICK)?1:0;
			nvo->LINE_F = (vi->AZTR_R==PICK)?1:0;
			nvo->LINE_O = (vi->AZTR_R==DROP)?1:0;
			nvo->BELL = ((vi_oe->BPN==PICK) && (vi_oe->SM_KEY==PICK))?1:0;

			lstate_SL = 1;
			break;
		default :
			// shouldnt be here
			break;
	}
}

/**
 * @brief Interlock State Machine for TGT
 * This function implements the interlocking logic for the TGT section.
 * It manages the state transitions based on the inputs and controls TGT_G and TGT_GA outputs accordingly.
 * The state machine is designed to PICK, DROP or Pulsate TGT_G(A) on train movements, line closures.
 *
 * @param [in] vo_in_u.x
 * Vital output feedback structure used to monitor TGTR (Train Grant Token Relay) status.
 *
 * @param [in,out] nvo_u.x
 * Non-vital output structure updated with TGT indications (TGT_G, TGT_GA) and
 * cooperation status (CAN_COOP).
 *
 * @param [in] vi_u.sl
 * Local-side vital input structure containing signals such as AZTR_R, SHKR,
 * CNL_COOP, etc.
 *
 * @param [in] vi_oe_u.sl
 * Remote-side vital input structure used for cooperation logic (e.g., CNR, BPN).
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
 * This function must be executed periodically (e.g., in main loop or task) for correct timing and state transitions.
 *
 * Proper handling of global timing variables (g_TGT_ticks, g_120s_ticks) is required for deterministic behavior.
 */
 
void interlock_TGT_SM(void) {
	static uint8_t lstate_TGT_SM = 0;
	static uint8_t shkr_state_SL = 0;

	vo_struct_t* vo_in = &vo_in_u.x;
	nvo_struct_t* nvo = &nvo_u.x;

	vi_sl_struct_t* vi = &vi_u.sl;
	vi_sl_struct_t* vi_oe = &vi_oe_u.sl; //remote side vital inputs

	//If SHKR transitions TGTR should be ignored so TGTR SM state is set to 0	
	// break commented intentionally to avoid confusion with the switch case structure
	// maintained switch-case for easy expansion in future
	if(shkr_state_SL == 0U) {
		switch (lstate_TGT_SM)
		{
			case 0:
				// Let TGT_G and TG_GA blick for 120 seconds
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
					lstate_TGT_SM = 1;
				}
				else {
					// do nothing
				}

				break; 

			case 1:
				if((vo_in->TGTR == PICK) && (vi->AZTR_R == PICK)) { // Clear For TGT
					nvo->TGT_G = 1;
					nvo->TGT_GA = 1;
					lstate_TGT_SM = 2;
				}
				else {
					// do nothing
				}
				break;

			case 2:
				if((vo_in->TGTR == PICK) && (vi->AZTR_R == DROP)) { // Train Enter at section
					nvo->TGT_G = 0;
					nvo->TGT_GA = 0;
					lstate_TGT_SM = 3;
				}
				else {
					if((vi_oe->CNR == PICK) && (vi_oe->BPN == PICK)) {
						nvo->CAN_COOP = 1;
					}		
					if((vi->CNL_COOP == PICK) && (nvo->CAN_COOP == 1U)) {  			
						g_120s_ticks = 120;
						nvo->CAN_COOP = 0;
						lstate_TGT_SM = 0;
					}
				}				
				break;

			case 3:
				if((vo_in->TGTR == PICK) && (vi->AZTR_R == PICK)) { // After Train movement
					if(g_TGT_ticks == 0U) { // one second elapsed
						nvo->TGT_G  = (nvo->TGT_G != 0U)?0:1; // Toggle TGT_G
						nvo->TGT_GA = (nvo->TGT_GA != 0U)?0:1; // Toggle TGT_GA
						g_TGT_ticks = 1000; // set for one second
					}
					if((vi_oe->CNR == PICK) && (vi_oe->BPN == PICK)) {
						nvo->CAN_COOP = 1;
					}		
					if((vi->CNL_COOP == PICK) && (nvo->CAN_COOP == 1U)) {  			
						g_120s_ticks = 120;
						nvo->CAN_COOP = 0;
						lstate_TGT_SM = 0;
					}
				}
				else if((vo_in->TGTR == DROP) && (vi->AZTR_R == PICK)) { // Train Exit at section - Line Close
					lstate_TGT_SM = 4;
				}
				else {
					// do nothing
				}				
				break;

			case 4:
				if((vo_in->TGTR == DROP) && (vi->AZTR_R == PICK)) { // Train Exit at section - Line Close
					nvo->TGT_G = 0;
					nvo->TGT_GA = 0;
					nvo->CAN_COOP = 0;
					lstate_TGT_SM = 0; // Reset to initial state
				}
				else {
					// do nothing
				}
				break;

			default: 
				//shouldn't be here
				break;
		}
	}

	// break commented intentionally to avoid confusion with the switch case structure
	// maintained switch-case for easy expansion in future
	switch (shkr_state_SL)
	{
		case 0:
			if((vi->SHKR == DROP) && (vi->AZTR_R == PICK)) {
				nvo->TGT_G = 0;
				nvo->TGT_GA = 0;
				shkr_state_SL = 1;
			}
			else {
				// do nothing
			}
			break;

		case 1:
			if((vi->SHKR == DROP) && (vi->AZTR_R == DROP)) {
				nvo->TGT_G = 0;
				nvo->TGT_GA = 0;
				shkr_state_SL = 2;
			}
			else {
				// do nothing
			}
			break;

		case 2:
			if((vi->SHKR == DROP) && (vi->AZTR_R == PICK)) {
				if(g_TGT_ticks == 0U) { // one second elapsed
					nvo->TGT_G  = (nvo->TGT_G != 0U)?0:1; // Toggle TGT_G
					nvo->TGT_GA = (nvo->TGT_GA != 0U)?0:1; // Toggle TGT_GA
					g_TGT_ticks = 1000; // set for one second
				}
				else {
				// do nothing
				}
			}
			else if((vi->SHKR == PICK) && (vi->AZTR_R == PICK)) {
				shkr_state_SL = 0;
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

/** 
 * @brief Interlock State Machine for TCF
 * This function implements the interlocking logic for the TCF section.
 * It manages the state transitions based on the inputs and controls TCF_G and TCF_GA outputs accordingly.
 * The state machine is designed to PICK, DROP or Pulsate TCF_G(A) on train movements, line closures.
 *
 * @param [in] vo_in_u.x
 * Vital output feedback structure used to monitor TCFR (Train Clear From Relay)
 * status.
 *
 * @param [in,out] nvo_u.x
 * Non-vital output structure updated with TCF indications (TCF_G, TCF_GA) and
 * CANCEL indication.
 *
 * @param [in] vi_u.sl
 * Local-side vital input structure containing signals such as AZTR_R, CNR, BPN,
 * etc.
 *
 * @param [in] vi_oe_u.sl
 * Remote-side vital input structure used for cancellation cooperation
 * (e.g., CNL_COOP).
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
 * Outputs are reset to safe states during transitions and invalid conditions.
 *
 * The function must be called periodically to maintain proper timing, blinking behavior, and state transitions.
 *
 * Proper synchronization of global timing variables and shared data structures is required in interrupt-driven systems.
 */
 
void interlock_TCF_SM(void) {
	static uint8_t cancel_req_TCF_SM = 0;
	static uint8_t lstate_TCF_SM = 0;

	vo_struct_t* vo_in = &vo_in_u.x;
	nvo_struct_t* nvo = &nvo_u.x;

	vi_sl_struct_t* vi = &vi_u.sl;
	vi_sl_struct_t* vi_oe = &vi_oe_u.sl; //remote side vital inputs

	// break commented intentionally to avoid confusion with the switch case structure
	// maintained switch-case for easy expansion in future
	switch (lstate_TCF_SM)
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
				lstate_TCF_SM = 1;
			}
			else {
				// do nothing
			}				
			break;

		case 1:
			if((vo_in->TCFR == PICK) && (vi->AZTR_R == PICK)) { // Clear For TCF
				nvo->TCF_G = 1;
				nvo->TCF_GA = 1;
				lstate_TCF_SM = 2;
			}
			break;

		case 2:
			if((vo_in->TCFR == PICK) && (vi->AZTR_R == DROP)) { // Train Enter at section
				nvo->TCF_G = 0;
				nvo->TCF_GA = 0;
				lstate_TCF_SM = 3;
			}
			if((vi->CNR == PICK) && (vi->BPN == PICK)) {
				cancel_req_TCF_SM = 1;
			}
			if((vi_oe->CNL_COOP == PICK) && (cancel_req_TCF_SM != 0U)) { 
				g_120s_ticks = 120;
				cancel_req_TCF_SM = 0;
				lstate_TCF_SM = 0;
			}
			break;

		case 3:
			if((vo_in->TCFR == PICK) && (vi->AZTR_R == PICK)) { // After Train movement
				if(g_TCF_ticks == 0U) { // one second elapsed
					nvo->TCF_G  = (nvo->TCF_G != 0U) ? 0 : 1; // Toggle TCF_G
					nvo->TCF_GA = (nvo->TCF_GA != 0U) ? 0 : 1; // Toggle TCF_GA
					g_TCF_ticks = 1000; // set for one second
				}
				if((vi->CNR == PICK) && (vi->BPN == PICK)) {
					cancel_req_TCF_SM = 1;
				}
				if((vi_oe->CNL_COOP == PICK) && (cancel_req_TCF_SM != 0U)) { 
					g_120s_ticks = 120;
					cancel_req_TCF_SM = 0;
					lstate_TCF_SM = 0;
				}
			}
			else if((vo_in->TCFR == DROP) && (vi->AZTR_R == PICK)) { // Train Exit at section - Line Close
				lstate_TCF_SM = 4;
			}
			else {
				// do nothing
			}
			break;

		case 4:
			if((vo_in->TCFR == DROP) && (vi->AZTR_R == PICK)) { // Train Exit at section - Line Close
				nvo->TCF_G = 0;
				nvo->TCF_GA = 0;
				lstate_TCF_SM = 0; // Reset to initial state
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

/**
 * @brief Interlock State Machine for Buzzer
 * It switches ON BELL on AZTR_R transitions.
 * On receiving ACKN, it will switch OFF the BELL.
 *
 * @param [in,out] nvo_u.x
 * Non-vital output structure used to control buzzer-related outputs such as
 * ACKN_D (acknowledgment) and SECTIONAL_BELL.
 *
 * @param [in] vi_u.sl
 * Local-side vital input structure containing AZTR_R and ACKN signals used
 * for buzzer triggering and reset conditions.
 *
 * @return
 * None.
 *
 * @details
 * Requirement ID(s) -
 * PI_SSBPAC_SwRS_OPER_071
 *
 * @note
 * The function ensures proper audible indication for line state transitions
 * and acknowledgment handling.
 *
 * Must be called periodically to maintain correct timing and transitions.
 *
 * Designed for easy extensibility using switch-case structure.
 *
 */
 
void interlock_Buzzer_SM(void) {
	static uint8_t lstate_Buzzer_SM = 0;
	
	nvo_struct_t* nvo = &nvo_u.x;
	
	vi_sl_struct_t* vi = &vi_u.sl;

	if((vi->ACKN == PICK) && (g_ACKN_D_ticks == 0U)) {
		nvo->ACKN_D = 0;
		nvo->SECTIONAL_BELL = 0;
	}
	else {
		// do nothing
	}
	// break commented intentionally to avoid confusion with the switch case structure
	// maintained switch-case for easy expansion in future
	switch (lstate_Buzzer_SM)
	{
		case 0:
			if(vi->AZTR_R == DROP) {
				lstate_Buzzer_SM = 2;
			}
			else if(vi->AZTR_R == PICK) {
				lstate_Buzzer_SM = 1;
			}
			else {
				// do nothing
			}
			nvo->ACKN_D = 0;
			nvo->SECTIONAL_BELL = 0;
			break;

		case 1:
			if(vi->AZTR_R == DROP) {
				nvo->ACKN_D = 1;
				nvo->SECTIONAL_BELL = 1;
				g_ACKN_D_ticks = 2000;
				lstate_Buzzer_SM = 2;
			}
			else {
				// do nothing
			}
			break;
		case 2:
			if(vi->AZTR_R == PICK) {
				nvo->ACKN_D = 1;
				nvo->SECTIONAL_BELL = 1;
				g_ACKN_D_ticks = 2000;
				lstate_Buzzer_SM = 1;
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
