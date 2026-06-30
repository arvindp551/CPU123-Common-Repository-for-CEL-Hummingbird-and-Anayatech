
/***************************************************************************//**
 * @file     cmds.c
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
 * void cli_showversion(void); <br>
 * void print_state_nvo(uint8_t fpga_cpuid, uint8_t nr_items, event_t* p_item); <br>
 * void print_state(uint8_t fpga_cpuid, uint8_t nr_items, event_t* p_item); <br>
 * void print_state_json(uint8_t fpga_cpuid, uint8_t nr_items, event_t* p_item); <br>
 * void cli_showstate(void); <br>
 * void cli_showstate_json(void); <br>
 * void cli_setstation(void); <br>
 * void cli_showinterlock(void); <br>
 * void cli_setaddress(void); <br>
 * void cli_settime(void); <br>
 * void cli_showtime(void); <br>
 * void cli_showtime(void); <br>
 * void cli_setdate(void); <br>
 * void cli_showdate(void); <br>
 * void cli_hangcpu(void); <br>
 * void cli_showdebug(void); <br>
 * void showdebug(void);
 ******************************************************************************/

#include "cmds.h"
#include <ctype.h>
#include "NuMicro.h"
#include <string.h>
#include <drvr_spi.h>
#include <drvr_uart.h>
#include <app.h>
#include <bsp.h>

#define CAT_APP 0

#define PRINT_PICK "  P"
#define PRINT_DROP "  D"
#define PRINT_ERR "  E"
#define PRINT_HI "  H"
#define PRINT_LO "  L"

#define ERROR_INVALID_PARAM         \
	token = my_strtok(NULL, DELIM); \
	if (*token != '\0')             \
	{                               \
		printf("Wrong Parameter");  \
		return;                     \
	}
	
static void cli_setaddress(void);
static void cli_settime(void);
static void cli_showinterlock(void);
static void cli_setstation(void);
static void cli_showstate_json(void);
static void cli_showversion(void);
static void showdebug(void);
static void print_state_nvo(uint8_t fpga_cpuid, uint8_t nr_items, event_t* p_item);
static void cli_showtime(void);
static void cli_hangcpu(void);
static void print_state(uint8_t fpga_cpuid, uint8_t nr_items, event_t* p_item);
static void print_state_json(uint8_t fpga_cpuid, uint8_t nr_items, event_t* p_item);
static void cli_setdate(void);
static void cli_showdate(void);
static void cli_showdebug(void);
static void cli_showstate(void);
extern void conv2TOD(uint32_t seconds, time_s *tod);
extern uint32_t conv2seconds(time_s *tod);

uint8_t interlock_mode;
char buf[80];
extern uint8_t g_test_hang;
extern uint8_t com_id;
time_s tod;

//char cliBuffer[16];
//char system_address[16];
//char unit_id[4];
//uint8_t length = 0;


/**
 * @brief list of command for cli
 */
cmdFormat_t cmdList[] = {
	// Misc
	{CAT_APP, "---- SYSTEM SETTINGS ----", "", "", NULL, 0},
	{CAT_APP, "Show firmware and hardware version", "", "showversion", &cli_showversion, 0 }, 
	{CAT_APP, "set stationI name", "<remote|local> <station>", "setstation", &cli_setstation, 0},
	{CAT_APP, "set equipment address", "<remote|local> <address>", "setaddr", &cli_setaddress, 0},
	{CAT_APP, "---- STATUS ----", "", "", NULL, 0},
	{CAT_APP, "Show State", "", "showstate", &cli_showstate, 0},
	{CAT_APP, "Show State in JSON", "", "showstatejson", &cli_showstate_json, 0},
	{CAT_APP, "show interlock  mode", "", "showinterlock", &cli_showinterlock, 0},
	{CAT_APP, "show debug info", "", "showdebug", &cli_showdebug, 0},
	// Date and Time
	{CAT_APP, "---- Date and Time ----", "", "", NULL, 0},
	{CAT_APP, "Set wall clock time in RTC manually", "HH MM SS", "settime", &cli_settime, 0},
	{CAT_APP, "Set date in RTC manually", "YYYY MM DD", "setdate", &cli_setdate, 0},
	{CAT_APP, "Show current RTC date", "", "showdate", &cli_showdate, 0},
	{CAT_APP, "Show current RTC time", "", "showtime", &cli_showtime, 0},
	{CAT_APP, "just for test Hang cpu", "cpu[0-3]", "hangcpu", &cli_hangcpu, 0},
};

uint8_t cmdListDepth = sizeof(cmdList) / sizeof(cmdFormat_t);
//uint8_t relay_set[40];

/**
 * @brief Displays hardware and firmware version information via CLI.
 *
 * This function prints the hardware version and firmware version details
 * to the CLI interface. Firmware version includes build date and time.
 *
 * @param [in,out] None
 *
 * @return void
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note
 * - Uses predefined macros:
 *      - HW_VERSION for hardware version
 *      - SW_VERSION for firmware version
 *      - __DATE__ and __TIME__ for build timestamp
 * - Intended to be used as a CLI command handler.
 * - ERROR_INVALID_PARAM macro is invoked (likely for CLI framework compliance).
 */

static void cli_showversion(void) {
	ERROR_INVALID_PARAM
	printf("HW Version: %s\r\n", HW_VERSION);
	printf("FW Version: %s %s %s", SW_VERSION, __DATE__, __TIME__);
}

/**
 * @brief Prints Non-Vital Output (NVO) states for a given CPU/FPGA.
 *
 * This function formats and prints the state of NVO items to the console.
 * It prefixes the output with the source identifier (CPU or FPGA) and
 * then prints each item's state using predefined string representations.
 *
 * @param [in] fpga_cpuid CPU/FPGA identifier (0–2 for CPUs, 3 for FPGA).
 * @param [in] nr_items Number of NVO items to print.
 * @param [in] p_item Pointer to array of event structures containing NVO states.
 *
 * @return void
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note
 * - Uses global buffer `buf` to construct the output string.
 * - Output prefix:
 *      - "C1:", "C2:", "C3:" for CPUs
 *      - "F: " for FPGA (cpuid = 3)
 * - State representation:
 *      - PRINT_HI  → state = 1
 *      - PRINT_LO  → state = 0
 *      - PRINT_ERR → any other state
 * - Assumes buffer size is sufficient for formatted output.
 */

static void print_state_nvo(uint8_t fpga_cpuid, uint8_t nr_items, event_t* p_item) {
		uint8_t j = 0;
	  uint8_t k;
	  event_t *s;

		if (fpga_cpuid == 3U) {
			j += sprintf((buf + j), "F: ");
		}
		else {
			j += sprintf((buf + j), "C%d:", fpga_cpuid + 1);
		}

		for (k = 0; k < nr_items; k++) {
			s = &p_item[k];
			j += sprintf((buf + j),"%s",(s->state==1) ? PRINT_HI: (s->state==0) ? PRINT_LO:PRINT_ERR);
		}
		printf("%s\r\n", buf);
}

/**
 * @brief Prints relay state information for a given CPU/FPGA.
 *
 * This function formats and prints the state of relay/event items to the console.
 * It prefixes the output with the source identifier (CPU or FPGA) and prints
 * each relay state using predefined string representations.
 *
 * @param [in] fpga_cpuid CPU/FPGA identifier (0–2 for CPUs, 3 for FPGA).
 * @param [in] nr_items Number of relay/event items to print.
 * @param [in] p_item Pointer to array of event structures containing relay states.
 *
 * @return void
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note
 * - Uses global buffer `buf` to construct the output string.
 * - Output prefix:
 *      - "C1:", "C2:", "C3:" for CPUs
 *      - "F: " for FPGA (cpuid = 3)
 * - State representation:
 *      - PRINT_PICK → state = 1 (energized/picked)
 *      - PRINT_DROP → state = 0 (de-energized/dropped)
 *      - PRINT_ERR  → any invalid/unknown state
 * - Assumes buffer size is sufficient for formatted output.
 */

static void print_state(uint8_t fpga_cpuid, uint8_t nr_items, event_t* p_item) {
		uint8_t j = 0;
	  uint8_t k;
	  event_t *s;

		if (fpga_cpuid == 3U){
			j += sprintf((buf + j), "F: ");
		}
		else{ 
			j += sprintf((buf + j), "C%d:", fpga_cpuid + 1);
		}

		for (k = 0; k < nr_items; k++) {
			s = &p_item[k];
			j += sprintf((buf + j),"%s",(s->state==1) ? PRINT_PICK:
											            (s->state==0) ? PRINT_DROP:
																	PRINT_ERR);
		}
		printf("%s\r\n", buf);
}

/**
 * @brief Prints relay/event states in JSON-like format for a given CPU/FPGA.
 *
 * This function formats the state of event items into a JSON-like structure
 * and prints it to the console. Each item is represented as a key-value pair
 * where the key is the index and the value indicates the state.
 *
 * @param [in] fpga_cpuid CPU/FPGA identifier (0–2 for CPUs, 3 for FPGA).
 * @param [in] nr_items Number of event items to print.
 * @param [in] p_item Pointer to array of event structures containing states.
 *
 * @return void
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note
 * - Uses global buffer `buf` to construct the output string.
 * - Output format example:
 *      C1:{
 *        0:1,
 *        1:2,
 *        ...
 *      },
 * - Prefix:
 *      - "C1:", "C2:", "C3:" for CPUs
 *      - "F:" for FPGA
 * - State encoding:
 *      - 2 → state = 1
 *      - 1 → state = 0
 *      - 3 → invalid/error state
 * - Output is JSON-like but may not be strictly compliant (e.g., trailing commas).
 * - Assumes buffer size is sufficient for formatted output.
 */

static void print_state_json(uint8_t fpga_cpuid, uint8_t nr_items, event_t* p_item) {
	uint8_t j = 0;
	uint8_t k;
	event_t *s;

	if (fpga_cpuid == 3U) {
		j += sprintf((buf + j), "F:{\n");
	}
	else{ 
		j += sprintf((buf + j), "C%d:{\n", fpga_cpuid + 1);
	}

	for (k = 0; k < nr_items; k++) {
		s = &p_item[k];
		j += sprintf((buf + j),"%d:%d,\n",k,(s->state==1) ? 2: (s->state==0) ? 1: 3);
	}
	printf("%s},\n", buf);
}
uint16_t com0_count = 0;
uint16_t com1_count = 0;
uint8_t testprint = 0;

/**
 * @brief Displays comprehensive system state information via CLI.
 *
 * This function prints the current status of local and remote system elements,
 * including vital inputs/outputs, logical relays, non-vital outputs, feedback
 * signals, communication status, card presence, and flash checksum values.
 * It uses helper print functions to format relay states for CPUs and FPGA.
 *
 * @param [in] void No input parameters.
 *
 * @return void
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note
 * - Outputs are printed to CLI using printf.
 * - Covers both local (CPU0–CPU3/FPGA) and remote (other end) states.
 * - Uses global structures:
 *      - pe[] for processing elements (CPU/FPGA data)
 *      - sys for system-level status
 *      - cfg for configuration data
 * - Uses helper functions:
 *      - print_state()
 *      - print_state_nvo()
 * - Displays:
 *      - Vital Inputs/Outputs (local & remote)
 *      - Logical relay states
 *      - Output feedback status
 *      - Non-vital outputs
 *      - Card presence (Input, Output, Non-vital cards)
 *      - Communication link status (COM0, COM1)
 *      - Flash checksum comparison (expected vs last read)
 * - Updates debug counters (com0_count, com1_count) when communication is down.
 * - Intended for debugging, diagnostics, and monitoring through CLI.
 */

 static void cli_showstate(void)
{
	uint8_t i, k;
	switch_settings_struct_t* ss;

	testprint = 1;

	//******
	printf("Local Side Vital Input \r\n");
	printf("   ");
	for (k = 0; k < (NR_VITAL_INPUT / 2); k++) {
		printf(" %2d", k + 1U);
	}
	printf("\r\n");
	for (i = 0U; i < 4U; i++) {
		print_state(i,(NR_VITAL_INPUT/2),pe[i].vi);
	}
	//******
	printf("\r\n");
	printf("Local Side Vital Output\r\n");
	printf("   ");
	for (k = 0; k < (NR_VITAL_OUTPUT / 2); k++) {
		printf(" %2d", k + 1U);
	}
	printf("\r\n");
	for (i = 0U; i < 4U; i++) {
		print_state(i,(NR_VITAL_OUTPUT/2),pe[i].vo);
	}	
	//******
	printf("\r\n");
	printf("Vital Output Feedback\r\n");
	printf("   ");
	for (k = 0; k < (NR_VITAL_OUTPUT / 2); k++) {
		printf(" %2d", k + 1U);
	}
	printf("\r\n");
	print_state(3U,(NR_VITAL_OUTPUT/2),pe[3].vo_fdbk);

	//******
	printf("\r\n");
	printf("Non Vital Output\r\n");
	printf("   ");
	for (k = 0; k < (NR_NON_VITAL_OUTPUT); k++) {
		printf(" %2d", k + 1U);
	}
	printf("\r\n");
	print_state_nvo(3U,(NR_NON_VITAL_OUTPUT),pe[3].nvo);

	//******
	printf("\r\n");
	printf("Logical Relays \r\n");
	printf("1:TGTXR 2:TGTYR 3:TCFRY 4:TGTZR 5:TCFXR 6:ASGNCPR 7:TGTYP 8:UNR 9:EGNR 10:TCFRY_1 11:CAR \r\n");
	printf("12:BTSR 13:FR1 14:FR2 15:TAR2 16:TAR1 17:EJ120 18:JPR120 19:TCFZR 20:TGTPR 21:TCFCR \r\n \r\n");
	printf("   ");
	for (k = 0; k < ((NR_LOGIC_RELAYS - 6)/2) ; k++)
	{ // exclude spare (6)
		printf(" %2d", k + 1U);
	}
	printf("\r\n");
	for (i = 0U; i < 4U; i++) {
		print_state(i,(NR_LOGIC_RELAYS - 6U)/2,pe[i].lo);
	}


	/********* REMOTE SIDE ***************/
	printf("\r\n");
	printf("Remote Side Vital Input\r\n");
	printf("   ");
	for (k = 0; k < (NR_VITAL_INPUT / 2); k++)
	{
		// State - PICK|DROP, Error_Code 0x0(no error) else error
		printf(" %2d", k + 1U);
	}
	printf("\r\n");
	for (i = 0U; i < 3U; i++) {
		print_state(i,(NR_VITAL_INPUT/2),pe[i].oe_vi);
  }
	//******
	printf("\r\n");
	printf("Remote Side Logical Relay\r\n");
	printf("   ");
	for (k = 0; k < ((NR_LOGIC_RELAYS-6)/2); k++)
	{
		// State - PICK|DROP, Error_Code 0x0(no error) else error
		printf(" %2d", k + 1U);
	}
	printf("\r\n");
	for (i = 0U; i < 3U; i++) { 
		print_state(i,(NR_LOGIC_RELAYS-6U)/2,pe[i].oe_lo);
	}
	//******
	printf("\r\n");
	printf("Remote Side Vital Output\r\n");
	printf("   ");
	for (k = 0; k < (NR_VITAL_OUTPUT / 2); k++) {
		// State - PICK|DROP, Error_Code 0x0(no error) else error
		printf(" %2d", k + 1U);
	}
	printf("\r\n");
	for (i = 0U; i < 3U; i++) {
		print_state(i,(NR_VITAL_OUTPUT/2),pe[i].oe_vo);
	}


	ss = (switch_settings_struct_t*)(pe[3].switch_settings_byte); 
	printf("\r\nInput Card Plugged:");
	printf(" -- #%d  %s", 1, (ss->IC1_PRESENT == 0) ? "Yes" : "No");
	printf(" -- #%d  %s", 2, (ss->IC2_PRESENT == 0) ? "Yes" : "No");
	printf(" -- #%d  %s", 3, (ss->IC3_PRESENT == 0) ? "Yes" : "No");

	printf("\r\nOutput Card Plugged:");
	printf(" -- #%d  %s", 4, ss->OC_PRESENT ? "Yes" : "No");

	printf("\r\nNON VITAL Card Plugged:");
	printf(" -- #%d  %s\r\n", 5, ss->NVC_PRESENT ? "Yes" : "No");

	printf("\r\n");
	if(sys.com_state & 0x1U){ 
		printf("COM0 UP!  ");
	}
	else{
		com0_count++;
		printf("COM0 DOWN!  ");
	}
	if(sys.com_state & 0x2U){ 
		printf("COM1 UP!\r\n");
	}
	else{
		com1_count++;
		printf("COM1 DOWN!\r\n");
	}
	printf("com0 down counting-> %d  com1 down counting-> %d \r\n",com0_count,com1_count);
	
	printf("\r\n");

	printf("FLASH CHECKSUM: Expected(LAST READ)\r\n");

	printf(" CPU0: 0x%08x (0x%08x)\r\n", cfg.my_flash_cksum, sys.flash_cksum);
	for (i = 0U; i < 3U; i++){
	   printf(" CPU%d: 0x", i+1U);
	   for (k = 0U; k < 4U; k++) {
	   	printf("%02x",cfg.flash_checksum[i][k]);
		 }
	   printf(" (0x");
	   for (k = 0U; k < 4U; k++) {
	   	printf("%02x",pe[i].flash_checksum[k]);
		 }
	   printf(")\r\n");
	}
}

/**
 * @brief Displays system state information in JSON format via CLI.
 *
 * This function prints a structured JSON representation of the system state,
 * including local and remote relay states, feedback signals, card status,
 * communication status, and flash checksum details. It is primarily intended
 * for machine-readable diagnostics and external system integration.
 *
 * @param [in] void No input parameters.
 *
 * @return void
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note
 * - Output is printed using printf in JSON-like format.
 * - Covers:
 *      - Local side:
 *          - Vital Inputs (vi)
 *          - Vital Outputs (vo)
 *          - Vital Output Feedback (vo_fdbk)
 *          - Non-Vital Outputs (nvo)
 *          - Logical Outputs (lo)
 *      - Remote side:
 *          - Vital Inputs (oe_vi)
 *          - Logical Outputs (oe_lo)
 *          - Vital Outputs (oe_vo)
 *      - Card presence status (IC1, IC2, IC3, OC, NVC)
 *      - Communication status (COM1, COM2)
 *      - Stored flash checksum (cfg)
 *      - Last read flash checksum (runtime values)
 * - Uses helper function print_state_json() for structured relay output.
 * - Relies on global structures:
 *      - pe[] for processing elements
 *      - sys for system-level data
 *      - cfg for configuration data
 * - Output format is JSON-like but may include trailing commas (not strict JSON).
 * - Useful for debugging, logging, or interfacing with external monitoring tools.
 */

static void cli_showstate_json(void) {
	uint8_t i, k;
	switch_settings_struct_t* ss;

	printf("{\n");
	//******
	printf("vi:{\n");
	for (i = 0U; i < 4U; i++) {
		print_state_json(i,(NR_VITAL_INPUT/2),pe[i].vi);
	}
	printf("},\n");

	//******
	printf("vo:{\n");
	for (i = 0U; i < 4U; i++) {
		print_state_json(i,(NR_VITAL_OUTPUT/2),pe[i].vo);
	}
	printf("},\n");

	//******
	printf("vo_fdbk:{\n");
	
	print_state_json(3,(NR_VITAL_OUTPUT/2),pe[3].vo_fdbk);
	printf("},\n");

	//******
	printf("nvo:{\n");
	print_state_json(3,(NR_NON_VITAL_OUTPUT),pe[3].nvo);
	printf("},\n");

	//******
	printf("lo:{\n");
	for (i = 0U; i < 4U; i++) {
		print_state_json(i,(NR_LOGIC_RELAYS - 6U)/2,pe[i].lo);
	}
	printf("},\n");


	printf("oe_vi:{\n");
	/********* REMOTE SIDE ***************/
	for (i = 0U; i < 3U; i++) {
		print_state_json(i,(NR_VITAL_INPUT/2),pe[i].oe_vi);
	}
	printf("},\n");

	printf("oe_lo:{\n");
	for (i = 0U; i < 3U; i++) {
		print_state_json(i,(NR_LOGIC_RELAYS-6U)/2,pe[i].oe_lo);
	}
	printf("},\n");

	printf("oe_vo:{\n");
	for (i = 0U; i < 3U; i++) { 
		print_state_json(i,(NR_VITAL_OUTPUT/2),pe[i].oe_vo);
	}
	printf("},\n");

	ss = (switch_settings_struct_t*)(pe[3].switch_settings_byte); 
	printf("card_status:{\n");
	printf("IC1:%d,\n",!ss->IC1_PRESENT);
	printf("IC2:%d,\n",!ss->IC2_PRESENT);
	printf("IC3:%d,\n",!ss->IC3_PRESENT);
	printf("OC:%d,\n",!ss->OC_PRESENT);
	printf("NVC:%d,\n",!ss->NVC_PRESENT);
	printf("},\n");

	printf("COM:{\n");
	printf("1:%d,\n",(sys.com_state & 0x1U)? 1:0);
	printf("2:%d\n", (sys.com_state & 0x2U)? 1:0);
	printf("},\n");

	printf("stored_chksum:{\n");
	for (i = 0U; i < 3U; i++){
	   printf("CPU%d: 0x", i+1U);
	   for (k = 0U; k < 4U; k++) {
	   	printf("%02x",cfg.flash_checksum[i][k]);
		 }
	   printf(",\n");
	}
	printf("CPU0: 0x");
	printf("%02x,\n",cfg.my_flash_cksum);
	printf("},\n");

	printf("last_chksum:{\n");
	for (i = 0U; i < 3U; i++){
	   printf("CPU%d: 0x", i+1U);
	   for (k = 0U; k < 4U; k++) {
	   	printf("%02x",pe[i].flash_checksum[k]);
		 }
	   printf(",\n");
	}
	printf("CPU0:0x");
	printf("%02x,\n",sys.flash_cksum);
	printf("}\n");

	printf("}\n");
}

/**
 * @brief Sets the local or remote station ID via CLI command.
 *
 * This function parses CLI input tokens to update either the local
 * station ID or the remote (other-end) station ID. It validates the
 * input length and updates the corresponding configuration fields.
 *
 * @param [in] void No direct parameters. Input is taken from global CLI token parsing.
 *
 * @return void
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note
 * - Uses my_strtok() to parse CLI arguments.
 * - Expected command format:
 *      - setstation local <station_id>
 *      - setstation remote <station_id>
 * - Station ID must be between 1 to 16 characters.
 * - Updates:
 *      - cfg.stationID for local station
 *      - cfg.oe_station for remote station
 * - Sets sys.addr_pkt.ev_pending when local station ID is updated.
 * - Sets ERROR_CODE to WRONG_FORMAT on invalid input or length violation.
 * - Prints error message for invalid parameters or oversized station ID.
 */

static void cli_setstation(void) {
	uint8_t k;

	token = my_strtok(NULL, DELIM);
	k = strlen(token);
	if (!strcmp(token, "local")) {
		token = my_strtok(NULL, DELIM);
		k = strlen(token);
		if (k <= 16U) {
			strcpy(cfg.stationID, token);
			sys.addr_pkt.ev_pending = 1;
		}
		else {
			printf("Station ID should be 1-16 characters\r\n");
			ERROR_CODE = WRONG_FORMAT;
		}
	}
	else if (!strcmp(token, "remote")) {
		token = my_strtok(NULL, DELIM);
		k = strlen(token);
		if (k <= 16U) {
			strcpy(cfg.oe_station, token);
		}
		else {
			printf("Station ID should be 1-16 characters\r\n");
			ERROR_CODE = WRONG_FORMAT;
		}
	}
	else {
		printf("Invalid Parameter\r\n");
		ERROR_CODE = WRONG_FORMAT;
	}
}

/**
 * @brief Displays the current interlock mode via CLI.
 *
 * This function checks the current interlock mode from the CPU control
 * structure and prints its corresponding mode name to the CLI output.
 *
 * @param [in] void No input parameters.
 *
 * @return void
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note
 * - Reads interlock mode from cpu_ctrls_u.x.interlock_mode.
 * - Possible output values:
 *      - "DOUBLE_LINE"  → INTERLOCK_MODE_DOUBLE_LINE
 *      - "SINGLE_LINE"  → INTERLOCK_MODE_SINGLE_LINE
 *      - "MUX"          → INTERLOCK_MODE_MUX
 *      - "INVALID MODE" → Unknown/unsupported mode
 * - Intended for diagnostic and monitoring purposes via CLI.
 */

static void cli_showinterlock(void) {
		if (cpu_ctrls_u.x.interlock_mode == INTERLOCK_MODE_DOUBLE_LINE){
			printf("DOUBLE_LINE\r\n");
		}
		else if (cpu_ctrls_u.x.interlock_mode == INTERLOCK_MODE_SINGLE_LINE){
			printf("SINGLE_LINE\r\n");
		}
		else if (cpu_ctrls_u.x.interlock_mode == INTERLOCK_MODE_MUX){
			printf("MUX\r\n");
		}
		else {
			printf("INVALID MODE\r\n");
		}
}

static void cli_setaddress(void) {
	uint8_t k;

	token = my_strtok(NULL, DELIM);
	k = strlen(token);
	if (!strcmp(token, "local"))
	{
		token = my_strtok(NULL, DELIM);
		k = strlen(token);
		if (k <= 4U)
		{
			strcpy(cfg.unit_address, token);
			sys.addr_pkt.ev_pending = 1;
		}
		else
		{
			printf("Unit Address should be 1-4 characters\r\n");
			ERROR_CODE = WRONG_FORMAT;
		}
	}
	else if (!strcmp(token, "remote"))
	{
		token = my_strtok(NULL, DELIM);
		k = strlen(token);
		if (k <= 4U)
		{
			strcpy(cfg.oe_unit_address, token);
		}
		else
		{
			printf("Unit Address should be 1-4 characters\r\n");
			ERROR_CODE = WRONG_FORMAT;
		}
	}
	else
	{
		printf("Invalid Parameter\r\n");
		ERROR_CODE = WRONG_FORMAT;
	}
}

/**
 * @brief Sets system time (HH:MM:SS) via CLI command.
 *
 * This function parses CLI input tokens for hours, minutes, and seconds,
 * validates the time format, and updates the system timestamp accordingly.
 * The provided time is converted into seconds and stored in the global system time.
 *
 * @param [in] void No direct parameters. Input is taken from CLI token parsing.
 *
 * @return void
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note
 * - Expected command format:
 *      - settime <hh> <mm> <ss>
 * - Uses my_strtok() to extract time components from CLI input.
 * - Valid ranges:
 *      - Hours   : 0–23
 *      - Minutes : 0–59
 *      - Seconds : 0–59
 * - On valid input:
 *      - Updates tod structure (hh, mm, ss)
 *      - Converts time to seconds using conv2seconds()
 *      - Stores result in sys.global_ts
 * - On invalid input:
 *      - Prints "Time format error"
 *      - Sets ERROR_CODE to WRONG_FORMAT
 * - ERROR_INVALID_PARAM macro is used for initial validation handling.
 */

static void cli_settime(void) {
	char str[] = "SETTIME ";
	uint16_t tmp;
	uint8_t k;
	char *token_hh, *token_mm, *token_ss;

	token_hh = my_strtok(NULL, DELIM); // hh
	token_mm = my_strtok(NULL, DELIM); // mm
	token_ss = my_strtok(NULL, DELIM); // ss
	ERROR_INVALID_PARAM
	tmp = toint(token_hh);
	if (tmp > 23U) {
		printf("Time format error");
		ERROR_CODE = WRONG_FORMAT;
		return;
	}
	tmp = toint(token_mm);
	if (tmp > 59U) {
		printf("Time format error");
		ERROR_CODE = WRONG_FORMAT;

		return;
	}
	tmp = toint(token_ss);
	if (tmp > 59U) {
		printf("Time format error");
		ERROR_CODE = WRONG_FORMAT;
		return;
	}
	tod.hh = toint(token_hh);
	tod.mm = toint(token_mm);
	tod.ss = toint(token_ss);
	sys.global_ts = conv2seconds(&tod);
}

/**
 * @brief Displays the current system time via CLI.
 *
 * This function converts the global system timestamp (in seconds)
 * into a time-of-day (HH:MM:SS) format and prints it to the CLI.
 *
 * @param [in] void No input parameters.
 *
 * @return void
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note
 * - Uses conv2TOD() to convert sys.global_ts into a time_s structure.
 * - Prints time in HH:MM:SS format using printf.
 * - Uses ERROR_INVALID_PARAM macro for CLI validation handling.
 * - Intended for monitoring current system time via CLI.
 */

static void cli_showtime(void) {
	ERROR_INVALID_PARAM
	time_s tod;
	conv2TOD(sys.global_ts, &tod);
	printf("%02d:", tod.hh);
	printf("%02d:", tod.mm);
	printf("%02d", tod.ss);
}

/**
 * @brief Sets the system date (YY:MM:DD) via CLI command.
 *
 * This function parses CLI input tokens for year, month, and day,
 * validates the date format, and updates the system timestamp accordingly.
 * The provided date is converted into seconds and stored in the global system time.
 *
 * @param [in] void No direct parameters. Input is taken from CLI token parsing.
 *
 * @return void
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note
 * - Expected command format:
 *      - setdate <yy> <mm> <dd>
 * - Uses my_strtok() to extract date components from CLI input.
 * - Valid ranges:
 *      - Day   : 1–31
 *      - Month : 1–12
 *      - Year  : 2024–2080
 * - Internally stores year as offset from 2000 (yy = input_year - 2000).
 * - On valid input:
 *      - Updates tod structure (yy, mo, dd)
 *      - Converts date to seconds using conv2seconds()
 *      - Stores result in sys.global_ts
 * - On invalid input:
 *      - Prints "Date format error"
 *      - Sets ERROR_CODE to WRONG_FORMAT
 * - Uses ERROR_INVALID_PARAM macro for initial validation handling.
 */

static void cli_setdate(void) {
	uint8_t k;
	uint16_t tmp;
	// uart_struct_t* handle = &mcu_uart;
	char str[] = "SETDATE ";
	char *token_dd, *token_mm, *token_yy;
	token_yy = my_strtok(NULL, DELIM); // yy
	token_mm = my_strtok(NULL, DELIM); // mm
	token_dd = my_strtok(NULL, DELIM); // dd
	ERROR_INVALID_PARAM

	tmp = toint(token_dd);
	if ((tmp > 31U) || (tmp < 1U)) {
		printf("Date format error");
		ERROR_CODE = WRONG_FORMAT;
		return;
	}
	tmp = toint(token_mm);
	if ((tmp > 12U) || (tmp < 1U)) {
		printf("Date format error");
		ERROR_CODE = WRONG_FORMAT;
		return;
	}
	tmp = toint(token_yy);
	if ((tmp < 2024U) || (tmp > 2080U)) {
		printf("Date format error");
		ERROR_CODE = WRONG_FORMAT;
		return;
	}
	tod.yy = toint(token_yy) - 2000U;
	tod.mo = toint(token_mm);
	tod.dd = toint(token_dd);
	sys.global_ts = conv2seconds(&tod);
}

/**
 * @brief Displays the current system date and time via CLI.
 *
 * This function converts the global system timestamp (in seconds)
 * into date and time format (DD/MM/YY HH:MM:SS) and prints it to the CLI.
 *
 * @param [in] void No input parameters.
 *
 * @return void
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note
 * - Uses conv2TOD() to convert sys.global_ts into a time_s structure.
 * - Prints date and time in the format: DD/MM/YY HH:MM:SS.
 * - Uses ERROR_INVALID_PARAM macro for CLI validation handling.
 * - Intended for monitoring current system date and time via CLI.
 */

static void cli_showdate(void) {
	ERROR_INVALID_PARAM
	time_s tod;
	conv2TOD(sys.global_ts, &tod);
	printf("%02d/%02d/%02d %02d:%02d:%02d", tod.dd, tod.mo, tod.yy, tod.hh, tod.mm, tod.ss);
}

/**
 * @brief Simulates CPU hang condition via CLI command.
 *
 * This function parses CLI input to select a specific CPU (cpu0–cpu3)
 * and sets the corresponding bit in the global g_test_hang variable
 * to simulate a hang condition for testing purposes.
 *
 * @param [in] void No direct parameters. Input is taken from CLI token parsing.
 *
 * @return void
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note
 * - Expected command format:
 *      - hangcpu <cpu_id>
 *      - Example: hangcpu cpu1
 * - Supported CPU identifiers:
 *      - "cpu0" → sets bit 0 (g_test_hang |= 1)
 *      - "cpu1" → sets bit 1 (g_test_hang |= 2)
 *      - "cpu2" → sets bit 2 (g_test_hang |= 4)
 *      - "cpu3" → sets bit 3 (g_test_hang |= 8)
 * - On invalid input:
 *      - Prints error message
 *      - Sets ERROR_CODE to WRONG_FORMAT
 * - Used for testing fault handling and watchdog/reset mechanisms.
 */

static void cli_hangcpu(void) {
	token = my_strtok(NULL, DELIM); // cpu1/2/3
	if (!strcmp(token, "cpu0")) {
		g_test_hang |= 1U;
	}
	else if(!strcmp(token, "cpu1")) {
		g_test_hang |= 2U;
	}	
	else if(!strcmp(token, "cpu2")) {
		g_test_hang |= 4U;
	}	
	else if(!strcmp(token, "cpu3")) {
		g_test_hang |= 8U;
	}	
	else {
		printf("should be cpu0|cpu123");
		ERROR_CODE = WRONG_FORMAT;
		return;
	}
}

/**
 * @brief Displays debug information via CLI.
 *
 * This function invokes the internal debug display routine to print
 * system/debug-related information to the CLI.
 *
 * @param [in] void No input parameters.
 *
 * @return void
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note
 * - Calls showdebug() to output debug information.
 * - Intended for diagnostics and troubleshooting.
 * - Output content depends on implementation of showdebug().
 */

static void cli_showdebug(void) {
	showdebug();
}

/**
 * @brief Displays detailed system debug information.
 *
 * This function prints various internal system states including relay feedback,
 * switch settings, communication status, control flags, and error counters.
 * It is primarily used for diagnostics and debugging purposes.
 *
 * @param [in] void No input parameters.
 *
 * @return void
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note
 * - Displays raw VO feedback fault bytes and switch setting bytes.
 * - Decodes and prints relay feedback signals using rly_fdbk_struct_t.
 * - Displays switch/card presence status using switch_settings_struct_t:
 *      - IC1, IC2, IC3 (Input Cards)
 *      - OC (Output Card)
 *      - NVC (Non-Vital Card)
 * - Prints configuration details:
 *      - Interlock mode
 *      - Remote ID and Local ID
 * - Displays CPU control states:
 *      - COM0/COM1 status
 *      - Hang enable status
 *      - Flash check enable
 *      - System FAIL/OK flags
 * - Displays communication error counters:
 *      - Checksum errors
 *      - Address mismatch errors
 *      - Incomplete packet errors
 * - Resets communication error counters after displaying.
 * - Intended for internal debugging and troubleshooting via CLI.
 */

static void showdebug(void) {
	uint8_t i, k;
	switch_settings_struct_t* ss;
	rly_fdbk_struct_t* rfdbk;


   printf("\r\nVO FEEDBACK\r\n");
   for(i=0U;i<4U;i++) {
	printf(" %02x",pe[3].vo_fdbk_fault_byte[i]);
   }

   printf("\r\nSWITCH SETTINGS\r\n");
   for(i=0U;i<4U;i++) {
	printf(" %02x",pe[3].switch_settings_byte[i]);
   }

	printf("\r\n");
	printf("\r\n");
	rfdbk = (rly_fdbk_struct_t*)(pe[3].vo_fdbk_fault_byte);	
	printf(" -- ASCR FDBK: %d\r\n",rfdbk->RLY_FDBK_ASCR);
	printf(" -- TGTR_P FDBK: %d\r\n",rfdbk->RLY_FDBK_TGTR_P);
	printf(" -- TGTR_D FDBK: %d\r\n",rfdbk->RLY_FDBK_TGTR_D);
	printf(" -- TCFR_P FDBK: %d\r\n",rfdbk->RLY_FDBK_TCFR_P);
	printf(" -- TCFR_D FDBK: %d\r\n",rfdbk->RLY_FDBK_TCFR_D);


	printf("\r\n");
	ss = (switch_settings_struct_t*)(pe[3].switch_settings_byte); 
	printf(" -- IC1 Present :%c\r\n", (ss->IC1_PRESENT == 0)? 'P':'A');
	printf(" -- IC2 Present :%c\r\n", (ss->IC2_PRESENT == 0)? 'P':'A');
	printf(" -- IC3 Present :%c\r\n", (ss->IC3_PRESENT == 0)? 'P':'A');
	printf(" -- OC  Present :%c\r\n", (ss->OC_PRESENT  == 0)? 'P':'A');
	printf(" -- NVC Present :%c\r\n", (ss->NVC_PRESENT == 0)? 'P':'A');
	printf(" -- INTERLOCK :%d\r\n", ss->LINE_SELECTION_MODE);
	printf(" -- RemoteID  :0x%02x\r\n", ss->REMOTE_ID);
	printf(" -- LocalID   :0x%02x\r\n", ss->LOCAL_ID);

	printf("COM0:%d",cpu_ctrls_u.x.com0_state);
	printf("COM1:%d",cpu_ctrls_u.x.com1_state);
	printf("HANG:%d",cpu_ctrls_u.x.enable_hang);
	printf("INTERLOCK:%d",cpu_ctrls_u.x.interlock_mode);
	printf("FLASH_CHECK:%d",cpu_ctrls_u.x.run_flash_check);
	printf("FAIL:%d",cpu_ctrls_u.x.SSBPAC_FAIL);
	printf("OK:%d\r\n",cpu_ctrls_u.x.SSBPAC_OK);
	printf("CHECKSUM COUNT : %d\r\n",sys.com_cksum_err[com_id]);
	printf("MISMATCH COUNT : %d\r\n",sys.com_addr_mismatch_err[com_id]);
	printf("INCOM PKT COUNT : %d\r\n",sys.com_pkt_incomplete_err[com_id]);
	sys.com_cksum_err[com_id] = 0;
	sys.com_addr_mismatch_err[com_id] = 0;
	sys.com_pkt_incomplete_err[com_id] = 0;
}
