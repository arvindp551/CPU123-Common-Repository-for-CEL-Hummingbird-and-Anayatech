/***************************************************************************//**
 * @file     interlock_BYPASS.c
 * @brief    All application specific source files
 * @version  1.0.0
 * @details
 * Compiler : Keil uVision
 * Target : Nuvoton M263
 * Module : Central Controller Card
 * Date Created : Jan 09, 2026
 * @copyright Copyright (C) 2026 Anaya Tech Systems Pvt. Ltd. . All rights reserved.
 * @author    DG 
 * @par Functions Included
 * void interlock_Bypass(void);
 ******************************************************************************/
 
#include "NuMicro.h"
#include "interlock.h"
#include "app.h"

/**
* @brief
* 	In bypass mode (MUX) vital output state are same as vital input state.
*
* 	In this mode, the interlocking logic is bypassed and the output relays are driven directly from the corresponding remote-side vital input signals.
* 	This mode is typically used for testing, diagnostics, or MUX-based operation where external control logic determines the relay states.
*
* @param [in] vi_oe_u.sl
* 		Structure containing remote-side vital input signals received from the external equipment interface.
*
* @param [in] vi_oe->AZTR_R
* 		Remote AZTR relay input mapped to TGTR output.
*
* @param [in] vi_oe->SHKR
* 		Remote SHKR relay input mapped to TGTR_D output.
*
* @param [in] vi_oe->ASGNCR
* 		Remote ASGNCR relay input mapped to TCFR output.
*
* @param [in] vi_oe->HSGNCR
* 		Remote HSGNCR relay input mapped to TCFR_D output.
*
* @param [in] vi_oe->ASCR
* 		Remote ASCR relay input mapped directly to ASCR output.
*
* @param [in] vi_oe->TGTR_FDBK
* 		Feedback signal mapped to spare output SPARE0.
*
* @param [in] vi_oe->TCFR_FDBK
* 		Feedback signal mapped to spare output SPARE1.
*
* @param [in] vi_oe->HSATPR
* 		Additional remote signal mapped to spare output SPARE2.
*
* @param [out] vo_u.x
* 		Vital Output structure where relay outputs are updated based on the received remote input signals.
*
* @return
* 		None.
*
* @details
* Requirement ID(s) -
* PI_SSBPAC_SwRS_OPER_064
*
* @note
* 		This function is used when the system operates in bypass or MUX mode.
* 		No internal interlocking safety logic is applied in this mode.
* 		Outputs are directly controlled by remote vital inputs.
* 		Spare outputs are used for diagnostic or feedback signal propagation.
*/
void interlock_Bypass(void) {
	vo_struct_t* vo = &vo_u.x;
	vi_sl_struct_t* vi_oe = &vi_oe_u.sl; //remote side vital inputs
	
	vo->TGTR = vi_oe->AZTR_R;
	vo->TGTR_D = vi_oe->SHKR;
	vo->TCFR = vi_oe->ASGNCR;
	vo->TCFR_D = vi_oe->HSGNCR;
	vo->ASCR = vi_oe->ASCR;
	vo->SPARE0 = vi_oe->TGTR_FDBK;
	vo->SPARE1 = vi_oe->TCFR_FDBK;
	vo->SPARE2 = vi_oe->HSATPR;	
}
