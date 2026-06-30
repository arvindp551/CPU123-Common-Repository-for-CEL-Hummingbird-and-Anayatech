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

#include "common_header.h"
#include "drvr_spi.h"
#include "fifo.h"
#include "interlock.h"

#define OK 			0x10U //changes for testin 10 to 0x10
#define NOT_OK 	0x01U

#define CMD_READ_STATUS					0xA1U
#define CMD_READ_INRLY					0xA2U
#define CMD_READ_INTERLOCK			0xA4U
#define CMD_READ_REG						0xA5U
#define CMD_READ_NON_VITAL			0xA6U
#define CMD_READ_FLASH_CHECKSUM	0xA7U
#define CMD_READ_REMOTE_INRLY		0xA8U //from CPU1,2,3
#define CMD_READ_VO							0xA8U //from FPGA
#define CMD_READ_LOGICAL_RELAY	0xA9U
#define CMD_READ_REMOTE_LO			0xAAU //from CPU1,2,3
#define CMD_READ_REMOTE_VO			0xABU //from CPU1,2,3
#define CMD_READ_INRLY_UNSTABLE	0xACU
#define CMD_READ_INTERLOCK_MODE	0xADU

#define CMD_RD_RLY_FDBK 		0xAAU
#define CMD_RD_MISC  				0xABU

#define CMD_WRITE_REG						0xB1U
#define CMD_WRITE_REMOTE_RELAY	0xB3U
#define CMD_WRITE_CPU_CTRLS			0xB4U

#define INTERLOCK_MODE_DOUBLE_LINE 0x03U
#define INTERLOCK_MODE_SINGLE_LINE 0x01U
#define INTERLOCK_MODE_MUX 0x02U
#define INTERLOCK_MODE_NONE 0x00U

#define STATUS_OK	0U
#define STATUS_FAIL	1U

#define DO_READ_FLASH_CHECKSUM   0U
#define DO_FLASH_INTEGRITY_CHECK 1U

#define IN_RLY_CARD  0U
#define OUT_RLY_CARD 3U
#define NV_RLY_CARD  4

#define ERR_CARD_NONE 0
#define ERR_CARD_NO_RESPONSE 1
#define ERR_CARD_ROM_FAULTY	2
#define ERR_CARD_VI_UNSTABLE	3
#define ERR_CARD_VI_MISMATCH	4
#define ERR_CARD_VO_MISMATCH	5
#define ERR_CARD_VI_NON_COMPLIMENTARY 6
#define ERR_CARD_VO_FDBK_MISMATCH 7
#define ERR_CARD_COM1 8
#define ERR_CARD_COM2 9
#define ERR_CARD_COM1n2 10 

#define ERR_RELAY_NONE 0
#define ERR_RELAY_NON_COMPLIMENTARY 1U

#define MS(v) (v/TICK_PERIOD)
#define SEC(v) ((v*1000U)/TICK_PERIOD)
#define HOUR(v) ((v*3600U*1000U)/TICK_PERIOD)

#define OE_RELAY_STATE_WIDTH ((NR_VITAL_INPUT + NR_LOGIC_RELAYS + NR_VITAL_OUTPUT)/8U)

#define STATE_OK		0x00U
#define STATE_OK_FAIL 	0x01U
#define STATE_FAIL		0x02U
#define STATE_CHECK		0x03U

#define PRIMARY_LINK 1 
#define BACKUP_LINK 0

typedef enum {
	INIT_STATE = 0,
	POST_STATE,
	OPERATIONAL_STATE
} CPU_state_e;

typedef enum {
	PKT_TYPE_STATUS = 0,
	PKT_TYPE_RESET,
	PKT_TYPE_ACK
} pkt_type_e;
// station configuration database structure

typedef struct {
	char stationID[17];
	char unit_address[5];
	char oe_station[17];
	char oe_unit_address[5];

	uint8_t flash_checksum[3][5]; //[cpuid CPU1,2,3][cksumB0,1,2,3,chksum read status err if != 0
	uint32_t my_flash_cksum;
	uint8_t block_mode;
	uint8_t DL_is_sending;
} cfg_t;

// Event Database Structure
typedef struct {
	//event database
	uint8_t state;
	uint8_t last_state;
	bool ev_pending;
	uint8_t ev_count;
	uint16_t err_code;
} event_t;

//Realy processor database structure
typedef struct {
	uint8_t rom_crc[4];

	//errors
	bool err_no_response;
	bool err_rom_faulty;
	bool err_vi_unstable;
	bool err_vi_mismatch; // vi not matching the vi after 2oo3 logic
	bool err_vo_mismatch; // interlock output not matching the 2oo3 output
	bool err_vo_fdbk_mismatch; // FPGA out relay fdbk not matching 2oo3 vital output
	bool err_vi_non_complimentary;
							   
	bool ev_pending;

	uint8_t err_code;
	uint8_t err_code_c;

	uint8_t flash_checksum[5]; // holds 4B(0-3) flash checksum + 1B(4) checksum status (!=0 -checksum read failed)  

	// local relay and switch states	
	uint8_t vi_byte[NR_VITAL_INPUT/8U];				// packed binary vital input from cpu input or FPGA 2oo3 logic
	uint8_t vo_byte[NR_VITAL_OUTPUT/8U];   			// packed binary vital out from interlock or 2oo3 logic	
	uint8_t lo_byte[NR_LOGIC_RELAYS/8U];   			// packed binary logical out from interlock or 2oo3 logic 	
	uint8_t nvo_byte[NR_NON_VITAL_OUTPUT/8U];		// packed binary non vital out from interlock or 2oo3 logic
	uint8_t vo_fdbk_fault_byte[NR_VITAL_OUTPUT/8U];	// packed binary vo fdbk in fault from FPGA
	uint8_t vo_fdbk_byte[NR_VITAL_OUTPUT/8U];		// packed binary vo fdbk from FPGA
	uint8_t vi_unstable_byte[NR_VITAL_INPUT/8U];		// packed binary vital input unstable from CPU1,2,3
	uint8_t switch_settings_byte[4];				// packed binary switch settings from FPGA

	// remote side relay states	
	uint8_t oe_vi_byte[NR_VITAL_INPUT/8U];   	// packed binary vital input from cpu input or FPGA 2oo3 logic
	uint8_t oe_vo_byte[NR_VITAL_OUTPUT/8U];   	// packed binary vital out from interlock or 2oo3 logic	
	uint8_t oe_lo_byte[NR_LOGIC_RELAYS/8U];   	// packed binary logical out from interlock or 2oo3 logic 	
	uint8_t oe_nvo_byte[NR_NON_VITAL_OUTPUT/8U]; // packed binary non vital out from interlock or 2oo3 logic

	// last state to detect changes
	uint8_t vi_last_byte[NR_VITAL_INPUT/8U];   		// packed binary vital input from cpu input or FPGA 2oo3 logic
	uint8_t vo_last_byte[NR_VITAL_OUTPUT/8U];   		// packed binary vital out from interlock or 2oo3 logic	
	uint8_t nvo_last_byte[NR_NON_VITAL_OUTPUT/8U]; 	// packed binary non vital out from interlock or 2oo3 logic
	uint8_t lo_last_byte[NR_LOGIC_RELAYS/8U];   		// packed binary logical out from interlock or 2oo3 logic
	uint8_t vo_last_fdbk_byte[NR_VITAL_OUTPUT/8U]; 	// packed binary feddback vital out from FPGA
	uint8_t oe_vi_last_byte[NR_VITAL_INPUT/8U];
	event_t vi[NR_VITAL_INPUT/2U];   			// vital input from cpu input or FPGA 2oo3 logic
	event_t vo[NR_VITAL_OUTPUT/2U];   			// vital out from interlock or 2oo3 logic			     
	event_t lo[NR_LOGIC_RELAYS];   				// logical out from interlock or 2oo3 logic 			     
	event_t nvo[NR_NON_VITAL_OUTPUT]; 			// non vital out from interlock or 2oo3 logic			     
	event_t vo_fdbk_fault[NR_VITAL_OUTPUT/2U]; 	// vital output feedback fault from fpga	     
	event_t vo_fdbk[NR_VITAL_OUTPUT/2U]; 		// vital output feedback from fpga	     
	event_t vi_unstable[NR_VITAL_INPUT/2U];  	// stability status of vital input from cpu
											
	event_t oe_vi[NR_VITAL_INPUT/2U];   		// vital input from cpu input or FPGA 2oo3 logic
	event_t oe_vo[NR_VITAL_OUTPUT/2U];   	// vital out from interlock or 2oo3 logic			     
	event_t oe_lo[NR_LOGIC_RELAYS];   		// logical out from interlock or 2oo3 logic 			     
	event_t oe_nvo[NR_NON_VITAL_OUTPUT]; 	// non vital out from interlock or 2oo3 logic			     
} processor_t;

extern processor_t pe[4];  //processing element 0-2 cpu1-3, 3-fpga

//system database structure
typedef struct {
	uint32_t global_ts;
	event_t addr_pkt;
	uint32_t flash_cksum;

	uint16_t secure_code_orig_com[2];
	uint16_t secure_code_ans_com[2];

	char line_state_TGT[4];
	char line_state_TCF[4];
	uint8_t com_state;
	uint16_t error;
	char oled_TCF[16];
	char oled_TGT[16];
	char oled[16];

	char line_state_TGT_c[4];
	char line_state_TCF_c[4];
	uint8_t com_state_c;
	uint16_t error_c;

	uint32_t com_cksum_err[2];
	uint32_t com_addr_mismatch_err[2];
	uint32_t com_pkt_incomplete_err[2];
	
	bool ev_pending;

	bool do_reset;
	pkt_type_e send_pkt_type;
	uint8_t reset_pkt_cnt;
	uint8_t reset_ack_pkt_cnt;
	uint8_t C_pkt_cnt;
} system_t;

//timer for display and com port database structure
typedef struct {
	uint16_t primary_com_ok;
	uint16_t secondary_com_ok;
	uint16_t com_send_pkt;
	uint16_t update_oled;
	uint16_t check_faults;
	uint16_t btn_reset;
	uint16_t wait_reset;
	uint16_t wait_reset_ack;
	uint16_t wait_C_pkt;
	uint32_t check_flash;
	uint32_t tgtr_p_fault_cnt;
	uint32_t tgtr_d_fault_cnt;
	uint32_t tcfr_p_fault_cnt;
	uint32_t tcfr_d_fault_cnt;
	uint32_t ascr_fault_cnt;
} timers_t; 

//relay event flag data structure
typedef struct {
	bool com_send_pkt;
	bool check_flash;
	bool update_oled;
	bool check_faults;
} event_flag_t;

//time format database structure
typedef struct {
	uint8_t dd;
	uint8_t mo;
	uint8_t yy;
	uint8_t hh;
	uint8_t mm;
	uint8_t ss;
} time_s;

//rx struct for each com port
typedef struct __attribute__((__packed__)) {
	uint8_t type;
	uint8_t sop; //$
	uint8_t station[17];
	uint8_t unit_address[4];
	uint8_t secure_code[2];
	uint8_t global_ts[17];
	uint8_t sys_err_code[5];
	uint8_t line_status_TGT[3]; //DL: TGT line status SL: Line status 
	uint8_t line_status_TCF[3]; //DL: TCF line status SL: Invalid
	uint8_t inrelay_state[NR_VITAL_INPUT/8U];
	uint8_t vorelay_state[NR_VITAL_OUTPUT/8U];
	uint8_t lorelay_state[NR_LOGIC_RELAYS/8U];
	uint16_t cksum;
} com_rx_struct_t;

typedef union {
	com_rx_struct_t rx;
	uint8_t bytes[sizeof(com_rx_struct_t)];
}com_rx_union_t;

//spi database structue
typedef struct {
	spi_struct_t* pSPI;
	uint8_t agent_id;
	uint8_t cmd;
	uint8_t* pargs;
	uint8_t nargs;
	uint8_t* pdata;
	uint8_t ndata;

	uint8_t* p_byte;
	uint8_t newreq;
	volatile uint8_t isbusy;
	volatile uint8_t iscpubusy;
	volatile uint8_t isfpgabusy;
	uint8_t status;
} spi_com_t;

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
	uint8_t reset_completed : 1;  //TODO DHEERAJ
	uint8_t spare : 4;
} ctrls_struct_t;

typedef union {
	ctrls_struct_t x;
	uint8_t bytes[sizeof(ctrls_struct_t)];
}ctrls_union_t;

//for relay feadback
typedef struct __attribute__((__packed__)) {
	uint8_t RLY_FDBK_TGTR_P:2;
	uint8_t RLY_FDBK_TGTR_D:2;
	uint8_t RLY_FDBK_TCFR_P:2;
	uint8_t RLY_FDBK_TCFR_D:2;
	uint8_t RLY_FDBK_ASCR:2;
	uint8_t SPARE1:6;
} rly_fdbk_struct_t;

typedef struct __attribute__((__packed__)) {
	uint8_t RLY_FAULT_TGTR_P:2;
	uint8_t RLY_FAULT_TGTR_D:2;
	uint8_t RLY_FAULT_TCFR_P:2;
	uint8_t RLY_FAULT_TCFR_D:2;
	uint8_t RLY_FAULT_ASCR:2;
	uint8_t SPARE2:6;
} rly_fdbk_fault_struct_t;

//for MISC
typedef struct __attribute__((__packed__)) {
	uint8_t NVC_PRESENT:1;
	uint8_t OC_PRESENT:1;
	uint8_t IC1_PRESENT:1;
	uint8_t IC2_PRESENT:1;
	uint8_t IC3_PRESENT:1;
	uint8_t LINE_SELECTION_MODE:2;
	uint16_t SPARE1:9;
	uint8_t LOCAL_ID:8;
	uint8_t REMOTE_ID:8;
} switch_settings_struct_t;

typedef struct {
	uint8_t ascr;
	uint8_t tgtr_d;
	uint8_t tcfr_d;
	uint8_t tgtr_p;
	uint8_t tcfr_p;
	bool isRdbkFault;
} fdbk_fault_t;

extern fdbk_fault_t fdbk_fault;
extern ctrls_union_t cpu_ctrls_u;
extern spi_com_t spi_com;
extern com_rx_union_t oe_pkt[2];

extern time_s tod;
extern cfg_t cfg;
extern timers_t tmr;
extern event_flag_t ev;
extern system_t sys;
extern uint8_t respbuf[10];
extern spi_com_t spi_com_cpu;
extern spi_com_t spi_com_fpga;
extern uint8_t oe_relay_state[OE_RELAY_STATE_WIDTH];
extern uint8_t local_addr_pwrup;
extern uint8_t remote_addr_pwrup;
extern uint8_t interlock_mode_pwrup;


void cpu_sm(void);
void update_cpu_ctrls();

uint8_t write_oe_relay_state();
uint8_t write_user_cmd_flags(uint8_t fpga_cpuid, uint8_t user_cmd_flags);
uint8_t read_vi(uint8_t fpga_cpuid);
uint8_t read_vo(uint8_t fpga_cpuid);
uint8_t read_lo(uint8_t fpga_cpuid);
uint8_t read_vo_fdbk();
uint8_t read_vi_unstable_err(uint8_t cpuid);
uint8_t read_flash_checksum(uint8_t cpuid);
uint8_t read_nvo();
uint8_t read_relay_fdbk();
uint8_t read_switch_settings(bool wait_for_response);

uint8_t read_oe_vi(uint8_t cpuid);
uint8_t read_oe_lo(uint8_t cpuid);
uint8_t read_oe_vo(uint8_t cpuid);
uint8_t read_interlock_mode(uint8_t cpuid);
uint8_t chk_spi_com_response(uint8_t fpga_cpuid);

uint8_t read_reg(spi_struct_t* pSPI, uint8_t agent_id, uint8_t* args, uint8_t nargs, uint8_t nresp_bytes);
uint8_t write_reg(spi_struct_t* pSPI, uint8_t agent_id, uint8_t* args, uint8_t nargs);

uint8_t write_cpu_ctrls(uint8_t cpuid, bool wait_for_response);
