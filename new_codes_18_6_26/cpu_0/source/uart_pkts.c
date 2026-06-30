/***************************************************************************//**
 * @file     uart_pkts.c
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
 * uint8_t LINE_COM_write_packet(fifo_t* p, uint8_t com_id); <br>
 * uint8_t LINE_COM_read_packet(fifo_t* p, uint8_t com_id); <br>
 * uint8_t wr_sys_status_pkt(fifo_t* p); <br>
 * uint8_t wr_card_status_pkt(fifo_t* p); <br>
 * uint8_t wr_relay_status_pkt(fifo_t* p,uint8_t card, uint8_t channelNo);
 ******************************************************************************/

#include "common_header.h"
#include "uart_pkts.h"
#include "app.h"
#include "bsp.h"
#include "interlock.h"
#include "string.h"
/**
 * @file uart_pkts.c
 * @author SG
 * @date 1 Jan 2025
 * @brief File contains all uart packet function.
 *
 * there are various type of packet function that generate various packet format. Which are send through COM port, event logger and display ports. 
 * @see https://www.anayatech.com
 */

extern void conv2TOD(uint32_t seconds,time_s* tod);
extern uint32_t conv2seconds(time_s* tod);
uint32_t toint(char* str);
extern system_t sys;
extern uint8_t com_id;

char strg[50];
com_rx_union_t oe_pkt[2];

/**
 * @brief  generate com port relay information packet
 * Based on user selection either com0 or com1 generate packet. 
 * packet hold station ID, unit address, secure code and time stamp 
 * and vital input state, vital output state, logical relay and checksum.
 * .
 *
 * @param  [in,out] p       Pointer to FIFO buffer where packet is written.
 * @param  [in]     com_id  Communication link identifier (e.g., primary/secondary).
 *
 * @return 0 if packet is successfully written to FIFO,
 *         1 if FIFO is not empty and packet is not written.
 *
 * @details
 * Requirement ID(s) -
 * PI_SSBPAC_SwRS_POST_034
 * PI_SSBPAC_SwRS_POST_035
 *
 * @note   - Packet is written only if FIFO is empty to avoid data overlap.
 *         - Packet format includes:
 *             * Start delimiter ':' 
 *             * Packet type ('C' for status change)
 *             * Station ID and local ID
 *             * Communication status (secure code response)
 *             * Timestamp (converted from sys.global_ts)
 *             * System error code
 *             * Line states (TGT, TCF)
 *             * Vital Inputs (VI), Vital Outputs (VO), Logical Outputs (LO)
 *             * Checksum (2 bytes)
 *             * End delimiter '%'
 *         - Uses conv2TOD() for timestamp conversion.
 *         - Data is written MSB first for VI, VO, and LO.
 *         - Checksum is accumulated during FIFO write operations.
 *         - Assumes FIFO and system structures are properly initialized.
 */
uint8_t LINE_COM_write_packet(fifo_t* p, uint8_t com_id) {
	uint16_t cksum;
	uint8_t i;
	switch_settings_struct_t* ss;
	ss = (switch_settings_struct_t*)(pe[3].switch_settings_byte); 

	// COM status change packet
	if(p->is_empty) {
		fifo_write(p,':');
		p->cksum_wr = 0;	
		if(sys.send_pkt_type == PKT_TYPE_RESET) {
			fifo_write(p,'R'); //Reset
		}
		else if(sys.send_pkt_type == PKT_TYPE_ACK) {
			fifo_write(p,'A'); //Ack
		}
		else {
			fifo_write(p,'C'); //Status Change
		}
		fifo_write(p,'$');
		fifo_write_bytes(p,(uint8_t*)cfg.stationID,17);
		sprintf(strg,"%03d",local_addr_pwrup);
		fifo_write_bytes(p,(uint8_t*)strg,4);
		fifo_write_bytes(p,(uint8_t*)&(sys.secure_code_ans_com[com_id]),2);
		conv2TOD(sys.global_ts,&tod);
		sprintf(strg,"%02d-%02d-%02d %02d-%02d-%02d",tod.yy,tod.mo,tod.dd,tod.hh,tod.mm,tod.ss);
		fifo_write_bytes(p,(uint8_t*)strg,17);
		sprintf(strg,"%04d",sys.error);
		fifo_write_bytes(p,(uint8_t*)strg,5);
		fifo_write_bytes(p,(uint8_t*)sys.line_state_TGT,3);
		fifo_write_bytes(p,(uint8_t*)sys.line_state_TCF,3);

		for(i=0U;i<(NR_VITAL_INPUT/8U);i++) {
			fifo_write(p,pe[3].vi_byte[i]); //send MSB first
		}

		for(i=0U;i<(NR_VITAL_OUTPUT/8U);i++) {
			fifo_write(p,pe[3].vo_byte[i]); //send MSB first
		}
		for(i=0U;i<(NR_LOGIC_RELAYS/8U);i++) {
			fifo_write(p,pe[3].lo_byte[i]); //send MSB first
		}

		cksum = p->cksum_wr;
		fifo_write_bytes(p,(uint8_t*)&cksum,2);
		fifo_write(p,'%');
		return 0;
	}
	return 1;
}

/**
 * @brief  read com port relay information packet
 * Based on user selection either com0 or com1 read packet. 
 * If packet start from : and end with % and checksum match 
 * with receiving point checksum than read the vital input, 
 * vital output and logical output otherwise discard the packet.
 *
 * @param  [in,out] p       Pointer to FIFO buffer containing received data.
 * @param  [in]     com_id  Communication link identifier (e.g., primary/secondary).
 *
 * @return 0 if a valid packet is received and processed successfully,
 *         1 if packet is invalid, incomplete, or no valid packet available.
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note   - Processes incoming data stream byte-by-byte from FIFO.
 *         - Detects start of frame (':') and end of frame ('%').
 *         - Maintains separate parsing state (index, checksum, SOF flag)
 *           for each communication channel using static arrays.
 *         - Validates packet using:
 *             * Checksum comparison
 *             * Remote address match (with switch settings)
 *         - On successful validation:
 *             * Updates oe_relay_state[] with received input, output,
 *               and logical relay states (MSB first).
 *         - Error handling:
 *             * Increments checksum error counter on mismatch.
 *             * Increments address mismatch counter if address differs.
 *             * Increments incomplete packet error if expected size exceeded.
 *         - Uses com_rx_struct_t structure layout for packet decoding.
 *         - Assumes FIFO and packet buffers are properly initialized.
 */
 
uint8_t LINE_COM_read_packet(fifo_t* p, uint8_t com_id) {
	uint8_t data,i;
	uint8_t ret = 1;
	fifo_t* f = uart_el.ptx;

	static uint16_t cksum[2];
	static uint16_t indx[2] = {0,0}; // maintain buffer write pointer for each com port
	static uint8_t sof_rcvd[2] = {0,0};

	switch_settings_struct_t* ss;

	while(!p->is_empty) {
		ss = (switch_settings_struct_t*)(pe[3].switch_settings_byte); 
		fifo_read(p,&data);
		if(data == ':') { //start of frame
			indx[com_id] = 0; // new packet reset buffer pointer
			cksum[com_id] = 0;	
			sof_rcvd[com_id] = 1;
		}
		else if(sof_rcvd[com_id]) {
			uint16_t x;

			if(data == '%') { //end of frame
				sof_rcvd[com_id] = 0;
				cksum[com_id] -= oe_pkt[com_id].bytes[indx[com_id]-1];
				cksum[com_id] -= oe_pkt[com_id].bytes[indx[com_id]-2];

				uint32_t addr = toint((char*)oe_pkt[com_id].rx.unit_address);

				if(cksum[com_id] != oe_pkt[com_id].rx.cksum) {
					sys.com_cksum_err[com_id]++;
					ret = 1;
				}
				else if(addr != remote_addr_pwrup) {
					sys.com_addr_mismatch_err[com_id]++;
					ret = 1;
				}
				else if(oe_pkt[com_id].rx.type == 'A') {
					sys.reset_ack_pkt_cnt++;
					tmr.wait_reset_ack = SEC(3);
					ret = 0;
				}
				else if(oe_pkt[com_id].rx.type == 'R') {
					sys.reset_pkt_cnt++;
					tmr.wait_reset = SEC(3);
					ret = 0;
				}
				else {
					sys.C_pkt_cnt++;
					tmr.wait_C_pkt = SEC(3);
					x = 0;
					sys.reset_ack_pkt_cnt = 0;
					for(i=0U;i<(NR_VITAL_INPUT/8U);i++) {
						oe_relay_state[x+i] = oe_pkt[com_id].rx.inrelay_state[i]; //MSB first
					}
					x += (NR_VITAL_INPUT/8U);
					for(i=0U;i<(NR_VITAL_OUTPUT/8U);i++) {
						oe_relay_state[x+i] = oe_pkt[com_id].rx.vorelay_state[i]; //MSB first
					}
					x += (NR_VITAL_OUTPUT/8U);
					for(i=0U;i<(NR_LOGIC_RELAYS/8U);i++) {
						oe_relay_state[x+i] = oe_pkt[com_id].rx.lorelay_state[i]; //MSB first
					}
					ret = 0;
				}
			}
			else {
				i = sizeof(com_rx_struct_t);

				if(indx[com_id] == i) { //expected eop % but failed. Retract
					sof_rcvd[com_id] = 0;
					sys.com_pkt_incomplete_err[com_id]++;
					ret = 1;
				}
				else {
					oe_pkt[com_id].bytes[indx[com_id]] = data;
					indx[com_id]++;
				}
			}
			cksum[com_id] += data;
		}
		else{
			// Do Nothing
		}
	}
	return ret;
}

// Event Logger
//function for line_status and relay logic implementation
/**
 * @brief  Constructs and writes a system status packet to the FIFO buffer.
 *         The packet contains timestamp, system error, communication state,
 *         block mode, and line states.
 *
 * @param  [in,out] p  Pointer to FIFO buffer where the packet will be written.
 *
 * @return 0 if packet is successfully written,
 *         1 if FIFO is not empty and packet is not written.
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note   - Packet is written only when FIFO is empty to avoid overwriting data.
 *         - Packet format:
 *             * Start delimiter '%'
 *             * Packet type 'D' (System Status)
 *             * Timestamp (YY:MM:DD:HH:MM:SS)
 *             * System error code
 *             * Communication state
 *             * Block mode (S/D/M/E)
 *             * Line states (TGT and TCF)
 *             * End delimiter '$' followed by newline
 *         - Uses conv2TOD() to convert system timestamp.
 *         - Assumes sys, cfg, and FIFO structures are properly initialized.
 */
 
uint8_t wr_sys_status_pkt(fifo_t* p) {
	if(p->is_empty) {
		fifo_write(p,'%');
		fifo_write(p,'D'); //System Status
		conv2TOD(sys.global_ts,&tod);
		sprintf(strg,"%02d:%02d:%02d:%02d:%02d:%02d",tod.yy,tod.mo,tod.dd,tod.hh,tod.mm,tod.ss);
		fifo_write_bytes(p,(uint8_t*)strg,17);
		sprintf(strg,"%04d",sys.error);
		fifo_write_bytes(p,(uint8_t*)strg,5);
		sprintf(strg,"%d",sys.com_state);
		fifo_write_bytes(p,(uint8_t*)strg,1);
		fifo_write(p,cfg.block_mode);
		sprintf(strg,"%s",sys.line_state_TGT);
		fifo_write_bytes(p,(uint8_t*)sys.line_state_TGT,3);
		fifo_write_bytes(p,(uint8_t*)sys.line_state_TCF,3);
		fifo_write(p,'$');
		fifo_write(p,'\n');
		return 0;
	}
	return 1;
}

/**
 * @brief  Constructs and writes a card status packet to the FIFO buffer.
 *         The packet includes timestamp, error codes of CPUs/FPGA, and
 *         presence status of different cards.
 *
 * @param  [in,out] p  Pointer to FIFO buffer where the packet will be written.
 *
 * @return 0 if packet is successfully written,
 *         1 if FIFO is not empty and packet is not written.
 *
 * @details
 * Requirement ID(s) -
 * PI_SSBPAC_SwRS_POST_022
 * PI_SSBPAC_SwRS_POST_023
 * PI_SSBPAC_SwRS_POST_024
 *
 * @note   - Packet is written only when FIFO is empty to prevent overwrite.
 *         - Packet format:
 *             * Start delimiter '%'
 *             * Packet type 'S' (Card Status)
 *             * Timestamp (YY:MM:DD:HH:MM:SS)
 *             * Error codes for CPU0–CPU3/FPGA (4 entries)
 *             * Card presence status:
 *                 'P' = Present, 'A' = Absent
 *                 (IC1, IC2, IC3, OC, NVC)
 *             * End delimiter '$' followed by newline
 *         - Uses conv2TOD() for timestamp conversion.
 *         - Reads switch settings from FPGA (pe[3]) to determine card presence.
 *         - Assumes FIFO and system structures are properly initialized.
 */

uint8_t wr_card_status_pkt(fifo_t* p) {
	uint8_t i;
	switch_settings_struct_t* ss;

	if(p->is_empty) {
		ss = (switch_settings_struct_t*)(pe[3].switch_settings_byte); 
		
		fifo_write(p,'%'); //SOP
		fifo_write(p,'S'); //TYPE
		conv2TOD(sys.global_ts,&tod);
		sprintf(strg,"%02d:%02d:%02d:%02d:%02d:%02d",tod.yy,tod.mo,tod.dd,tod.hh,tod.mm,tod.ss);
		fifo_write_bytes(p,(uint8_t*)strg,17);
		for(i=0U;i<4U;i++) {
			sprintf(strg,"%04d",pe[i].err_code);
			fifo_write_bytes(p,(uint8_t*)strg,5);
		}
		
		if(ss->IC1_PRESENT) {
			fifo_write(p,'P');
		}		
		else{
			fifo_write(p,'A');
		}
		if(ss->IC2_PRESENT) {
			fifo_write(p,'P');
		}		
		else{ 
			fifo_write(p,'A');
		}		
		if(ss->IC3_PRESENT) {
			fifo_write(p,'P');
		}		
		else {
			fifo_write(p,'A');
		}
		if(ss->OC_PRESENT) {
			fifo_write(p,'P');
		}
		else{ 
			fifo_write(p,'A');
		}
		if(ss->NVC_PRESENT) {
			fifo_write(p,'P');
		}
		else{ 
			fifo_write(p,'A');
		}
		fifo_write(p,'$'); //EOP
		fifo_write(p,'\n');
		return 0;
	}
	return 1;
}

/**
 * @brief Writes system status packet to the provided FIFO buffer.
 *
 * Constructs a system status packet containing timestamp, system error code,
 * communication state, block mode, and line states. The packet is written
 * only if the FIFO buffer is empty.
 *
 * @param [in,out] p Pointer to FIFO buffer where the packet will be written.
 *
 * @return uint8_t Returns 0 if packet is successfully written,
 *                 returns 1 if FIFO is not empty (no write performed).
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note Packet format includes start/end markers and formatted timestamp.
 */

uint8_t wr_relay_status_pkt(fifo_t* p,uint8_t card, uint8_t channelNo) {
	if(p->is_empty) {
		fifo_write(p,'%'); //SOP
		fifo_write(p,'T'); //TYPE
		conv2TOD(sys.global_ts,&tod);
		sprintf(strg,"%02d:%02d:%02d:%02d:%02d:%02d",tod.yy,tod.mo,tod.dd,tod.hh,tod.mm,tod.ss);
		fifo_write_bytes(p,(uint8_t*)strg,17); //timestamp

		//card type
		event_t* s;
		if(card == IN_RLY_CARD) {
			fifo_write(p, 'I');
			s = &(pe[3].vi[channelNo]);
		}
		else if(card == OUT_RLY_CARD) {
			fifo_write(p, 'O');
			s = &(pe[3].vo[channelNo]);
		}
		else{
			// Do Nothing
		}
		//channel no
		sprintf(strg,"%2d",channelNo+1);
		fifo_write_bytes(p,(uint8_t*)strg,2);
		//channel status
		if(s->state == 0U){
			fifo_write(p,'D');
		}
		else if(s->state == 1U){
			fifo_write(p,'P');
		}
		else{ 
			fifo_write(p,'E');
		}
		fifo_write(p,'$'); //EOP
		fifo_write(p,'\n');
		return 0;
	}
	return 1;
}

