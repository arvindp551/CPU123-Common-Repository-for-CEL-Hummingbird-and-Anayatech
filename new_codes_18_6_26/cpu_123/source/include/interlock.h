/***************************************************************************//**
 * @file     interlock.h
 * @brief    All application specific source files
 * @version  1.0.0
 * @details
 * Compiler : Keil uVision
 * Target : Nuvoton M263
 * Module : Central Controller Card
 * Date Created : Dec 12, 2025
 * @copyright Copyright (C) 2026 Anaya Tech Systems Pvt. Ltd. . All rights reserved.
 * @author    DG 
 ******************************************************************************/
#ifndef INTERLOCK_H__
#define INTERLOCK_H__

#include "common_header.h"

#define NR_VITAL_INPUT 		40U
#define NR_VITAL_OUTPUT 	16U
#define NR_NON_VITAL_OUTPUT	40U 
#define NR_LOGIC_RELAYS		48U //42b (21x2b)relays + 6 spares 

#define PICK 				2U //10
#define DROP 				1U //01

/* unsigned char used in place of uint8_t typedef as per MISRA C guideline
 * Bit field is not bool or explicit integral. (MISRA C:2025 R.6.1)
 */
//vital input structure for double line
typedef struct __attribute__((__packed__)) {
	// Inputs from Line
	unsigned char AZTR_R:2;
	unsigned char AZTR_D:2;
	unsigned char ASGNCR:2;
	unsigned char HSGNCR:2;
	unsigned char ASCR_FDBK:2;
	unsigned char TGTR_FDBK:2; //used as ASRR
	unsigned char TCFR_FDBK:2; //used as ackn_tcf
	unsigned char HSATPR:2;
	unsigned char HSBTPR:2;
	unsigned char HSRR:2;
	// Inputs from SM Panel
	unsigned char TGTN:2; //Train Going To
	unsigned char BPN:2;  //Bell Button
	unsigned char ACKN_TGT:2; //Ack Button
	unsigned char CNR:2;  // Cancel Button
	unsigned char CNL_COOP:2; //Cancel Co-Op button
	unsigned char SM_KEY:2; //SM key
	unsigned char LCB_KEY:2; //Block Line Clear
  	unsigned char ASCR:2;
  	//uint8_t ACKN_TCF:2; //ACK on receiving SMP
	unsigned char EKT_KEY:2;
  	unsigned char RST_BTN:2;
} vi_dl_struct_t;

// vital input structure for single line
typedef struct __attribute__((__packed__)) {
	// Inputs from Line
	unsigned char AZTR_R:2;
	unsigned char SHKR:2;
	unsigned char ASGNCR:2;
	unsigned char HSGNCR:2;
	unsigned char ASCR:2;
	unsigned char TGTR_FDBK:2; //used ASRR
	unsigned char TCFR_FDBK:2; //not used
	unsigned char HSATPR:2;
	unsigned char HSBTPR:2;
	unsigned char HSRR:2;
	// Inputs from SM Panel
	unsigned char TGTN:2; //Train Going To
	unsigned char BPN:2;  //Bell Button
	unsigned char ACKN:2; //Ack Button
	unsigned char CNR:2;  // Cancel Button
	unsigned char CNL_COOP:2; //Cancel Co-Op button
	unsigned char SM_KEY:2; //SM key
	unsigned char SR_KEY:2; //Shunt Release Key
  	unsigned char RST_BTN:2;
  	unsigned char EKT_KEY:2; //not used
  	unsigned char ASCR_FDBK:2; //not used
} vi_sl_struct_t;

typedef union {
	vi_dl_struct_t dl;
	vi_sl_struct_t sl;
	uint8_t bytes[NR_VITAL_INPUT/8U];
} vi_union_t;

//vital output structure
typedef struct __attribute__((__packed__)) {
	unsigned char TGTR:2; // output relay
	unsigned char TGTR_D:2; // output relay
	unsigned char TCFR:2; // output relay
	unsigned char TCFR_D:2; // output relay
	unsigned char ASCR:2; // output relay
	unsigned char SPARE0:2; // not used
	unsigned char SPARE1:2; // not used
	unsigned char SPARE2:2; // not used
} vo_struct_t;

typedef union {
	vo_struct_t x;
	uint8_t bytes[NR_VITAL_OUTPUT/8U];
} vo_union_t;

// Non vital output structure
typedef struct __attribute__((__packed__)) {
	unsigned char CAN_CNTR:1;
	unsigned char TGT_R_TOL:1;
	unsigned char TGT_RA:1;
	unsigned char TGT_G:1;
	unsigned char TGT_GA:1;
	unsigned char TCF_R_TOL:1;
	unsigned char TCF_RA:1;
	unsigned char TCF_G:1;
	unsigned char TCF_GA:1;
	unsigned char CANCEL:1;
	unsigned char CAN_COOP:1;
	unsigned char SMKEY:1;
	unsigned char SNOEK:1;
	unsigned char SNK:1;
	unsigned char LC:1;
	unsigned char ACKN_R:1;
	unsigned char COM0_STA:1;
	unsigned char SHK_G:1;
	unsigned char SHK_R:1;
	unsigned char ACKN_D:1;
	unsigned char SNK_R:1;
	unsigned char COM1_STA:1;
	unsigned char RESET_CNTR:1;
	unsigned char SSBPAC_OK:1;
	unsigned char SSBPAC_FAIL:1;
	unsigned char LSS_R:1;
	unsigned char LSS_G:1;
	unsigned char LINE_F:1;
	unsigned char LINE_O:1;
	unsigned char BELL:1;
	unsigned char SECTIONAL_BELL:1;
	unsigned char LC_DL:1;
	unsigned char LINE_TCF_F:1;
	unsigned char LINE_TCF_O:1;
	unsigned char LCB_KEY:1;
	unsigned char SECTIONAL_BELL_DL:1;
	unsigned char LINE_STATUS_TCF:2;
	unsigned char LINE_STATUS_TGT:2;
} nvo_struct_t;

typedef union {
	nvo_struct_t x;
	uint8_t bytes[NR_NON_VITAL_OUTPUT/8U];
} nvo_union_t;

//logical relay structure
typedef struct __attribute__((__packed__)) {
	unsigned char TGTXR:2;
	unsigned char TGTYR:2;
	unsigned char TCFRY:2;
	unsigned char TGTZR:2;
	unsigned char TCFXR:2;
	unsigned char ASGNCPR:2;
	unsigned char TGTYP:2;
	unsigned char UNR:2;
	unsigned char EGNR:2;
	unsigned char TCFRY_1:2;
	unsigned char CAR:2;
	unsigned char BTSR:2;
	unsigned char FR1:2;
	unsigned char FR2:2;
	unsigned char TAR2:2;
	unsigned char TAR1:2;
	unsigned char EJ120:2;
	unsigned char JPR120:2;
	unsigned char TCFZR:2;
	unsigned char TGTPR:2;
	unsigned char TCFCR:2;
	unsigned char SPARE:6;
} lo_struct_t;

typedef union {
	lo_struct_t x;
	uint8_t bytes[NR_LOGIC_RELAYS/8U];
} lo_union_t;

//for relay feadback
typedef struct __attribute__((__packed__)) {
	unsigned char RLY_FDBK_TGTR_P:2;
	unsigned char RLY_FDBK_TGTR_D:2;
	unsigned char RLY_FDBK_TCFR_P:2;
	unsigned char RLY_FDBK_TCFR_D:2;
	unsigned char RLY_FDBK_ASCR:2;
	unsigned char SPARE1:6;
	unsigned char RLY_FAULT_TGTR_P:2;
	unsigned char RLY_FAULT_TGTR_D:2;
	unsigned char RLY_FAULT_TCFR_P:2;
	unsigned char RLY_FAULT_TCFR_D:2;
	unsigned char RLY_FAULT_ASCR:2;
	unsigned char SPARE2:6;
} rly_fdbk_struct_t;

typedef union {
	rly_fdbk_struct_t x;
	uint8_t bytes[4];
} rly_fdbk_union_t;

//for MISC
typedef struct __attribute__((__packed__)) {
	unsigned char NVC_PRESENT:1;
	unsigned char OC_PRESENT:1;
	unsigned char IC1_PRESENT:1;
	unsigned char IC2_PRESENT:1;
	unsigned char IC3_PRESENT:1;
	unsigned char LINE_SELECTION_MODE:4;
	unsigned char SPARE1:7;
	unsigned char LOCAL_ID:8;
	unsigned char REMOTE_ID:8;
} misc_struct_t;

typedef union {
	misc_struct_t x;
	uint8_t bytes[4];
} misc_union_t;

extern uint16_t g_TGT_ticks;
extern uint16_t g_120s_ticks;
extern uint16_t g_TCF_ticks;
extern uint16_t g_ACKN_D_ticks;
extern uint16_t g_ACKN_R_ticks;

extern uint8_t TGTR_1, TGTR_2, TGTR_3;	
extern uint8_t TCFR_1, TCFR_2, TCFR_3;	
extern uint8_t ASCR_1, ASCR_2;	

extern vi_union_t vi_u;
extern vo_union_t vo_u;
extern vo_union_t vo_in_u;
extern nvo_union_t nvo_u;
extern lo_struct_t lo_in_s;
extern lo_union_t lo_u; 

//remote side vital inputs
extern vo_union_t vo_oe_u;
extern vi_union_t vi_oe_u; 
extern lo_union_t lo_oe_u; 

void interlock(void);

void interlock_set_defaults(void);
void interlock_Bypass(void);
void interlock_SL(void);
void interlock_TGT_SM(void);
void interlock_TCF_SM(void);
void interlock_Buzzer_SM(void);
void interlock_DL(void);
void interlock_TGT_SM_DL(void);
void interlock_TCF_SM_DL(void);
void interlock_Buzzer_SM_TCF_DL(void);
void interlock_Buzzer_SM_TGT_DL(void);

#endif  /* __INTERLOCK_H__ */
/*** (C) COPYRIGHT 2024 Anayatech Systems Pvt. Ltd. ***/
