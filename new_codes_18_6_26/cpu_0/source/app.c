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
 * static void oled_screen_init(void); <br>
 * void cpu_sm(void); <br>
 * static void update_oled_screen(void); <br>
 * static void check_flash(uint8_t todo); <br>
 * static void schedule_events(void); <br>
 * static void cpu_read_write(void); <br>
 * static void fpga_read_write(void); <br>
 * static void send_com_pkt(void); <br>
 * static void rcv_com_pkt(void); <br>
 * static void app_extract_ts(uint8_t* buf); <br>
 * static void poll_gps(void); <br>
 * static uint8_t get_flash_checksum(uint32_t* pv); <br>
 * static void print_vi_changes(void); <br>
 * void update_cpu_ctrls(void); <br>
 * static void check_faults(void); <br>
 * static void set_ctrl_defaults(void);
 ******************************************************************************/
 
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "NuMicro.h"
#include "common_header.h"
#include "app.h"
#include "bsp.h"
#include "drvr_spi.h"
#include "uart_pkts.h"
#include "interlock.h"
#include "cli.h"
#include "SH1106.h"

#define ENABLE_DEBUG 0

cfg_t cfg;
uint8_t cpu_state = INIT_STATE;
timers_t tmr;
event_flag_t ev;
uint8_t com_id;
uint8_t buf_ts[100];
uint8_t enable_watchdog = 0;
uint8_t g_test_hang = 0; //just for test
static bool is_enabled_check_faults = false;
static int8_t state_reset_seq = -1;

ctrls_union_t cpu_ctrls_u;
volatile uint8_t reset_completed = 0;
static bool b_inReset = true;
static bool checksum_print_flag = true;
char oled_buf[16];
uint8_t fault_debug = 0;;
fdbk_fault_t fdbk_fault;
rly_fdbk_fault_struct_t* rfdbk;
uint8_t state_fault = STATE_CHECK;
uint8_t local_addr_pwrup = 0;
uint8_t remote_addr_pwrup = 0;
uint8_t interlock_mode_pwrup=0;
switch_settings_struct_t* ss;


extern char* my_strtok(char* str, char delim);
extern uint32_t conv2seconds(time_s* tod);
extern void watchdog_init();
extern void conv2TOD(uint32_t seconds, time_s *tod);
extern void spi_com_fpga_sm();
extern void spi_com_cpu_sm();
extern void send_el_pkt();
static void check_faults();
static uint8_t get_flash_checksum(uint32_t* pv);
static void poll_gps();
static void app_extract_ts(uint8_t* buf);
static void check_flash(uint8_t todo);
static void oled_screen_init();
static void update_oled_screen();
static void set_ctrl_defaults();

static void schedule_events();
static void cpu_read_write(void);
static void fpga_read_write(void);
static void send_com_pkt();
static void rcv_com_pkt();
static void check_if_reset_initiated(void);
void update_cpu_ctrls();
void app_check_fdbk_fault();
void app_reset_fdbk_fault();
#if ENABLE_DEBUG==1
static void print_vi_changes();
#endif

/**
 * @brief  Initializes the OLED display and shows startup message.
 *         Configures the SH1106 display, positions the cursor,
 *         prints a static text message, and updates the screen.
 *
 * @param  None
 *
 * @return None
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note
 * - Uses SH1106 driver for OLED control.
 * - Displays the text "Central Electronics" at position (0,32).
 * - Font used: 7x10 with white color.
 * - Screen update is required after writing to display buffer.
 * - Typically called during system initialization for user indication.
 */

static void oled_screen_init(void) {
	SH1106_Init();
	SH1106_GotoXY(0,32);
	SH1106_Puts("Central Electronics",&Font_7x10,SH1106_COLOR_WHITE);
	SH1106_UpdateScreen();
}


/**
 * @brief  CPU state machine handling initialization, self-test, and operational tasks.
 *         Controls system startup sequence, performs power-on self-test (POST),
 *         and manages continuous operation including communication, FPGA interface,
 *         event scheduling, display updates, and fault monitoring.
 *
 * @param  None
 *
 * @return None
 *
 * @details
 * Requirement ID(s) - 
 * PI_SSBPAC_SwRS_POST_001
 * PI_SSBPAC_SwRS_POST_003
 * PI_SSBPAC_SwRS_POST_004
 * PI_SSBPAC_SwRS_POST_014
 * PI_SSBPAC_SwRS_GENR_029
 *
 * @note
 * - Implements three main states: INIT_STATE, POST_STATE, and OPERATIONAL_STATE.
 * - INIT_STATE:
 *   - Initializes control defaults and OLED display.
 *   - Waits for other CPUs and FPGA to stabilize.
 * - POST_STATE:
 *   - Reads switch settings and updates control configuration.
 *   - Initiates flash checksum verification.
 *   - Clears display and transitions to operational state.
 * 	OPERATIONAL state: Stay in this state unless CPU0 is reset or re-powered and do following operations
 *   -GPS polling for time
 *   -Schedule Line COM packet transmit and flash check events  
 *   -Read CPU1,2,3 registers through CPUx SPI 
 *   -Read FPGA registers through FPGA SPI
 *   -Send Line COM packet if scheduled
 *   -Receive and process Line COM packet from remote 
 *   -Send Event Logger Packet
 */
 
static bool ev_update_oled_reset = false;
void cpu_sm(void){
	uint8_t v;

	switch(cpu_state){
		case INIT_STATE:
			cpu_state = POST_STATE;
			set_ctrl_defaults();
			oled_screen_init();
			is_enabled_check_faults = false;
			sys.send_pkt_type = PKT_TYPE_RESET;
			b_inReset = true;
			rfdbk = (rly_fdbk_fault_struct_t*)(pe[3].vo_fdbk_fault_byte);
			ss = (switch_settings_struct_t*)(pe[3].switch_settings_byte); 

			wait_ms(5000); // let CPU1,2,3 and FPGA get initialized
     
			/*
			 * Once started watchdog can't be stopped
			 * watchdog runs on 10KHz (100us) tick from internal RC oscillator
			 * watchdog timeout set to 2pow14 = 16384 ticks = 16384*0.1ms = 1.638 sec
			 */
			//watchdog_init(); 
			break;
		case POST_STATE:  // POWER ON SELF TEST
			// Read Switch settings			  
			// Read flash checksum CPU0,1,2,3

			v = read_switch_settings(true);
			if (v != 0U) {
				break;
			}
			else {
				local_addr_pwrup = ss->LOCAL_ID;
				remote_addr_pwrup = ss->REMOTE_ID;
				interlock_mode_pwrup = ss->LINE_SELECTION_MODE;
				update_cpu_ctrls();

				//do nothing
			}
			ev.check_flash = 1;
			check_flash(DO_READ_FLASH_CHECKSUM);
			// wnable is set by CPU0. Once started watchdog can't be stopped
			// watchdog runs on 10KHz (100us) tick from internal RC oscillator
			// watchdog timeout set to 2pow14 = 16384 ticks = 16384*0.1ms = 1.638 sec
			// TODO: check how much time flash checksum takes
			SH1106_Clear();
			app_reset_fdbk_fault();
			tmr.btn_reset = 1000;
			cpu_state = OPERATIONAL_STATE; 
			break;
		case OPERATIONAL_STATE:
			schedule_events();
			check_if_reset_initiated();
			poll_gps();
			schedule_events();
			cpu_read_write();
			send_com_pkt();
			rcv_com_pkt();
			send_el_pkt();
			fpga_read_write();
			check_flash(DO_FLASH_INTEGRITY_CHECK);
			update_oled_screen();
			check_faults();
			app_check_fdbk_fault();

			// Reset watchdog - You are here means not in HANG
			// Should not be called in ISR as main loop may hang
			WDT_RESET_COUNTER();

#if ENABLE_DEBUG==1
			print_vi_changes(); 
#endif

			// @Note For Testing watchdog 
			if((g_test_hang & 0x01U)) {
				while(1) {
					//do nothing
				};
			}

			break;
		default:
			//do nothing
			break;
	}
}

/**
 * @brief  Updates OLED display with current system status and interlock information.
 *         Displays line status, card presence, and system health based on current mode.
 *
 * @param  None
 *
 * @return None
 *
 * @details
 * Requirement ID(s) -
 * PI_SSBPAC_SwRS_SDESG_002
 *
 * @note
 * - Function executes only when `ev.update_oled` flag is set.
 * - Supports both Single Line and Double Line interlock modes:
 *   - Single Line: Displays overall line status.
 *   - Double Line: Displays TGT and TCF statuses separately.
 * - Displays card presence status (IC1, IC2, IC3, OC, NVC):
 *   - 'P' = Present, 'A' = Absent.
 * - Displays system health:
 *   - "SYS OK" when no error.
 *   - "SYS ERR <code>" when error exists.
 * - Clears each display line before updating to avoid artifacts.
 * - Uses SH1106 OLED driver with 7x10 font.
 * - Resets `ev.update_oled` flag after update to prevent redundant refresh.
 * - Requires `SH1106_UpdateScreen()` to apply buffer changes to display.
 */

static void update_oled_screen(void) {
	if(!ev.update_oled) {
		//do nothing
	}
	else {
		uint8_t offset = 4;
		//switch_settings_struct_t* ss;
		//ss = (switch_settings_struct_t*)(pe[3].switch_settings_byte); 
		
		if(checksum_print_flag){
			//Display CPU flash checksums on OLED
			
			//Clear line
			SH1106_GotoXY(0,offset);
			SH1106_Puts("                 ",&Font_7x10,SH1106_COLOR_WHITE);
			//Print line
			SH1106_GotoXY(0,offset);
			sprintf(oled_buf,"  CHECKSUM");
			SH1106_Puts(oled_buf, &Font_7x10, SH1106_COLOR_WHITE);

			// CPU0
			//Clear line
			SH1106_GotoXY(0,(14U+offset));
			SH1106_Puts("                ",&Font_7x10,SH1106_COLOR_WHITE);
			//Print line
			SH1106_GotoXY(0,(14U+offset));
			sprintf(oled_buf, "CPU0 %08lX", (unsigned long)sys.flash_cksum);
			SH1106_Puts(oled_buf, &Font_7x10, SH1106_COLOR_WHITE);

			/* CPU1 */
			//Clear line
			SH1106_GotoXY(0,(28U+offset));
			SH1106_Puts("                ",&Font_7x10,SH1106_COLOR_WHITE);
			//Print line
			SH1106_GotoXY(0,(28U+offset));
			sprintf(oled_buf, "CPU1 %02X%02X%02X%02X",
							pe[0].flash_checksum[3],
							pe[0].flash_checksum[2],
							pe[0].flash_checksum[1],
							pe[0].flash_checksum[0]);
			SH1106_Puts(oled_buf, &Font_7x10, SH1106_COLOR_WHITE);

			/* CPU2 */
			//Clear line
			SH1106_GotoXY(0,(42U+offset));
			SH1106_Puts("                ",&Font_7x10,SH1106_COLOR_WHITE);
			//Print line
			SH1106_GotoXY(0,(42U+offset));
			sprintf(oled_buf, "CPU2 %02X%02X%02X%02X",
							pe[1].flash_checksum[3],
							pe[1].flash_checksum[2],
							pe[1].flash_checksum[1],
							pe[1].flash_checksum[0]);
			SH1106_Puts(oled_buf, &Font_7x10, SH1106_COLOR_WHITE);

			/* CPU3 */
			//Clear line
			SH1106_GotoXY(0,(56U+offset));
			SH1106_Puts("                ",&Font_7x10,SH1106_COLOR_WHITE);
			//Print line
			SH1106_GotoXY(0,(56U+offset));
			sprintf(oled_buf, "CPU3 %02X%02X%02X%02X",
							pe[2].flash_checksum[3],
							pe[2].flash_checksum[2],
							pe[2].flash_checksum[1],
							pe[2].flash_checksum[0]);
			SH1106_Puts(oled_buf, &Font_7x10, SH1106_COLOR_WHITE);
			wait_ms(300);
			checksum_print_flag = false;
		}
		else if (state_reset_seq == 2) {
			SH1106_GotoXY(0,(0U + offset));
			SH1106_Puts("                 ",&Font_7x10,SH1106_COLOR_WHITE);
			SH1106_GotoXY(0,(14U + offset));
			SH1106_Puts("                ",&Font_7x10,SH1106_COLOR_WHITE);
			//Print system state
			SH1106_GotoXY(0,(14U + offset));
			sprintf(oled_buf,"     WAITING"); 
			SH1106_Puts(oled_buf,&Font_7x10,SH1106_COLOR_WHITE);
			SH1106_GotoXY(0,(28U + offset));
			SH1106_Puts("                ",&Font_7x10,SH1106_COLOR_WHITE);
			//Print system state
			SH1106_GotoXY(0,(28U + offset));
			sprintf(oled_buf,"   REMOTE RESET");
			SH1106_Puts(oled_buf,&Font_7x10,SH1106_COLOR_WHITE);
			SH1106_GotoXY(0,(42U + offset));
			SH1106_Puts("                ",&Font_7x10,SH1106_COLOR_WHITE);		
		}
		else if(b_inReset){
			if (tmr.btn_reset == 0) {
				SH1106_GotoXY(0,(0U + offset));
				SH1106_Puts("                 ",&Font_7x10,SH1106_COLOR_WHITE);
				SH1106_GotoXY(0,(14U + offset));
				SH1106_Puts("                ",&Font_7x10,SH1106_COLOR_WHITE);
				//Print system state
				SH1106_GotoXY(0,(14U + offset));
				sprintf(oled_buf,"     RELEASE"); 
				SH1106_Puts(oled_buf,&Font_7x10,SH1106_COLOR_WHITE);
				SH1106_GotoXY(0,(28U + offset));
				SH1106_Puts("                ",&Font_7x10,SH1106_COLOR_WHITE);
				//Print system state
				SH1106_GotoXY(0,(28U + offset));
				sprintf(oled_buf,"      RESET");
				SH1106_Puts(oled_buf,&Font_7x10,SH1106_COLOR_WHITE);
				SH1106_GotoXY(0,(42U + offset));
				SH1106_Puts("                ",&Font_7x10,SH1106_COLOR_WHITE);			
			}
			else {
				//isPON = false;
				// After this device goes in reset
				SH1106_GotoXY(0,(0U + offset));
				SH1106_Puts("                 ",&Font_7x10,SH1106_COLOR_WHITE);
				SH1106_GotoXY(0,(14U + offset));
				SH1106_Puts("                ",&Font_7x10,SH1106_COLOR_WHITE);
				//Print system state
				SH1106_GotoXY(0,(14U + offset));
				sprintf(oled_buf,"  SYS IS READY"); 
				SH1106_Puts(oled_buf,&Font_7x10,SH1106_COLOR_WHITE);
				SH1106_GotoXY(0,(28U + offset));
				SH1106_Puts("                ",&Font_7x10,SH1106_COLOR_WHITE);
				//Print system state
				SH1106_GotoXY(0,(28U + offset));
				sprintf(oled_buf,"    FOR RESET");
				SH1106_Puts(oled_buf,&Font_7x10,SH1106_COLOR_WHITE);
				SH1106_GotoXY(0,(42U + offset));
				SH1106_Puts("                ",&Font_7x10,SH1106_COLOR_WHITE);
			}
		}
		else { 
			if(cpu_ctrls_u.x.interlock_mode == INTERLOCK_MODE_SINGLE_LINE) {
			
			//Clear line
			SH1106_GotoXY(0,offset);
			SH1106_Puts("                 ",&Font_7x10,SH1106_COLOR_WHITE);
			//Print line state
			SH1106_GotoXY(0,offset);
			SH1106_Puts(sys.oled,&Font_7x10,SH1106_COLOR_WHITE);

			//Clear line
			SH1106_GotoXY(0,(14U+offset));
			SH1106_Puts("                ",&Font_7x10,SH1106_COLOR_WHITE);

		}
		else if(cpu_ctrls_u.x.interlock_mode == INTERLOCK_MODE_DOUBLE_LINE) {
			//Clear line
			SH1106_GotoXY(0,(0U+offset));
			SH1106_Puts("                ",&Font_7x10,SH1106_COLOR_WHITE);
			//Print TGT state
			SH1106_GotoXY(0,(0U+offset));
			SH1106_Puts(sys.oled_TGT,&Font_7x10,SH1106_COLOR_WHITE);
			//Clear line
			SH1106_GotoXY(0,(14U+offset));
			SH1106_Puts("                ",&Font_7x10,SH1106_COLOR_WHITE);
			//Print TCF state
			SH1106_GotoXY(0,(14U+offset));
			SH1106_Puts(sys.oled_TCF,&Font_7x10,SH1106_COLOR_WHITE);
		}
		
		//Clear line
		SH1106_GotoXY(0,(28U+offset));
		SH1106_Puts("                ",&Font_7x10,SH1106_COLOR_WHITE);
		//Print card state
		SH1106_GotoXY(0,(28U + offset));
		sprintf(oled_buf,"1%c 2%c 3%c O%c N%c",
				(ss->IC1_PRESENT == 0U) ? 'P':'A',
				(ss->IC2_PRESENT == 0U) ? 'P':'A',
				(ss->IC3_PRESENT == 0U) ? 'P':'A',
				(ss->OC_PRESENT  == 0U) ? 'P':'A',
				(ss->NVC_PRESENT == 0U) ? 'P':'A'
			   );
		SH1106_Puts(oled_buf,&Font_7x10,SH1106_COLOR_WHITE);

		SH1106_GotoXY(0,(42U + offset));
		SH1106_Puts("                ",&Font_7x10,SH1106_COLOR_WHITE);
		//Print system state
		SH1106_GotoXY(0,(42U + offset));
		sprintf(oled_buf,"SYS %s %04d",(sys.error == 0U) ? "OK" : "ERR", sys.error);
		SH1106_Puts(oled_buf,&Font_7x10,SH1106_COLOR_WHITE);
		
		}
		ev.update_oled = false;
		SH1106_UpdateScreen();
	}
}

/**
 * @brief  Performs flash checksum operation for all CPUs and validates integrity.
 *         Initiates checksum calculation on remote CPUs, computes local checksum,
 *         reads back results, and either stores or verifies flash integrity.
 *
 * @param  [in]  todo   Operation selector:
 *                      - DO_READ_FLASH_CHECKSUM: Store calculated checksum values.
 *                      - DO_FLASH_INTEGRITY_CHECK: Compare checksums for integrity verification.
 *
 * @return None
 *
 * @details
 * Requirement ID(s) - 
 * PI_SSBPAC_SwRS_POST_016
 *
 * @note
 * - Function executes only if `ev.check_flash` is set and SPI is not busy.
 * - Triggers remote CPUs (CPU1, CPU2, CPU3) to compute flash checksum.
 * - Computes local CPU flash checksum using `get_flash_checksum()`.
 * - Waits additional time to ensure remote checksum completion.
 * - Reads checksum values from remote CPUs.
 * - In DO_READ_FLASH_CHECKSUM mode:
 *   - Stores local and remote checksums into configuration (`cfg`).
 * - In DO_FLASH_INTEGRITY_CHECK mode:
 *   - Compares current checksum values with stored configuration.
 *   - Sets error flag (`err_rom_faulty`) if mismatch or read error detected.
 * - Flash read error is indicated by index [4] in checksum array.
 * - Blocking delay (`wait_ms`) is used; may impact real-time behavior.
 */
 
static void check_flash(uint8_t todo) {
	uint8_t i,k;
	//TODO - Keep FPGA on hold till checksum is running upto 3 Sec


	// let CPU spi get free if flash check is pending
	if(!ev.check_flash || spi_com_cpu.isbusy) {
		//do nothing
	}
	else {	
		ev.check_flash = false;

		//enable CPU1,2,3 to calculate flash checksum
		cpu_ctrls_u.x.run_flash_check = 1;
		for(i=0;i<3U;i++) {
			write_cpu_ctrls(i,true);
		}
		cpu_ctrls_u.x.run_flash_check = 0;

		//Calculate self flash checksum 
		get_flash_checksum(&sys.flash_cksum);
		//wait a second more
		wait_ms(1000);

		for(i=0;i<3U;i++) {
			read_flash_checksum(i);
		}

		if(todo == DO_READ_FLASH_CHECKSUM) { // store flash checksum in flash
			cfg.my_flash_cksum = sys.flash_cksum;

			for(i=0U;i<3U;i++) {
				for(k=0U;k<4U;k++) {
					cfg.flash_checksum[i][k] = pe[i].flash_checksum[k];
				}
			}
		}
		else if(todo == DO_FLASH_INTEGRITY_CHECK) { // compare read checksum of each CPU with checksum stored in flash
			for(i=0U;i<3U;i++) {
				pe[i].err_rom_faulty = 0;
				if(pe[i].flash_checksum[4] != 0U) { // [4]: indicates error in flash read
					pe[i].err_rom_faulty = 1;
				}

				for(k=0U;k<4U;k++) {
					if(cfg.flash_checksum[i][k] != pe[i].flash_checksum[k]) {
						pe[i].err_rom_faulty = 1;
					}
				}
			}
		}
		else{
			//do nothing
		}
	}
}

/**
 * @brief  Schedules periodic system events based on timer expirations.
 *         Sets event flags for communication, flash check, OLED update,
 *         and fault monitoring at predefined intervals.
 *
 * @param  None
 *
 * @return None
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note
 * - COM packet scheduling:
 *   - Sends one packet every 1 second based on baud rate constraints.
 * - Flash integrity check:
 *   - Scheduled at long intervals (currently 300 seconds).
 * - OLED update:
 *   - Triggered every 20 seconds.
 * - Fault checking:
 *   - Enabled only after communication (COM0 or COM1) becomes active.
 *   - Runs every 2 seconds once enabled.
 * - Uses timer counters (`tmr.*`) and event flags (`ev.*`) for scheduling.
 * - `is_enabled_check_faults` remains true once communication is detected.
 * - Macros like SEC() are used to convert time into system ticks.
 * - Designed for periodic invocation in main loop (non-blocking scheduler).
 */
 
static void schedule_events(void) {
	event_t* s;

	/** 
	 * COM packet scheduler: 
	 * packet bytes = 65 , baudrate is 1200 = 120 B/second * so packet can be sent @ half+ sec. 
	 * Keeping a packet gap one packet per sec is sent   
	 */
	if(tmr.com_send_pkt == 0U) {
		tmr.com_send_pkt = SEC(1U);
		ev.com_send_pkt = true;
	}

	/**
	 * Internal flash check scheduler. The Flash check routine is called at long gaps.  
	 */
	if(tmr.check_flash == 0U) {
		tmr.check_flash = SEC(300U);   //HOUR(24)
		ev.check_flash = true;
	}

	if(tmr.update_oled == 0U) {
		tmr.update_oled = SEC(20U);
		ev.update_oled = true;
	}

	bool b = ((cpu_ctrls_u.x.com0_state == 1U) || (cpu_ctrls_u.x.com1_state == 1U));

	if(b) { // remains true till reset or powerup
		is_enabled_check_faults = true;
	}
	else {
		//do nothing
	}

	if(is_enabled_check_faults == false) {
		tmr.check_faults = SEC(2U);
		ev.check_faults = false;
	}	
	else if(tmr.check_faults == 0U) {
		tmr.check_faults = SEC(2U);
		ev.check_faults = true;
	}
	else
	{
		//Do Nothing
	}
}


/**
 * @brief read relay information from CPU123 using SPI1
 * This function read the relay information like vital input, vital output, logical output, otherend vital input, otherend logical output and otherend vital output from CPU123 and write user command flag for mode selection com stateand SSBPAC OK not OK state on CPU123 using non blocking method.
 */
static void cpu_read_write(void){
	static uint8_t state = 0;
	static uint8_t ch = 0;
	static bool en_cpu_resp_chk = false;

	if(spi_com_cpu.isbusy) {
		// do nothing
	}
	else {
		if(en_cpu_resp_chk) {
			chk_spi_com_response(ch);
		}
		else {
			//do nothing
		}

		if(ev.check_flash) {
			// no new CPU request
		}
		else {
		 if(state == 0U){ 
		 	en_cpu_resp_chk = true; 
		 	read_vi(ch);

		 	if(ch == 2U) {
		 		ch = 0;
		 	}
		 	else {
		 		ch++;
		 	}
		 }
		 else if(state == 1U) { 
		 	en_cpu_resp_chk = true; 
		 	read_vo(ch);
		 }
		 else if(state == 2U) { 
		 	en_cpu_resp_chk = true; 
		 	read_lo(ch);
		 }
		 else if(state == 3U) {
		 	en_cpu_resp_chk = true; 
		 	read_oe_vi(ch);
		 }
		 else if(state == 4U) { 
		 	en_cpu_resp_chk = true; 
		 	read_oe_lo(ch);
		 }
		 else if(state == 5U) { 
		 	en_cpu_resp_chk = true; 
		 	read_oe_vo(ch);
		 }
		 else if(state == 6U) {
		 	en_cpu_resp_chk = true; 
		 	read_vi_unstable_err(ch);
		 }
		 else if(state == 7U) {
		 	en_cpu_resp_chk = false; 
		 	write_cpu_ctrls(ch,false);
		 }
		 else {
		 	state = 0;
		 }

		 if(state == 7U) {
		 	state = 0;
		 }
		 else {
		 	state++;
		 }
		}
	}

	spi_com_cpu_sm(); // process read/write pending request for CPU1,2,3,FPGA
}

 /**
 * @brief  Handles cyclic SPI communication with FPGA.
 *         Performs sequential read/write operations for switch settings,
 *         relay feedback, I/O states, and synchronization with remote relay states.
 *
 * @param  None
 *
 * @return None
 *
 * @details
 * Requirement ID(s) - 
 * PI_SSBPAC_SwRS_POST_018
 * PI_SSBPAC_SwRS_POST_025
 * PI_SSBPAC_SwRS_POST_026
 * PI_SSBPAC_SwRS_POST_027
 * PI_SSBPAC_SwRS_POST_031
 * PI_SSBPAC_SwRS_POST_032
 *
 * @note
 * - Operates only when FPGA SPI interface is not busy.
 * - Uses internal state machine (state: 0–7) for scheduling operations:
 *   - 0: Read switch settings
 *   - 1: Read relay feedback
 *   - 2: Read vital outputs (VO)
 *   - 3: Read logical outputs (LO)
 *   - 4: Read non-vital outputs (NVO)
 *   - 5: Read vital inputs (VI)
 *   - 6: Write other-end relay state (conditional on COM active)
 *   - 7: Mark other-end state availability
 * - Response validation is performed using `chk_spi_com_response()` when enabled.
 * - Response check is skipped on first transaction after reset.
 * - `oe_state_avl` flag indicates availability of remote (other-end) state.
 * - Write operation (state 6) executes only if COM0 or COM1 is active.
 * - Calls `spi_com_fpga_sm()` to process pending SPI transactions.
 * - Designed for periodic invocation in main loop (non-blocking communication handler).
 */
 
static void fpga_read_write(void){
	static uint8_t state = 0;
	static uint8_t ch = 0;
	static uint8_t en_fpga_resp_chk = 0;

	if(spi_com_fpga.isbusy) {
		//do nothing
	}
	else {
		if(en_fpga_resp_chk) {
			chk_spi_com_response(3); // no response check on first entry
		}
		else {
			//do nothing
		}

		if(state == 0U){ 
			en_fpga_resp_chk = 1; 
			read_switch_settings(false);
		}		
		else if(state == 1U){ 
			en_fpga_resp_chk = 1; 
			read_relay_fdbk();
		}
		else if(state == 2U){ 
			en_fpga_resp_chk = 1; 
			read_vo(3);
		}
		else if(state == 3U){ 
			en_fpga_resp_chk = 1; 
			read_lo(3);
		}
		else if(state == 4U){ 
			en_fpga_resp_chk = 1; 
			read_nvo();
		}
		else if(state == 5U){
			en_fpga_resp_chk = 1; 
			read_vi(3);
		}
		else if(state == 6U) {
			en_fpga_resp_chk = 0; 
			if((cpu_ctrls_u.x.com0_state == 1) || (cpu_ctrls_u.x.com1_state == 1)) {  
				write_oe_relay_state();
			}
			else {
	//			state = 0;
			}
		}
		else if(state == 7U) {
			cpu_ctrls_u.x.oe_state_avl = 1;
		}
		else {
			state = 0;
		}

		if(state == 7U) {
			state = 0;
		}
		else {
			state++;
		}
	}

	spi_com_fpga_sm();
}

/**
 * @brief   Send communication packet over primary and backup links.
 *
 * @param   [in,out] None
 *
 * @return  None
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note    Sends identical packets on both communication channels
 *          when transmit FIFOs are empty and event is scheduled.
 */
 
static void send_com_pkt(void) {
	static uint8_t state = 0;


	fifo_t* aftx = uart_com[PRIMARY_LINK].ptx;
	fifo_t* bftx = uart_com[BACKUP_LINK].ptx;

	// send exact copy on both the links, bftx is added during fixed error
	if(aftx->is_empty && bftx->is_empty && ev.com_send_pkt) { //primary tx fifo os empty and  com send scheduled
		LINE_COM_write_packet(aftx,PRIMARY_LINK);
		LINE_COM_write_packet(bftx,BACKUP_LINK);
		ev.com_send_pkt = false;
	}
}
 
 /**
 * @brief   Receive and process communication packets from primary and backup links.
 *
 * @param   [in,out] None
 *
 * @return  None
 *
 * @details
 * Requirement ID(s) -
 * PI_SSBPAC_SwRS_GENR_050
 *
 * @note    - Reads incoming packets from both primary and secondary communication FIFOs.
 *          - Updates communication health based on packet validity and timeout counters.
 *          - Uses software timers (tmr.primary_com_ok and tmr.secondary_com_ok)
 *            to determine link status.
 *          - Interrupts are temporarily disabled while updating shared timer variables
 *            to ensure data consistency.
 *
 *          Communication state mapping:
 *          - Both links OK       : sys.com_state = 3
 *          - Only secondary OK   : sys.com_state = 2
 *          - Only primary OK     : sys.com_state = 1
 *          - Both links NOT OK   : sys.com_state = 0
 *
 *          Also updates:
 *          - Active communication channel (com_id)
 *          - Secure code response indicators (sys.secure_code_ans_com[])
 */
 
static void rcv_com_pkt(void) {
	static uint8_t state = 0;
	uint8_t vp;
	uint8_t vs;
	uint8_t v;
	static uint8_t p_com;
	static uint8_t s_com;

	fifo_t* afrx = uart_com[PRIMARY_LINK].prx;
	fifo_t* bfrx = uart_com[BACKUP_LINK].prx;

	vp = LINE_COM_read_packet(afrx,PRIMARY_LINK);
	vs = LINE_COM_read_packet(bfrx,BACKUP_LINK);

	if(vp == 0U){
		// primary_com_ok_timer is updated in ISR also. Disable irq before updating it in main thread
		__disable_irq();
		tmr.primary_com_ok = SEC(10U);
		__enable_irq();
		p_com = OK;
	}
	else if(vp == 1U){
		if(tmr.primary_com_ok == 0U){
			p_com = NOT_OK;
		}
	}
	else
	{
		//Do Nothing
	}

	if(vs == 0U){
		// secondary_com_ok_timer is updated in ISR also. Disable irq before updating it in main thread
		__disable_irq();
		tmr.secondary_com_ok = SEC(10U);
		__enable_irq();
		s_com = OK;
	}
	else if(vs == 1U){
		if(tmr.secondary_com_ok == 0U){
			s_com = NOT_OK;
		}
	}
	else
	{
		//Do Nothing
	}


	if(p_com == OK && s_com == OK){
		sys.com_state = 3;
		com_id = 0;
		sys.secure_code_ans_com[0] = 'O';
		sys.secure_code_ans_com[1] = 'O';
	}
	else if(p_com == NOT_OK && s_com == OK){
		sys.com_state = 2;
		com_id = 0;
		//check for com1 failure error code
		sys.secure_code_ans_com[0] = 'E';
    	sys.secure_code_ans_com[1] = 'O';
	}
	else if(p_com == OK && s_com == NOT_OK){
		sys.com_state = 1;
		com_id = 1;
		//check for com0 failure error code
		sys.secure_code_ans_com[1] = 'E';
	}
	else if(p_com == NOT_OK && s_com == NOT_OK){
		sys.com_state = 0;
		com_id = 0;
		sys.secure_code_ans_com[0] = 'E';
		sys.secure_code_ans_com[1] = 'E';
	}
}

/**
 * @brief   Extract the timestamp that got through GPS, into standared time format.
 *
 * @param   [in] buf   Pointer to buffer containing NMEA sentence (e.g., $GPRMC/$GNRMC).
 *
 * @param   [in,out] None
 *
 * @return  None
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note    - Parses RMC (Recommended Minimum Navigation Information) sentence.
 *          - Supports both "$GPRMC" and "$GNRMC" formats.
 *          - Extracts time (hhmmss) and date (ddmmyy) fields.
 *          - Converts extracted values into a time structure (time_s).
 *          - Updates system global timestamp (sys.global_ts) in seconds.
 *
 *          Assumptions:
 *          - Input buffer is a valid, null-terminated NMEA string.
 *          - Tokenization is performed using my_strtok().
 *          - No validation is performed for malformed or incomplete tokens.
 */
 
static void app_extract_ts(uint8_t* buf) {
	time_s tod;
	//sprintf(rmc,"$GPRMC,%02d%02d%02d,%c,5128.956,N,00046.200,W,000.0,084,%02d%02d%02d,,003.1,A,W*6A\r\n",
	char* token;
	uint32_t v = 0;

	token = my_strtok((char*)buf,','); // GPRMC
	if(!strcmp(token,"$GPRMC")) 
	{
		v = 1;
	}
	if(!strcmp(token,"$GNRMC")) 
	{		
		v = 1;
	}

	if(v == 0U) 
	{
		return; // Not valid RMC sentence
	}

	token = my_strtok(NULL,','); // hhmmss
	v = atoi(token);
	tod.ss = v%100;
	v /=100;
	tod.mm = v%100;
	v /=100;
	tod.hh = v;

	token = my_strtok(NULL,','); // V/A
	token = my_strtok(NULL,','); //lat
	token = my_strtok(NULL,','); //N
	token = my_strtok(NULL,','); //lon
	token = my_strtok(NULL,','); //W
	token = my_strtok(NULL,','); //..
	token = my_strtok(NULL,','); //angle
	token = my_strtok(NULL,','); //ddmmyy

	v = atoi(token);
	tod.yy = v%100U;
	v /=100U;
	tod.mo = v%100U;
	v /=100U;
	tod.dd = v;

	sys.global_ts = conv2seconds(&tod);
}

/**
 * @brief   computing process of repeatedly checking and get the latest location information using UART port.
 *          This function actively request location data from global positioning system receiver at specific intervals
 *
 * @param   [in,out] None (uses global UART FIFO and buffer)
 *
 * @return  None
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note    - Reads incoming GPS data byte-by-byte from UART FIFO.
 *          - Accumulates characters into a buffer until end-of-line (EOL/CR).
 *          - On receiving complete sentence, null-terminates buffer and
 *            calls app_extract_ts() to parse timestamp.
 *          - Buffer index wraps around if it exceeds 100 bytes to avoid overflow.
 *
 *          Assumptions:
 *          - GPS data is received in NMEA sentence format.
 *          - Buffer (buf_ts) is sufficiently sized.
 *          - Function is called periodically to ensure FIFO does not overflow.
 */

static void poll_gps(void){
	fifo_t* f = uart_gps.prx;
	uint8_t ch;
	static uint8_t k=0;

	if(!f->is_empty) {
		fifo_read(f,&ch);
		buf_ts[k] = ch;
		k++;
		if (k > 100U)
		{			
			k = 0;
		}
		if (ch == EOL || ch == CR) {
			buf_ts[k] = EOS;
			app_extract_ts(buf_ts);
			k = 0;
		}
	}
}
 
/**
 * @brief   Compute flash checksum (CRC32) of APROM and return the result.
 * 			This function enable the FMC ISP interface and get the computed flash checksum value for validation and than disable the FMC ISP interface.
 *
 * @param   [out] pv   Pointer to variable where computed checksum will be stored.
 *
 * @return  uint8_t    Status of operation (currently always returns 0).
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note    - Uses FMC hardware to calculate CRC32 checksum over APROM region.
 *          - FMC ISP function is enabled before calculation and disabled after.
 *          - CODE_SIZE_ALIGNED defines the size of flash region to be checked.
 *
 *          Assumptions:
 *          - Pointer pv is valid and non-NULL.
 *          - FMC hardware and clock are properly initialized.
 *
 *          Limitation:
 *          - No error handling is implemented; function always returns 0.
 */
 
static uint8_t get_flash_checksum(uint32_t* pv) {
	FMC_Open();				/* Enable FMC ISP function */
	// Request FMC hardware to run CRC32 calculation on APROM bank 0.
	*pv = FMC_GetChkSum(FMC_APROM_BASE, CODE_SIZE_ALIGNED); //
	FMC_Close();      /* Disable FMC ISP function */

	return 0;
}

#if ENABLE_DEBUG==1
/**
 * @brief   Monitor and print changes in vital input (VI) and remote VI states.
 * 			This function print the vital input and other end vital input changes and time in second.
 *
 * @param   [in,out] None (uses global data structures for VI states and timestamp)
 *
 * @return  None
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note    - Converts global timestamp (sys.global_ts) to time-of-day format.
 *          - Compares current VI and OE_VI bytes with their previous values.
 *          - Prints changes along with index and seconds field of timestamp.
 *          - Updates last known values after detecting a change.
 *
 *          Assumptions:
 *          - Arrays pe[].vi_byte, pe[].vi_last_byte, pe[].oe_vi_byte,
 *            and pe[].oe_vi_last_byte are properly initialized.
 *          - Function is used mainly for debugging/logging purposes.
 */
 
static void print_vi_changes(void){
	time_s tod;
	conv2TOD(sys.global_ts, &tod);
	for(int idx = 0; idx < 5; idx++){
		if(pe[3].vi_byte[idx] != pe[3].vi_last_byte[idx]){
			printf(" vi %d.%02x %02d\r\n", idx,pe[3].vi_byte[idx], tod.ss);
			pe[3].vi_last_byte[idx] = pe[3].vi_byte[idx];
		}
		if(pe[0].oe_vi_byte[idx] != pe[0].oe_vi_last_byte[idx]){
			printf("oe_vi %d.%02x %02d\r\n", idx,pe[0].oe_vi_byte[idx], tod.ss);
			pe[0].oe_vi_last_byte[idx] = pe[0].oe_vi_byte[idx];
		}
	}
}
#endif

/**
 * @brief   Update CPU control structure based on switch settings and communication status.
 *
 * @param   [in,out] None (uses global configuration and system state structures)
 *
 * @return  None
 *
 * @details
 * Requirement ID(s) -
 * PI_SSBPAC_SwRS_POST_036
 *
 * @note    - Reads line selection mode from switch settings (pe[3]).
 *          - Updates configuration block mode (cfg.block_mode):
 *              'D' = Double Line
 *              'S' = Single Line
 *              'M' = Multiplex (MUX)
 *              'E' = Error/Invalid mode
 *          - Updates CPU control structure (cpu_ctrls_u):
 *              - interlock_mode from switch settings
 *              - com0_state and com1_state from system communication state (sys.com_state)
 *
 *          Assumptions:
 *          - pe[3].switch_settings_byte contains valid switch configuration data.
 *          - sys.com_state bit mapping:
 *              bit0 -> COM0 status
 *              bit1 -> COM1 status
 */

void update_cpu_ctrls(void){
	uint8_t v = 0;
	//switch_settings_struct_t* ss;

	//ss = (switch_settings_struct_t*)(pe[3].switch_settings_byte); 

	if(interlock_mode_pwrup == INTERLOCK_MODE_DOUBLE_LINE) {
		cfg.block_mode = 'D';
	}
	else if(interlock_mode_pwrup == INTERLOCK_MODE_SINGLE_LINE) {
		cfg.block_mode = 'S';
	}
	else if(interlock_mode_pwrup == INTERLOCK_MODE_MUX) {
		cfg.block_mode = 'M';
	}
	else {
		cfg.block_mode = 'E'; //Error
	}

	cpu_ctrls_u.x.interlock_mode = interlock_mode_pwrup;
	cpu_ctrls_u.x.com0_state = (sys.com_state & 0x1U) != 0U ? 1U : 0U;
	cpu_ctrls_u.x.com1_state = (sys.com_state & 0x2U) != 0U ? 1U : 0U;
	if(sys.do_reset) {
		cpu_ctrls_u.x.reset_state = 3;
		cpu_ctrls_u.x.oe_state_avl = 0;
		cpu_ctrls_u.x.SSBPAC_OK = 1;
		cpu_ctrls_u.x.SSBPAC_FAIL = 1;
	}
	else {
		cpu_ctrls_u.x.reset_state = 0;
	}
}

/**
 * @brief   Perform system fault checks and update system health status.
 *
 * @param   [in,out] None (uses global system, configuration, and event structures)
 *
 * @return  None
 *
 * @details
 * Requirement ID(s) -
 * PI_SSBPAC_SwRS_SERR_001
 * PI_SSBPAC_SwRS_SERR_002
 * PI_SSBPAC_SwRS_SERR_003
 * PI_SSBPAC_SwRS_SERR_004
 * PI_SSBPAC_SwRS_SERR_005
 * PI_SSBPAC_SwRS_SERR_006
 * PI_SSBPAC_SwRS_SERR_007
 * PI_SSBPAC_SwRS_SERR_008
 * PI_SSBPAC_SwRS_SERR_009
 * PI_SSBPAC_SwRS_SERR_010
 * PI_SSBPAC_SwRS_SERR_011
 * PI_SSBPAC_SwRS_SERR_012
 * PI_SSBPAC_SwRS_SERR_013
 * PI_SSBPAC_SwRS_SERR_014
 * PI_SSBPAC_SwRS_SERR_015
 * PI_SSBPAC_SwRS_SERR_016
 * PI_SSBPAC_SwRS_SERR_017
 * PI_SSBPAC_SwRS_SERR_018
 * PI_SSBPAC_SwRS_SERR_019
 * PI_SSBPAC_SwRS_SERR_020
 * PI_SSBPAC_SwRS_SERR_021
 * PI_SSBPAC_SwRS_SERR_022
 * PI_SSBPAC_SwRS_SERR_023
 * PI_SSBPAC_SwRS_SERR_024
 * PI_SSBPAC_SwRS_SERR_025
 * PI_SSBPAC_SwRS_SERR_026
 * PI_SSBPAC_SwRS_SERR_027
 * PI_SSBPAC_SwRS_GENR_049
 *
 * @note    - Evaluates multiple system conditions to determine operational health:
 *              1. Minimum required input cards (IC1–IC3) present
 *              2. Output card (OC) presence
 *              3. Non-vital card (NVC) presence
 *              4. Communication link status (COM)
 *              5. Local and remote address consistency (power-up vs runtime)
 *              6. Interlock mode consistency
 *              7. CPU response availability (at least two CPUs responding)
 *              8. FPGA response status
 *              9. Optional relay feedback fault checks (currently disabled)
 *
 *          - Uses a state machine:
 *              STATE_CHECK   : Evaluate all fault conditions
 *              STATE_OK      : System healthy (SSBPAC_OK = 1, FAIL = 0)
 *              STATE_OK_FAIL : Degraded mode (SSBPAC_OK = 1, FAIL = 1)
 *              STATE_FAIL    : Fail-safe state (SSBPAC_OK = 0, FAIL = 1)
 *
 *          - Once system enters STATE_FAIL, recovery requires reset sequence.
 *
 *          - Stores initial (power-up) values of:
 *              - Local address
 *              - Remote address
 *              - Interlock mode
 *            and compares them during runtime for consistency.
 *
 *          - Fault debug information is printed using printf() for diagnostics.
 *
 *          Assumptions:
 *          - pe[], sys, cfg, and cpu_ctrls_u structures are properly initialized.
 *          - Function is triggered periodically via event flag (ev.check_faults).
 */

static void check_faults(void) {
	uint8_t ic_present_cnt;
	uint8_t cpu_no_response_cnt;
	uint8_t fpga_no_response;
	static bool am_i_ready = false;

	/*
	 * All of following conditions should be OK 
	 * to keep system in normal state otherwise 
	 * system will remain in default fail safe state.
	 * 1. IC1-3 >1 cards present
	 * 2. OC  present
	 * 3. NV card  present
	 * 4. COM1 and/or COM2 up
	 * 5. Remote address should match in COM
	 * 6. Card Detect, local & remote address, interlock mode settings changed
	 * 7. Any two of CPU1,2,3 response received means not in hang state
	 * 8. Relay Feedback isn't in fault 
	 * 9. response OK from CPU1,2,3 or FPGA
	 * 
	 * Once enter into default or fail safe state, recovery can be done through a reset sequence
	 */
//#if 0

	if(!ev.check_faults) {
		//do nothing
	}
	else {	
		ev.check_faults = false;
		//ss = (switch_settings_struct_t*)(pe[3].switch_settings_byte); 

		if(!am_i_ready) {
			am_i_ready = true;
		}
		
		else {
			uint8_t ic1_p = (ss->IC1_PRESENT == 0U) ? 1 : 0;
			uint8_t ic2_p = (ss->IC2_PRESENT == 0U) ? 1 : 0;
			uint8_t ic3_p = (ss->IC3_PRESENT == 0U) ? 1 : 0;
			
			ic_present_cnt = ic1_p + ic2_p + ic3_p; 
			cpu_no_response_cnt = pe[0].err_no_response + pe[1].err_no_response + pe[2].err_no_response;
			fpga_no_response = pe[3].err_no_response;

		
			switch (state_fault) {
				case STATE_CHECK:

					if(ic_present_cnt < 2U) {
						state_fault = STATE_FAIL;
						fault_debug = 1;
						printf("IC cnt error: %d\r\n",ic_present_cnt);
					}
					else if(ss->OC_PRESENT != 0U) {
						state_fault = STATE_FAIL;
						fault_debug = 2;
						printf("OC error: %d\r\n",ss->OC_PRESENT);
					}
					else if(ss->NVC_PRESENT != 0U) {
						state_fault = STATE_FAIL;
						fault_debug = 3;
						printf("NVC error: %d\r\n",ss->NVC_PRESENT);
					}
					else if(sys.com_state == 0U) {
						state_fault = STATE_FAIL;
						fault_debug = 4;
						printf("COM error: %d\r\n",sys.com_state);
					}
					else if(local_addr_pwrup != ss->LOCAL_ID) {
						state_fault = STATE_FAIL;
						fault_debug = 5;
						printf("local addr error: @powerup->%d @now->%d\r\n",local_addr_pwrup,ss->LOCAL_ID);
					}
					else if(remote_addr_pwrup != ss->REMOTE_ID) {
						state_fault = STATE_FAIL;
						fault_debug = 6;
						printf("remote addr error: @powerup->%d @now->%d\r\n",remote_addr_pwrup,ss->REMOTE_ID);
					}
					else if(interlock_mode_pwrup != ss->LINE_SELECTION_MODE) {
						state_fault = STATE_FAIL;
						fault_debug = 7;
						printf("Interlock error: @powerup->%d @now->%d\r\n",interlock_mode_pwrup,ss->LINE_SELECTION_MODE);
					}
					#if 1
					else if(fdbk_fault.isRdbkFault){
						state_fault = STATE_FAIL;
						//fault_debug = 8;
					}
					#endif
					

					else if(ic_present_cnt == 2U) {
						state_fault = STATE_OK_FAIL;
					}
					else if((sys.com_state == 1U) || (sys.com_state == 2U)) {
						state_fault = STATE_OK_FAIL;
					}
					else if(cpu_no_response_cnt > 2U) {
						state_fault = STATE_OK_FAIL;
					}
					else if(fpga_no_response == 1U) {
						state_fault = STATE_OK_FAIL;
					}
					else {
						state_fault = STATE_OK;
					}
					break;
//#endif
				case STATE_OK:
					cpu_ctrls_u.x.SSBPAC_OK = 1;
					cpu_ctrls_u.x.SSBPAC_FAIL = 0;
					state_fault = STATE_CHECK;
					break;

				case STATE_OK_FAIL:
					cpu_ctrls_u.x.SSBPAC_OK = 1;
					cpu_ctrls_u.x.SSBPAC_FAIL = 1;
					state_fault = STATE_CHECK;
					break;
//#endif
					// remain in this state till reset sequence is initiated
				case STATE_FAIL: 
					cpu_ctrls_u.x.SSBPAC_OK = 0;
					cpu_ctrls_u.x.SSBPAC_FAIL = 1;
				    // Return to normal state after reset sequence completes
				    if(reset_completed)
				    {
				        reset_completed = 0;

				        // Clear all control outputs
				        set_ctrl_defaults();

							// Clear reset communication counters
							// sys.send_pkt_type = PKT_TYPE_RESET; // Added By Akhilesh
							  sys.reset_pkt_cnt = 0;
				        sys.reset_ack_pkt_cnt = 0;
				        sys.C_pkt_cnt = 0;

				        // System back to monitoring state
				        state_fault = STATE_CHECK;
				    }
					break;
					//#endif
				default:
					//do nothing
					break;
			}
			
		}
		
	}
	//#endif
}

/**
 * @brief   Initialize CPU control structure with default values.
 *
 * @param   [in,out] None (initializes global cpu_ctrls_u structure)
 *
 * @return  None
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note    - Resets all CPU control flags and status fields to default (safe) state.
 *          - Used during system initialization or reset.
 *
 *          Fields initialized:
 *          - run_flash_check : Disabled
 *          - enable_hang     : Disabled
 *          - com0_state      : Not active
 *          - com1_state      : Not active
 *          - interlock_mode  : 0
 *          - SSBPAC_OK       : Cleared
 *          - SSBPAC_FAIL     : Cleared
 *          - oe_state_avl    : Cleared
 *          - spare           : Cleared
 */
 
static void set_ctrl_defaults(void) {
	cpu_ctrls_u.x.run_flash_check = 0;
	cpu_ctrls_u.x.enable_hang = 0;
	cpu_ctrls_u.x.com0_state = 0U;
	cpu_ctrls_u.x.com1_state = 0U;
	cpu_ctrls_u.x.interlock_mode = 0;
	cpu_ctrls_u.x.SSBPAC_OK = 0;
	cpu_ctrls_u.x.SSBPAC_FAIL = 0;
	cpu_ctrls_u.x.oe_state_avl = 0;
	cpu_ctrls_u.x.spare = 0;
}

/**
 * @brief    Reset sequence initialization.
 *
 * @param   None 
 *
 * @return  None
 *
 * @details
 * Requirement ID(s) - 
 * PI_SSBPAC_SwRS_POST_033
 *
 * @note    - 
 *
 */

static void check_if_reset_initiated(){
	vi_dl_struct_t* p_vi;
	p_vi = (vi_dl_struct_t*)pe[3].vi_byte;
	
	switch(state_reset_seq){
		case -1:			
			cpu_ctrls_u.x.SSBPAC_OK   = 0;
			cpu_ctrls_u.x.SSBPAC_FAIL = 1;
			if(tmr.btn_reset == 0) { //means reset button in pick state
				sys.send_pkt_type = PKT_TYPE_RESET;
				sys.do_reset = 1;
				state_reset_seq = 0;
				b_inReset = true;
				cpu_ctrls_u.x.SSBPAC_OK   = 0;
				cpu_ctrls_u.x.SSBPAC_FAIL = 0;

			}
			break;
		case 0:
			if(p_vi->RST_BTN == DROP) {
				state_reset_seq = 1;
			}
			break;
		case 1:
			if(sys.reset_pkt_cnt >= 5 || (sys.reset_ack_pkt_cnt >= 5)) { // reset pkts received -> enter into reset
				sys.send_pkt_type = PKT_TYPE_ACK;
				sys.do_reset = 1;
				state_reset_seq = 2;
				sys.reset_pkt_cnt = 0;
				cpu_ctrls_u.x.SSBPAC_OK = 0;
				cpu_ctrls_u.x.SSBPAC_FAIL = 0;
			}
			break;
		case 2:
			if((sys.reset_ack_pkt_cnt >= 5) || (sys.C_pkt_cnt >= 5)) {// sufficient ack or c pkts received -> exit reset
				sys.do_reset = 0;
				sys.C_pkt_cnt = 0;
				sys.reset_ack_pkt_cnt = 0;
				sys.reset_pkt_cnt = 0;
				state_reset_seq = 3;
				b_inReset = false;
				sys.send_pkt_type = PKT_TYPE_STATUS;
			}
			break;
		case 3: 
			state_reset_seq = 5;
			sys.do_reset = 0;
			cpu_ctrls_u.x.SSBPAC_OK = 0;
			cpu_ctrls_u.x.SSBPAC_FAIL = 0;

			// Indicate reset sequence finished
			reset_completed = 1;
			break;
		case 4:
			if(sys.reset_pkt_cnt >=5) { // sufficient reset pkts rcvd -> send ack
				sys.send_pkt_type = PKT_TYPE_ACK;
				sys.reset_pkt_cnt = 0;
				state_reset_seq = 2;
				//cpu_state = POST_STATE;
			}
			if(sys.reset_ack_pkt_cnt != 0){
				state_reset_seq = 2;
			}
			else if(sys.C_pkt_cnt != 0){
				state_reset_seq = 2;
			}
			break;
		case 5:
			if(sys.reset_pkt_cnt >=5 || (cpu_ctrls_u.x.SSBPAC_OK == 0 && cpu_ctrls_u.x.SSBPAC_FAIL == 1)) { //means reset button in pick state
				sys.send_pkt_type = PKT_TYPE_RESET;
				sys.do_reset = 1;
				state_reset_seq = -1;
				b_inReset = true;
			}
			break;
	}
	// keep timer set to 10 sec if BELL and RST buttons are not pressed together 
	//if((p_vi->BPN == DROP) && (p_vi->RST_BTN == DROP)) {
	if(p_vi->RST_BTN == DROP) {
		tmr.btn_reset = SEC(5);
	}
	else {
		//do nothing
	}
	// clear historical A packet count after ---- ms 
	if(tmr.wait_reset_ack==0) {
		sys.reset_ack_pkt_cnt = 0;
	}
	else {
		//do nothing
	}
	// clear historical C packet count after ---- ms
	if(tmr.wait_C_pkt==0) {
		sys.C_pkt_cnt = 0;
	}
	else {
		//do nothing
	}
	// clear historical R packet count after ---- ms 
	if(tmr.wait_reset==0) {
		sys.reset_pkt_cnt = 0;
	}
	else {
		//do nothing
	}
}


void app_check_fdbk_fault() {
		__disable_irq();
		if(rfdbk->RLY_FAULT_ASCR == 0U) {
			fdbk_fault.ascr = 20;
		}					
		
		if(rfdbk->RLY_FAULT_TCFR_D == 0U) {
			fdbk_fault.tcfr_d = 20;
		}
		
		if(rfdbk->RLY_FAULT_TCFR_P == 0U) {
			fdbk_fault.tcfr_p = 20;
		}
		
		if(rfdbk->RLY_FAULT_TGTR_D == 0U) {
			fdbk_fault.tgtr_d = 20;
		}
		
		if(rfdbk->RLY_FAULT_TGTR_P == 0U) {
			fdbk_fault.tgtr_p = 20;
		}
		__enable_irq();
		
			if(fdbk_fault.ascr == 0) {
			fdbk_fault.isRdbkFault = true;
			fault_debug = 8;
		}
		 else if(fdbk_fault.tcfr_d == 0) {
			fdbk_fault.isRdbkFault = true;
			 fault_debug = 9;
		}
		else if(fdbk_fault.tcfr_p == 0) {
			fdbk_fault.isRdbkFault = true;
			fault_debug = 10;
		}
		else if(fdbk_fault.tgtr_d == 0) {
			fdbk_fault.isRdbkFault = true;
			fault_debug = 11;
		}	
		else if(fdbk_fault.tgtr_p == 0) {
			fdbk_fault.isRdbkFault = true;
			fault_debug = 12;
		}
		else {
			fdbk_fault.isRdbkFault = false;
		}
		
		if(fdbk_fault.isRdbkFault) {
			fdbk_fault.isRdbkFault = false;
		}
}

void app_reset_fdbk_fault() {
	rfdbk->RLY_FAULT_ASCR = 0;
	rfdbk->RLY_FAULT_TCFR_D = 0;
	rfdbk->RLY_FAULT_TCFR_P = 0;
	rfdbk->RLY_FAULT_TGTR_D = 0;
	rfdbk->RLY_FAULT_TGTR_P = 0;
	
	fdbk_fault.ascr = 100;
	fdbk_fault.tcfr_d = 100;
	fdbk_fault.tcfr_p = 100;
	fdbk_fault.tgtr_d = 100;
	fdbk_fault.tgtr_p = 100;
	fdbk_fault.isRdbkFault = false;
}
