/***************************************************************************//**
 * @file     read_cpu.c
 * @brief    All application specific source files
 * @version  1.0.0
 * @details
 * Compiler : Keil uVision
 * Target : Nuvoton M263
 * Module : Central Controller Card
 * Date Created : Jan 17, 2026
 * @copyright Copyright (C) 2026 Anaya Tech Systems Pvt. Ltd. . All rights reserved.
 * @author    SG
 * @par Functions Included
 * uint8_t read_vi(uint8_t fpga_cpuid); <br>
 * uint8_t read_vi_unstable_err(uint8_t cpuid); <br>
 * uint8_t read_vo(uint8_t fpga_cpuid); <br>
 * uint8_t read_lo(uint8_t fpga_cpuid); <br>
 * uint8_t read_flash_checksum(uint8_t cpuid); <br>
 * uint8_t read_nvo(void); <br>
 * uint8_t read_relay_fdbk(void); <br>
 * uint8_t read_switch_settings(bool wait_for_response); <br>
 * uint8_t read_oe_lo(uint8_t cpuid); <br>
 * uint8_t read_oe_vi(uint8_t cpuid); <br>
 * uint8_t read_interlock_mode(uint8_t cpuid); <br>
 * uint8_t read_oe_vo(uint8_t cpuid); <br>
 * uint8_t write_oe_relay_state(void); <br>
 * uint8_t write_cpu_ctrls(uint8_t cpuid, bool wait_for_response); <br>
 * static uint8_t read_cpu_fpga(uint8_t fpga_cpuid, uint8_t cmd, uint8_t nr_bytes, uint8_t* p_byte); <br>
 * uint8_t chk_spi_com_response(uint8_t fpga_cpuid); <br>
 * uint8_t read_reg(spi_struct_t* pSPI, uint8_t agent_id, uint8_t* args, uint8_t nargs, uint8_t nresp_bytes); <br>
 * uint8_t write_reg(spi_struct_t* pSPI, uint8_t agent_id, uint8_t* args, uint8_t nargs); <br>
 * static uint8_t run_cmd(spi_struct_t* pSPI, uint8_t agent_id, uint8_t cmd, uint8_t* pargs, uint8_t nargs, uint8_t* pdata, uint8_t ndata); <br>
 * void spi_com_cpu_sm(void); <br>
 * void spi_com_fpga_sm(void); 
 ******************************************************************************/

#include "app.h"
#include "bsp.h"
#include "uart_pkts.h"

system_t sys;
spi_com_t spi_com_cpu;
spi_com_t spi_com_fpga;
processor_t pe[4];  //processing element 0-2 cpu1-3, 3-fpga					
static uint8_t respbuf_fpga[10];
static uint8_t respbuf_cpu[10];

extern volatile uint32_t wait_time;
extern uint8_t interlock_mode;

void spi_com_cpu_sm(void);
void spi_com_fpga_sm(void);

static uint8_t run_cmd(spi_struct_t* pSPI, uint8_t agent_id, uint8_t cmd, uint8_t* pargs, uint8_t nargs, 
					   uint8_t* pdata, uint8_t ndata);

static uint8_t read_cpu_fpga(uint8_t fpga_cpuid, uint8_t cmd, uint8_t nr_bytes, uint8_t* p_byte); 


/**
 * @brief read the vital input from CPU123 and FPGA using cmd CMD_READ_INRLAY->0xA2
 * @param fpga_cpuid	fpga_cpuid is 0, 1, 2 and 3-> fPGA 
 */

/**
 * @brief   Read vital input (VI) data from CPU/FPGA.
 *
 * @param   [in]  fpga_cpuid   Identifier of target CPU/FPGA from which VI data is read.
 *
 * @param   [out] None (updates global structure pe[fpga_cpuid].vi_byte)
 *
 * @return  uint8_t   Status of read operation (as returned by read_cpu_fpga).
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note    - Sends CMD_READ_INRLY command to specified CPU/FPGA.
 *          - Reads (NR_VITAL_INPUT/8) bytes of vital input data.
 *          - Stores received data into pe[fpga_cpuid].vi_byte buffer.
 *
 *          Assumptions:
 *          - fpga_cpuid is within valid range.
 *          - pe[] structure is properly initialized.
 *          - read_cpu_fpga() handles communication and error reporting.
 */
 
uint8_t read_vi(uint8_t fpga_cpuid) {
	uint8_t status;

	status = read_cpu_fpga(fpga_cpuid, CMD_READ_INRLY,(NR_VITAL_INPUT/8U),pe[fpga_cpuid].vi_byte);
	
	return status;
}

/**
 * @brief   Read unstable vital input (VI) error status from CPU/FPGA.
 *
 * @param   [in]  cpuid   Identifier of target CPU/FPGA from which unstable vi status is read.
 *
 * @param   [out] None (updates global structure pe[cpuid].vi_unstable_byte)
 *
 * @return  uint8_t   Status of read operation (as returned by read_cpu_fpga).
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note    - Sends CMD_READ_INRLY_UNSTABLE command to specified CPU/FPGA.
 *          - Reads (NR_VITAL_INPUT/8) bytes indicating unstable VI conditions.
 *          - Stores received data into pe[cpuid].vi_unstable_byte buffer.
 *
 *          Assumptions:
 *          - cpuid is within valid range.
 *          - pe[] structure is properly initialized.
 *          - read_cpu_fpga() manages communication and error handling.
 */
 
uint8_t read_vi_unstable_err(uint8_t cpuid) {
	uint8_t status;

	status = read_cpu_fpga(cpuid, CMD_READ_INRLY_UNSTABLE,(NR_VITAL_INPUT/8U),pe[cpuid].vi_unstable_byte);
	return status;
}

/**
 * @brief   Read vital output (VO) data from CPU or FPGA.
 *
 * @param   [in]  fpga_cpuid   Identifier of target CPU/FPGA from which VO data is read.
 *
 * @param   [out] None (updates global structure pe[fpga_cpuid].vo_byte)
 *
 * @return  uint8_t   Status of read operation (as returned by read_cpu_fpga).
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note    - Selects command based on target:
 *              - CMD_READ_INTERLOCK for CPU (cpuid 0–2)
 *              - CMD_READ_VO for FPGA (cpuid 3)
 *          - Reads (NR_VITAL_OUTPUT/8) bytes of output data.
 *          - Stores received data into pe[fpga_cpuid].vo_byte buffer.
 *
 *          Assumptions:
 *          - fpga_cpuid is within valid range.
 *          - pe[] structure is properly initialized.
 *          - read_cpu_fpga() handles communication and error reporting.
 */

uint8_t read_vo(uint8_t fpga_cpuid) {
	uint8_t status;
	uint8_t cmd = CMD_READ_INTERLOCK;

	if(fpga_cpuid == 3U){
     wait_ms(1000);
		cmd = CMD_READ_VO;
		}
	status = read_cpu_fpga(fpga_cpuid, cmd,(NR_VITAL_OUTPUT/8U),pe[fpga_cpuid].vo_byte);
	return status;
}

/**
 * @brief   Read logical relay (LO) data from CPU/FPGA.
 *
 * @param   [in]  fpga_cpuid   Identifier of target CPU/FPGA from which LO data is read.
 *
 * @param   [out] None (updates global structure pe[fpga_cpuid].lo_byte)
 *
 * @return  uint8_t   Status of read operation (as returned by read_cpu_fpga).
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note    - Sends CMD_READ_LOGICAL_RELAY command to specified CPU/FPGA.
 *          - Reads (NR_LOGIC_RELAYS/8) bytes of logical relay data.
 *          - Stores received data into pe[fpga_cpuid].lo_byte buffer.
 *
 *          Assumptions:
 *          - fpga_cpuid is within valid range.
 *          - pe[] structure is properly initialized.
 *          - read_cpu_fpga() handles communication and error reporting.
 */

uint8_t read_lo(uint8_t fpga_cpuid) {
	uint8_t status;
	status = read_cpu_fpga(fpga_cpuid, CMD_READ_LOGICAL_RELAY,(NR_LOGIC_RELAYS/8U),pe[fpga_cpuid].lo_byte);
	return status;
}

/**
 * @brief  Reads flash checksum data from a specified CPU and verifies SPI communication status.
 *
 * @param  [in]  cpuid   CPU ID (0–2 for CPUs, typically used to select target device).
 *
 * @return uint8_t
 *         - STATUS_OK   : Flash checksum read successfully.
 *         - STATUS_FAIL : SPI communication failed or no response received.
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note   This is a blocking function intended for periodic health checks.
 *         It reads 5 bytes: 4-byte checksum (LSB first) + 1-byte status.
 *         The function actively waits until SPI transfer is complete.
 *         In case of communication failure, received data is cleared and
 *         error flag (err_no_response) is set for the corresponding CPU.
 */

uint8_t read_flash_checksum(uint8_t cpuid) {
	uint8_t status;
  uint8_t i;
	
	// read checksum B0,1,2,3 and last checksum run status err if !=0
	status = read_cpu_fpga(cpuid, CMD_READ_FLASH_CHECKSUM,5,(uint8_t*)pe[cpuid].flash_checksum);

	//wait for spi xfer to complete
	//this function is blocking and executed at long intervals for health check
	do {
		spi_com_cpu_sm();	
	}
	while(spi_com_cpu.isbusy);

	if(spi_com_cpu.status != STATUS_OK){ // No Response
		pe[cpuid].err_no_response = 1;
		for(i=0;i<spi_com_cpu.ndata;i++) {
			spi_com_cpu.p_byte[i] = 0x00;
		}
		status = STATUS_FAIL;
	}
	else {
		pe[cpuid].err_no_response = 0;
		for(i=0;i<spi_com_cpu.ndata;i++) {
			spi_com_cpu.p_byte[i] = spi_com_cpu.pdata[i];
		}
		status = STATUS_OK;
	}
	return status;
}

/**
 * @brief  Reads non-vital output (NVO) data from FPGA and updates debug indicators.
 *
 * @param  [in]  None
 *
 * @return uint8_t
 *         - STATUS_OK   : NVO data read successfully.
 *         - STATUS_FAIL : Communication failure while reading NVO data.
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note   This function reads non-vital output bytes from FPGA (CPU ID = 3)
 *         and stores them in the global data structure (pe[3].nvo_byte).
 *         Debug LEDs are updated based on NVO signal fields such as TGT_G,
 *         TCF_G, CANCEL, and SECTIONAL_BELL.
 *         Ensure that nvo_u is properly mapped/updated with received data
 *         before using its fields for debug indication.
 */

uint8_t read_nvo(void) {
	uint8_t status;
	event_t* s;
	uint8_t i,k;
	nvo_union_t nvo_u;

	status = read_cpu_fpga(3U, CMD_READ_NON_VITAL,(NR_NON_VITAL_OUTPUT/8U),pe[3].nvo_byte);

#if 1
	LED_DBG0 = nvo_u.x.TGT_G;
	LED_DBG1 = nvo_u.x.TCF_G;
	LED_DBG2 = nvo_u.x.CANCEL;
	LED_DBG3 = nvo_u.x.SECTIONAL_BELL;
#endif
	return status;
}

/**
 * @brief  Reads relay feedback and fault status from FPGA.
 *
 * @param  [in]  None
 *
 * @return uint8_t
 *         - STATUS_OK   : Relay feedback data read successfully.
 *         - STATUS_FAIL : Communication failure while reading relay feedback.
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note   This function reads 4 bytes from FPGA (CPU ID = 3), which include
 *         relay feedback and fault information stored consecutively.
 *         The received data is stored in pe[3].vo_fdbk_fault_byte.
 *         Data format: first 2 bytes → feedback fault, next 2 bytes → feedback status.
 */
 
uint8_t read_relay_fdbk(void) {
	uint8_t status;

	//fdbk_fault,fdbk are consecutive 2 bytes each in pe array
	status = read_cpu_fpga(3,CMD_RD_RLY_FDBK,4,pe[3].vo_fdbk_fault_byte); 
	
	return status;
}

/**
 * @brief  Reads switch settings from FPGA with optional blocking response handling.
 *
 * @param  [in]  wait_for_response  Flag to indicate whether to wait for SPI response:
 *                                 - true  : Function blocks until response is received.
 *                                 - false : Function returns immediately after request.
 *
 * @return uint8_t
 *         - STATUS_OK   : Switch settings read successfully.
 *         - STATUS_FAIL : Communication failure or no response from FPGA.
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note   This function reads 4 bytes of switch setting data from FPGA (CPU ID = 3)
 *         and stores it in pe[3].switch_settings_byte.
 *         If wait_for_response is enabled, the function actively waits for SPI
 *         transaction completion and validates the response.
 *         On failure, received data is cleared and error flag (err_no_response)
 *         is set. On success, received data is copied to the processing buffer.
 */
 
uint8_t read_switch_settings(bool wait_for_response) {
	uint8_t status;
	uint8_t i;
	
	status = read_cpu_fpga(3,CMD_RD_MISC,4,pe[3].switch_settings_byte);

	if(wait_for_response) {	
		do {
			spi_com_fpga_sm();	
		}
		while(spi_com_fpga.isbusy);
	
		if(spi_com_fpga.status != STATUS_OK){ // No Response
			pe[3].err_no_response = 1;
			for(i=0;i<spi_com_fpga.ndata;i++) {
					spi_com_fpga.p_byte[i] = 0x00;
			}
			status = STATUS_FAIL;
		}
		else {
			pe[3].err_no_response = 0;
			for(i=0;i<spi_com_fpga.ndata;i++) {
					spi_com_fpga.p_byte[i] = spi_com_fpga.pdata[i];
			}
			status = STATUS_OK;
		}
	}

	return status;
}

/**
 * @brief  Reads remote logical output (OE_LO) data from specified CPU/FPGA.
 *
 * @param  [in]  cpuid   Target CPU/FPGA ID from which remote logical relay data is read.
 *
 * @return uint8_t
 *         - STATUS_OK   : Remote logical output data read successfully.
 *         - STATUS_FAIL : Communication failure while reading data.
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note   This function retrieves logical relay output status of the remote end
 *         using CMD_READ_REMOTE_LO command. The received data is stored in
 *         pe[cpuid].oe_lo_byte. Data length depends on NR_LOGIC_RELAYS.
 */
 
uint8_t read_oe_lo(uint8_t cpuid) {
	uint8_t status;

	status = read_cpu_fpga(cpuid, CMD_READ_REMOTE_LO,(NR_LOGIC_RELAYS/8U),pe[cpuid].oe_lo_byte);
	return status;
}

/**
 * @brief  Reads remote vital input (OE_VI) data from specified CPU/FPGA.
 *
 * @param  [in]  cpuid   Target CPU/FPGA ID from which remote vital input data is read.
 *
 * @return uint8_t
 *         - STATUS_OK   : Remote vital input data read successfully.
 *         - STATUS_FAIL : Communication failure while reading data.
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note   This function retrieves remote end vital input status using
 *         CMD_READ_REMOTE_INRLY command. The received data is stored in
 *         pe[cpuid].oe_vi_byte. Data length depends on NR_VITAL_INPUT.
 */

uint8_t read_oe_vi(uint8_t cpuid) {
	uint8_t status;

	status = read_cpu_fpga(cpuid, CMD_READ_REMOTE_INRLY,(NR_VITAL_INPUT/8U),pe[cpuid].oe_vi_byte);
	return status;
}

/**
 * @brief  Reads interlock mode from specified CPU/FPGA.
 *
 * @param  [in]  cpuid   Target CPU/FPGA ID from which interlock mode is read.
 *
 * @return uint8_t
 *         - STATUS_OK   : Interlock mode read successfully.
 *         - STATUS_FAIL : Communication failure while reading data.
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note   This function sends CMD_READ_INTERLOCK_MODE command to the specified
 *         device and retrieves 1 byte of interlock mode information.
 *         The received value is stored in the global variable 'interlock_mode'.
 */
 
uint8_t read_interlock_mode(uint8_t cpuid) {
	uint8_t status;

	status = read_cpu_fpga(cpuid, CMD_READ_INTERLOCK_MODE,1,&interlock_mode);
	return status;
}

/**
 * @brief  Reads remote vital output (OE_VO) data from specified CPU/FPGA.
 *
 * @param  [in]  cpuid   Target CPU/FPGA ID from which remote vital output data is read.
 *
 * @return uint8_t
 *         - STATUS_OK   : Remote vital output data read successfully.
 *         - STATUS_FAIL : Communication failure while reading data.
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note   This function retrieves remote end vital output status using
 *         CMD_READ_REMOTE_VO command. The received data is stored in
 *         pe[cpuid].oe_vo_byte. Data length depends on NR_VITAL_OUTPUT.
 */
 
uint8_t read_oe_vo(uint8_t cpuid) {
	uint8_t status;

	status = read_cpu_fpga(cpuid, CMD_READ_REMOTE_VO,(NR_VITAL_OUTPUT/8U),pe[cpuid].oe_vo_byte);
	return status;
}

 /**
 * @brief  Writes remote relay state (OE relay state) to FPGA.
 *
 * @param  [in]  None
 *
 * @return uint8_t
 *         - STATUS_OK   : Relay state written successfully.
 *         - STATUS_FAIL : Communication failure while writing data.
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note   This function sends OE relay state data to FPGA using
 *         CMD_WRITE_REMOTE_RELAY command via SPI interface.
 *         Data is transmitted MSB first, as FPGA shifts data left and
 *         transmits LSB first internally.
 *         The response buffer stores 1-byte response followed by checksum
 *         and inverted checksum. The first byte of response buffer is
 *         reserved for CPU ID.
 */
 
uint8_t write_oe_relay_state(void) {
	uint8_t status;

	//oe_relay_state MSB first as FPGA does left shift and sends lsb first (bit 0) of relay data to CPU123
	status = run_cmd(&spi_fpga,0,CMD_WRITE_REMOTE_RELAY,oe_relay_state,OE_RELAY_STATE_WIDTH,(uint8_t*)(respbuf_fpga+1),1); // frst entry kept free for cpuid, 1B response , checksum, inverted checksum

	return status;
}

/**
 * @brief  Writes CPU control parameters to a specified CPU and optionally waits for response.
 *
 * @param  [in]  cpuid              Target CPU ID to which control data is sent.
 * @param  [in]  wait_for_response  Flag to indicate response handling:
 *                                 - true  : Function blocks until SPI response is received.
 *                                 - false : Function returns immediately after command execution.
 *
 * @return uint8_t
 *         - STATUS_OK   : Control data written successfully.
 *         - STATUS_FAIL : Communication failure or no response from target CPU.
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note   This function updates CPU control structure and sends it using
 *         CMD_WRITE_CPU_CTRLS command over SPI.
 *         If wait_for_response is enabled, the function waits for SPI transaction
 *         completion and validates response status.
 *         In case of failure, err_no_response flag is set for the target CPU.
 *         Response buffer stores 1-byte response followed by checksum and inverted checksum.
 */
 
uint8_t write_cpu_ctrls(uint8_t cpuid, bool wait_for_response) {
	uint8_t status;
	uint8_t agent_id = cpuid;
	spi_struct_t* p_spi = &spi_cpux;
	spi_com_t* pspi_com = &spi_com_cpu;

	update_cpu_ctrls();
	status = run_cmd(p_spi,agent_id,CMD_WRITE_CPU_CTRLS,cpu_ctrls_u.bytes,sizeof(ctrls_struct_t),(uint8_t*)(respbuf_cpu+1),1U); 

	if(wait_for_response) {
		do {
			spi_com_cpu_sm();	
		}
		while(spi_com_cpu.isbusy);
	
		if(spi_com_cpu.status != STATUS_OK){ // No Response
			pe[cpuid].err_no_response = 1;
			status = STATUS_FAIL;
		}
		else {
			pe[cpuid].err_no_response = 0;
			status = STATUS_OK;
		}
	}

	return status;
}

/**
 * @brief  Sends a read command to CPU or FPGA and prepares buffer for received data.
 *
 * @param  [in]  fpga_cpuid  Target device ID:
 *                           - 0–2 : CPU devices
 *                           - 3   : FPGA device
 * @param  [in]  cmd         Command to be executed on target device.
 * @param  [in]  nr_bytes    Number of bytes expected in response.
 * @param  [out] p_byte      Pointer to buffer where received data will be stored.
 *
 * @return uint8_t
 *         - Always returns 0 (actual communication status handled asynchronously).
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note   This function configures SPI communication based on target device
 *         (CPU or FPGA) and issues a read command using run_cmd().
 *         It assigns the response buffer and links the received data pointer
 *         (p_byte) to SPI communication structure.
 *         Actual data transfer and status handling are performed asynchronously
 *         by SPI state machine (spi_com_cpu_sm / spi_com_fpga_sm).
 */
 
static uint8_t read_cpu_fpga(uint8_t fpga_cpuid, uint8_t cmd, uint8_t nr_bytes, uint8_t* p_byte) { 
	uint8_t status;
	event_t* s;
	uint8_t i;
	uint8_t agent_id;
	spi_com_t* pspi_com = &spi_com_cpu;
	spi_struct_t* p_spi = &spi_cpux;
	agent_id = fpga_cpuid;
	uint8_t* respbuf = respbuf_cpu;
	
	if(fpga_cpuid == 3U) {
		p_spi = &spi_fpga;
		agent_id = 0;
		pspi_com = &spi_com_fpga;
		respbuf = respbuf_fpga;
	}
	status = run_cmd(p_spi,agent_id,cmd,NULL,0,respbuf,nr_bytes);
	pspi_com->p_byte = p_byte;

	return 0;
}

/**
 * @brief  Checks SPI communication response status and processes received data.
 *
 * @param  [in]  fpga_cpuid  Target device ID:
 *                           - 0–2 : CPU devices
 *                           - 3   : FPGA device
 *
 * @return uint8_t
 *         - STATUS_OK   : Valid response received and data copied successfully.
 *         - STATUS_FAIL : No response or communication failure.
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note   This function validates SPI communication status for the specified
 *         device and updates corresponding error flags.
 *         On failure, received buffer is cleared and err_no_response is set.
 *         On success, received data (pdata) is copied to user buffer (p_byte).
 *         Uses appropriate SPI communication context (CPU or FPGA) based on ID.
 */
 
uint8_t chk_spi_com_response(uint8_t fpga_cpuid) { 
	uint8_t i;
	spi_com_t* pspi_com = &spi_com_cpu;
	uint8_t ret;
	
	if(fpga_cpuid == 3U) {
		pspi_com = &spi_com_fpga;
	}
	
	if(pspi_com->status != STATUS_OK){ // No Response
		pe[fpga_cpuid].err_no_response = 1;
		for(i=0;i<pspi_com->ndata;i++){
			pspi_com->p_byte[i] = 0x00;
		}
		ret = STATUS_FAIL;
	}
	else {
		pe[fpga_cpuid].err_no_response = 0;
		for(i=0;i<pspi_com->ndata;i++){
			pspi_com->p_byte[i] = pspi_com->pdata[i];
		}
		ret = STATUS_OK;
	}

	return ret;
}

/**
 * @brief  Sends a register read command over SPI to a specified agent.
 *
 * @param  [in]  pSPI         Pointer to SPI configuration/instance to be used.
 * @param  [in]  agent_id     Target device/agent ID for the SPI transaction.
 * @param  [in]  args         Pointer to argument buffer containing register address or parameters.
 * @param  [in]  nargs        Number of argument bytes to be sent.
 * @param  [in]  nresp_bytes  Number of expected response bytes from the target.
 *
 * @return uint8_t
 *         - STATUS_OK   : Command issued successfully.
 *         - STATUS_FAIL : Failed to issue SPI command.
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note   This function prepares and sends CMD_READ_REG command using run_cmd().
 *         The response is stored in a global response buffer (respbuf_cpu).
 *         Actual response handling and validation are performed asynchronously
 *         by the SPI communication state machine.
 */

uint8_t read_reg(spi_struct_t* pSPI, uint8_t agent_id, uint8_t* args, uint8_t nargs, uint8_t nresp_bytes) {
	uint8_t status;
	uint8_t* respbuf = respbuf_cpu;
	status = run_cmd(pSPI,agent_id,CMD_READ_REG,args,nargs,respbuf,nresp_bytes);
	return status;
}
 
/**
 * @brief  Sends a register write command over SPI to a specified agent.
 *
 * @param  [in]  pSPI     Pointer to SPI configuration/instance to be used.
 * @param  [in]  agent_id Target device/agent ID for the SPI transaction.
 * @param  [in]  args     Pointer to argument buffer containing register address and data to be written.
 * @param  [in]  nargs    Number of argument bytes to be sent.
 *
 * @return uint8_t
 *         - STATUS_OK   : Command issued successfully.
 *         - STATUS_FAIL : Failed to issue SPI command.
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note   This function sends CMD_WRITE_REG command using run_cmd().
 *         It expects a 2-byte response (typically ACK, checksum, etc.).
 *         The response is stored in a global response buffer (respbuf_cpu).
 *         Actual response validation is handled asynchronously by the SPI
 *         communication state machine.
 */

uint8_t write_reg(spi_struct_t* pSPI, uint8_t agent_id, uint8_t* args, uint8_t nargs) {
	uint8_t status;
	uint8_t* respbuf = respbuf_cpu;
	status = run_cmd(pSPI,agent_id,CMD_WRITE_REG,args,nargs,respbuf,2);
	return status;
}

/******************************************************************************/
// read status from cpux
// read input relay state from cpux
// read interlock output from cpux
// write cfg to cpux
// read cfg and status from cpux as per user command
// update event database
//
//pSPI - SPI master handle
//agent_id - SPI slave to be addressed.Used to select Chip select
//cmd - command to be executed
//pargs - pointer to arguments to command
//nargs - number of argument bytes
//pdata - pointer to data 
//ndata - number of data bytes

/**
 * @brief  Initializes and triggers an SPI command request.
 *
 * @param  [in]  pSPI       Pointer to SPI instance (CPU or FPGA SPI).
 * @param  [in]  agent_id   Target device/agent ID for the SPI transaction.
 * @param  [in]  cmd        Command to be executed.
 * @param  [in]  pargs      Pointer to argument buffer (can be NULL if no arguments).
 * @param  [in]  nargs      Number of argument bytes.
 * @param  [out] pdata      Pointer to buffer for received response data.
 * @param  [in]  ndata      Number of expected response bytes.
 *
 * @return uint8_t
 *         - Current SPI communication status (may be previous/ongoing status).
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note   This function prepares an SPI communication request by populating
 *         the spi_com_t structure and setting the new request flag.
 *         Actual SPI transfer is handled asynchronously by the SPI state machine
 *         (spi_com_cpu_sm or spi_com_fpga_sm).
 *         The function does not wait for completion; status must be checked later.
 */

static uint8_t run_cmd(spi_struct_t* pSPI, uint8_t agent_id, uint8_t cmd, uint8_t* pargs, uint8_t nargs, uint8_t* pdata, uint8_t ndata) {

	spi_com_t* pspi_com = &spi_com_cpu;

	if(pSPI == &spi_fpga){
		pspi_com = &spi_com_fpga;
	}

	pspi_com->pSPI = pSPI;
	pspi_com->agent_id = agent_id;
	pspi_com->cmd = cmd;
	pspi_com->pargs = pargs;
	pspi_com->nargs = nargs;
	pspi_com->pdata = pdata;
	pspi_com->ndata = ndata;
	pspi_com->newreq = 1;

	return pspi_com->status;
}
 
/**
 * @brief  SPI communication state machine for CPU devices.
 *
 * @param  [in]  None
 *
 * @return None
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note   This function implements a non-blocking state machine to handle
 *         SPI communication with CPU devices. It performs the following steps:
 *         - Prepares and sends command with arguments and checksum
 *         - Waits for transmission completion
 *         - Introduces a delay before reading response
 *         - Reads response bytes along with checksum and inverted checksum
 *         - Validates received data integrity
 *
 *         The function operates in multiple states:
 *         - State 0: Initialize and send command
 *         - State 1: Wait for transmit completion
 *         - State 2: Delay and prepare for read
 *         - State 3: Read response and validate checksum
 *
 *         Communication status is updated in spi_com_cpu.status:
 *         - 0 : Success
 *         - 1 : Failure (no response or checksum error)
 *
 *         This function must be called periodically (e.g., in main loop or ISR)
 *         to progress SPI transactions. It uses FIFO buffers for data transfer
 *         and supports variable-length command/response handling.
 */
 
void spi_com_cpu_sm(void) {

	uint8_t i;
	uint8_t cksum = 0;
	uint8_t ch;
	static uint8_t state = 0;

	switch(state) {
	case 0:
		if(!spi_com_cpu.newreq) {
			break;
		}
		spi_com_cpu.newreq = 0;
		spi_com_cpu.isbusy = 1;

		fifo_write(spi_com_cpu.pSPI->ptx,spi_com_cpu.cmd);
		cksum += spi_com_cpu.cmd;
		// write command arguments to fifo
		for(i=0;i<spi_com_cpu.nargs;i++) {
			fifo_write(spi_com_cpu.pSPI->ptx,spi_com_cpu.pargs[i]);
			cksum += spi_com_cpu.pargs[i];
		}
		fifo_write(spi_com_cpu.pSPI->ptx,cksum); //non inverted checksum
		fifo_write(spi_com_cpu.pSPI->ptx,~cksum); //inverted checksum
		// write command to fifo
		// send command to CPU through SPI
		spi_master_write(spi_com_cpu.pSPI,spi_com_cpu.agent_id);
		state = 1;
		break;

	case 1:
		// data sent?
		if(spi_com_cpu.pSPI->complete == 1) {
			state = 2;
			// discard read data
			fifo_reset(spi_com_cpu.pSPI->prx); 
			wait_time = 10; //set wait for 10ms
		}
		break;

	case 2:
		if(wait_time == 0U) { //check if wait over
			state = 3;
			// write ndata number of 0xFF to fifo. ndata depends on responses byte count 
			for(i=0;i<(spi_com_cpu.ndata + 2U);i++) { //+2 for checksum
				fifo_write(spi_com_cpu.pSPI->ptx,0xFF); // write 0xFF  
			}
			// read response now
			spi_master_read(spi_com_cpu.pSPI,spi_com_cpu.agent_id);
		}
		break;

	case 3:
		// data sent?
		if(spi_com_cpu.pSPI->complete == 1) {
			cksum = 0; // reset checksum
			for(i=0;i<(spi_com_cpu.ndata + 2U);i++) { //+2 for checksum
				if(spi_com_cpu.pSPI->prx->is_empty) {
					if(spi_com_cpu.pSPI != &spi_fpga){
						printf("@Error: CPU%d cmd %02X Failed\r\n",spi_com_cpu.agent_id,spi_com_cpu.cmd);
					}
					else
					{
						printf("@Error: FPGA cmd %02X Failed\r\n",spi_com_cpu.cmd);
					}

					spi_com_cpu.status = 1;
					spi_com_cpu.isbusy = 0;
					break;
				}
				fifo_read(spi_com_cpu.pSPI->prx,&ch);
				spi_com_cpu.pdata[i] = ch;
				if(i<(spi_com_cpu.ndata)){
					cksum += ch;
				}
			}

			spi_com_cpu.status = 0;
			
		if(spi_com_cpu.pSPI == &spi_fpga) {
			// invert checksum as first checksum byte is inverted
		//	cksum = ~cksum;
		}
		// validated received data checksum and inverted checksum
		if(cksum != spi_com_cpu.pdata[spi_com_cpu.ndata]){
			spi_com_cpu.status = 1;
		}
		cksum = ~cksum;
		if(cksum != spi_com_cpu.pdata[spi_com_cpu.ndata + 1U]) {
			spi_com_cpu.status = 1;
		}
	
		state = 0;
			spi_com_cpu.isbusy = 0;
		}
		break;
	default: 
		state = 0;
   		break;	   
	}
}

 /**
 * @brief  SPI communication state machine for FPGA device.
 *
 * @param  [in]  None
 *
 * @return None
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note   This function implements a non-blocking state machine to handle
 *         SPI communication with the FPGA. It performs the following steps:
 *         - Sends command with arguments and checksum
 *         - Waits for transmission completion
 *         - Introduces a delay before reading response
 *         - Reads response bytes along with checksum and inverted checksum
 *         - Validates received data integrity
 *
 *         The function operates in multiple states:
 *         - State 0: Initialize and send command
 *         - State 1: Wait for transmit completion
 *         - State 2: Delay and prepare for read
 *         - State 3: Read response and validate checksum
 *
 *         Communication status is updated in spi_com_fpga.status:
 *         - 0 : Success
 *         - 1 : Failure (no response or checksum error)
 *
 *         This function must be called periodically (e.g., in main loop or ISR)
 *         to progress SPI transactions. It uses FIFO buffers for data transfer
 *         and supports variable-length command/response handling.
 */
 
void spi_com_fpga_sm(void) {

	uint8_t i;
	uint8_t cksum = 0;
	uint8_t ch;
	static uint8_t state = 0;

	switch(state) {
	case 0:
		if(!spi_com_fpga.newreq) {
			break;
		}
		spi_com_fpga.newreq = 0;
		spi_com_fpga.isbusy = 1;
		//write cmd to fifo
		fifo_write(spi_com_fpga.pSPI->ptx,spi_com_fpga.cmd);
		cksum += spi_com_fpga.cmd;
		// write command arguments to fifo
		for(i=0;i<spi_com_fpga.nargs;i++) {
			fifo_write(spi_com_fpga.pSPI->ptx,spi_com_fpga.pargs[i]);
			cksum += spi_com_fpga.pargs[i];
		}
		fifo_write(spi_com_fpga.pSPI->ptx,cksum); //non inverted checksum
		fifo_write(spi_com_fpga.pSPI->ptx,~cksum); //inverted checksum
		// write command to fifo
		// send command to CPU through SPI
		spi_master_write(spi_com_fpga.pSPI,spi_com_fpga.agent_id);
		state = 1;
		break;

	case 1:
		// data sent?
		if(spi_com_fpga.pSPI->complete == 1) {
			state = 2;
			// discard read data
			fifo_reset(spi_com_fpga.pSPI->prx); 
			wait_time = 10; //set wait for 10ms
		}
		break;

	case 2:
		if(wait_time == 0U) { //check if wait over
			state = 3;
			// write ndata number of 0xFF to fifo. ndata depends on responses byte count 
			for(i=0;i<(spi_com_fpga.ndata+2U);i++) { //+2 for checksum
				fifo_write(spi_com_fpga.pSPI->ptx,0xFF); // write 0xFF  
			}
			// read response now
			spi_master_read(spi_com_fpga.pSPI,spi_com_fpga.agent_id);
		}
		break;

	case 3:
		// data rcvd?
		if(spi_com_fpga.pSPI->complete == 1) {
			cksum = 0; // reset checksum
			for(i=0;i<(spi_com_fpga.ndata + 2U);i++) { //+2 for checksum
				if(spi_com_fpga.pSPI->prx->is_empty) {
					if(spi_com_fpga.pSPI != &spi_fpga){
						printf("@Error: CPU%d cmd %02X Failed\r\n",spi_com_fpga.agent_id,spi_com_fpga.cmd);
					}
					else{
						printf("@Error: FPGA cmd %02X Failed\r\n",spi_com_fpga.cmd);
					}

					spi_com_fpga.status = 1;
					spi_com_fpga.isbusy = 0;
					break;
				}
				fifo_read(spi_com_fpga.pSPI->prx,&ch);
				spi_com_fpga.pdata[i] = ch;
				if(i<(spi_com_fpga.ndata)){
					cksum += ch;
				}
			}

			spi_com_fpga.status = 0;
			
		if(spi_com_fpga.pSPI == &spi_fpga) {
			// invert checksum as first checksum byte is inverted
		//	cksum = ~cksum;
		}
		// validated received data checksum and inverted checksum
		if(cksum != spi_com_fpga.pdata[spi_com_fpga.ndata]){
			spi_com_fpga.status = 1;
		}
		cksum = ~cksum;
		if(cksum != spi_com_fpga.pdata[spi_com_fpga.ndata + 1U]){
			spi_com_fpga.status = 1;
		}
	
		state = 0;
			spi_com_fpga.isbusy = 0;
		}
		break;
	default: 
		state = 0;
   		break;	   
	}
}
