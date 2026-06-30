/***************************************************************************//**
 * @file     interlock.h
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
#ifndef __INTERLOCK_H__
#define __INTERLOCK_H__

#include "common_header.h"

#define NR_VITAL_INPUT 40U
#define NR_VITAL_OUTPUT 16U
#define NR_NON_VITAL_OUTPUT	40U
#define NR_LOGIC_RELAYS	48U //42b (21x2b)relays + 6 spares 

#define PICK 2 //10
#define DROP 1 //01

typedef struct __attribute__((__packed__)) {
	// Inputs from Line
	uint8_t AZTR_R:2;
	uint8_t AZTR_D:2;
	uint8_t ASGNCR:2;
	uint8_t HSGNCR:2;
	uint8_t ASCR_FDBK:2;
	uint8_t TGTR_FDBK:2;
	uint8_t TCFR_FDBK:2;
	uint8_t HSATPR:2;
	uint8_t HSBTPR:2;
	uint8_t HSRR:2;
	// Inputs from SM Panel
	uint8_t TGTN:2; //Train Going To
	uint8_t BPN:2;  //Bell Button
	uint8_t ACKN_TGT:2; //Ack Button
	uint8_t CNR:2;  // Cancel Button
	uint8_t CNL_COOP:2; //Cancel Co-Op button
	uint8_t SM_KEY:2; //SM key
	
	uint8_t LCB_KEY:2; //Block Line Clear
   	uint8_t ASCR:2;	//ASRR
	uint8_t EKT_KEY:2;
  uint8_t RST_BTN:2;
} vi_dl_struct_t;

typedef struct __attribute__((__packed__)) {
	// Inputs from Line
	uint8_t AZTR_R:2;
	uint8_t SHKR:2;
	uint8_t ASGNCR:2;
	uint8_t HSGNCR:2;
	uint8_t ASCR:2;
	uint8_t TGTR_FDBK:2;
	uint8_t TCFR_FDBK:2;
	uint8_t HSATPR:2;
	uint8_t HSBTPR:2;
	uint8_t HSRR:2;
	// Inputs from SM Panel
	uint8_t TGTN:2; //Train Going To
	uint8_t BPN:2;  //Bell Button
	uint8_t ACKN:2; //Ack Button
	uint8_t CNR:2;  // Cancel Button 
	uint8_t CNL_COOP:2; //Cancel Co-Op button
	uint8_t SM_KEY:2; //SM key
	uint8_t SR_KEY:2; //Shunt Release Key
  	uint8_t RST_BTN:2;
  	uint8_t EKT_KEY:2; //not used
  	uint8_t ASCR_FDBK:2; //not used
} vi_sl_struct_t;

typedef union {
	vi_dl_struct_t dl;
	vi_sl_struct_t sl;
	uint8_t bytes[NR_VITAL_INPUT/8];
} vi_union_t;

typedef struct __attribute__((__packed__)) {
	uint8_t TGTR:2; // output relay
	uint8_t TGTR_D:2; // output relay
	uint8_t TCFR:2; // output relay
	uint8_t TCFR_D:2; // output relay
	uint8_t ASCR:2; // output relay
	uint8_t SPARE0:2; // not used
	uint8_t SPARE1:2; // not used
	uint8_t SPARE2:2; // not used
} vo_struct_t;

typedef union {
	vo_struct_t x;
	uint8_t bytes[NR_VITAL_OUTPUT/8];
} vo_union_t;

typedef struct __attribute__((__packed__)) {
	uint8_t CAN_CNTR:1;
	uint8_t TGT_R_TOL:1;
	uint8_t TGT_RA:1;
	uint8_t TGT_G:1;
	uint8_t TGT_GA:1;
	uint8_t TCF_R_TOL:1;
	uint8_t TCF_RA:1;
	uint8_t TCF_G:1;
	uint8_t TCF_GA:1;
	uint8_t CANCEL:1;
	uint8_t CAN_COOP:1;
	uint8_t SMKEY:1;
	uint8_t SNOEK:1;
	uint8_t SNK:1;
	uint8_t LC:1;
	uint8_t ACKN_R:1;
	uint8_t COM0_STA:1;
	uint8_t SHK_G:1;
	uint8_t SHK_R:1;
	uint8_t ACKN_D:1;
	uint8_t SNK_R:1;
	uint8_t COM1_STA:1;
	uint8_t RESET_CNTR:1;
	uint8_t SSBPAC_OK:1;
	uint8_t SSBPAC_FAIL:1;
	uint8_t LSS_R:1;
	uint8_t LSS_G:1;
	uint8_t LINE_F:1;
	uint8_t LINE_O:1;
	uint8_t BELL:1;
	uint8_t SECTIONAL_BELL:1;
	uint8_t LC_DL:1;
	uint8_t SPARE3:1;
	uint8_t SPARE4:1;
	uint8_t LCB_KEY:1;
	uint8_t SECTIONAL_BELL_DL:1;
	uint8_t LINE_STATUS_TCF:2;
	uint8_t LINE_STATUS_TGT:2;
} nvo_struct_t;

typedef union {
	nvo_struct_t x;
	uint8_t bytes[NR_NON_VITAL_OUTPUT/8];
} nvo_union_t;

typedef struct __attribute__((__packed__)) {
	uint8_t TGTXR:2;
	uint8_t TGTYR:2;
	uint8_t TCFRY:2;
	uint8_t TGTZR:2;
	uint8_t TCFXR:2;
	uint8_t ASGNCPR:2;
	uint8_t TGTYP:2;
	uint8_t UNR:2;
	uint8_t EGNR:2;
	uint8_t TCFRY_1:2;
	uint8_t CAR:2;
	uint8_t BTSR:2;
	uint8_t FR1:2;
	uint8_t FR2:2;
	uint8_t TAR2:2;
	uint8_t TAR1:2;
	uint8_t EJ120:2;
	uint8_t JPR120:2;
	uint8_t TCFZR:2;
	uint8_t TGTPR:2;
	uint8_t TCFCR:2;
	uint8_t SPARE:6;
} logical_struct_t;

typedef union {
	logical_struct_t x;
	uint8_t bytes[NR_LOGIC_RELAYS/8];
} logical_union_t;

extern vi_union_t vi_u;
extern vi_union_t vi_oe_u; //remote side vital inputs
extern vo_union_t vo_u;
extern vo_union_t vo_in_u;
extern nvo_union_t nvo_u;
extern logical_struct_t lo_in_s;
extern logical_union_t lo_u; 
extern logical_struct_t lo_s; 

void interlock();
void interlock_DL();
void interlock_SL();
void interlock_Bypass();
#endif  /* __INTERLOCK_H__ */
/*** (C) COPYRIGHT 2024 Anayatech Systems Pvt. Ltd. ***/
