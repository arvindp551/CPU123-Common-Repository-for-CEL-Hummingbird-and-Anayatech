/***************************************************************************//**
 * @file     fifo.c
 * @brief    All application specific source files
 * @version  1.0.0
 * @details
 * Compiler : Keil uVision
 * Target : Nuvoton M263
 * Module : Central Controller Card
 * Date Created : Dec 16, 2025
 * @copyright Copyright (C) 2026 Anaya Tech Systems Pvt. Ltd. . All rights reserved.
 * @author    DG
 * @par Functions Included
 * void fifo_init(fifo_t* pf, uint8_t* pbuf, uint16_t bufsize); <br>
 * void fifo_reset(fifo_t* pf); <br>
 * void fifo_read(fifo_t* pf, uint8_t* data); <br>
 * void fifo_write(fifo_t* pf, uint8_t data); <br>
 * void fifo_write_bytes(fifo_t* f, uint8_t* buf, uint8_t nr_bytes);
 ******************************************************************************/
 
#include <stdlib.h>
#include "fifo.h"
#include "stdbool.h"

/**
*
* @brief
* Initializes a FIFO (First-In-First-Out) buffer structure.
*
* This function sets up the FIFO control structure by initializing
* read/write pointers, status flags, error counters, and assigning
* the buffer memory and size.
*
* @param [out] pf
* Pointer to FIFO structure that will be initialized.
*
* @param [in] pbuf
* Pointer to the memory buffer allocated for FIFO storage.
*
* @param [in] bufsize
* Size of the FIFO buffer in bytes.
*
* @return
* None.
*
* @details
* Requirement ID(s) - ?
*
* @note
* After initialization:
* FIFO is empty (is_empty = true, is_full = false).
* Read and write pointers are reset to zero.
* Fill level is set to zero.
*
* Error counters (underrun and overflow) are cleared.
*
* The buffer memory must be allocated by the caller before calling this function.
*
* This function does not perform memory allocation; it only assigns
* the provided buffer to the FIFO structure.
*
* Ensure that bufsize is valid (> 0) to avoid undefined behavior.
*/

void fifo_init(fifo_t* pf, uint8_t* pbuf, uint16_t bufsize) {
	pf->rdptr = 0;
	pf->wrptr = 0;
	pf->is_full = false;
	pf->is_empty = true;
	pf->err_underrun = 0;
	pf->err_overflow = 0;
	pf->fill = 0;

	pf->bufsize = bufsize;
	pf->buf = pbuf;
}

/**
*
* @brief
* Resets the FIFO (First-In-First-Out) buffer to its initial empty state.
*
* This function clears the FIFO by resetting read/write pointers,
* status flags, error counters, and fill level without modifying
* the underlying buffer memory.
*
* @param [in,out] pf
* Pointer to FIFO structure to be reset.
*
* @return
* None.
*
* @details
* Requirement ID(s) - ?
*
* @note
* After reset:
* FIFO becomes empty (is_empty = true, is_full = false).
* Read and write pointers are set to zero.
* Fill level is reset to zero.
*
* Error counters (underrun and overflow) are cleared.
*
* The actual buffer data is not cleared; only control variables are reset.
*
* This function can be used to recover from error conditions such as overflow or underrun.
*/

void fifo_reset(fifo_t* pf) {
	pf->rdptr = 0;
	pf->wrptr = 0;
	pf->is_full = false;
	pf->is_empty = true;
	pf->err_underrun = 0;
	pf->err_overflow = 0;
	pf->fill = 0;
}

/**
 * @brief read from fifo
 * Read the data from fifo check if fifo is empty than make underflow to 1 and return 1, otherwise read the data at readpointer and calculate the checksum decrease the fill level until fifo not become empty. 
 * @param [in,out] pf
 * Pointer to FIFO structure from which data will be read.
 *
 * @param [out] data
 * Pointer to variable where the read byte will be stored.
 *
 * @return
 * None.
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note
 * If the FIFO is empty:
 * An underrun error flag (err_underrun) is set.
 * Data read may be invalid and should be handled carefully by the caller.
 *
 * After a successful read:
 * Read pointer (rdptr) is incremented with wrap-around handling.
 * Fill level is decremented.
 * FIFO is marked as not full.
 *
 * FIFO is marked empty when fill level reaches zero.
 *
 * Circular buffer behavior:
 * Read pointer wraps to zero when it reaches buffer size.
 *
 * Caller should ensure FIFO is not empty before calling this function to avoid underrun conditions.
*/

void fifo_read(fifo_t* pf, uint8_t* data) {
	if(pf->is_empty) {
		pf->err_underrun = 1;
	}

	*data = pf->buf[pf->rdptr];
	pf->rdptr++;
	pf->fill--;
	pf->is_full = false;

	if (pf->fill == 0U) {
		pf->is_empty = true;
	}

	if (pf->rdptr >= pf->bufsize) {
		pf->rdptr = 0;
	}
}

/**
 * @brief write in fifo
 * write data in fifo at write check if fifo is not full and increase write pointer and fill by 1 and calculate checksum by adding data 
 *
 * @param [in,out] pf
 * Pointer to FIFO structure where data will be written.
 *
 * @param [in] data
 * Byte of data to be written into the FIFO buffer.
 *
 * @return
 * None.
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note
 * If the FIFO is full:
 * An overflow error flag (err_overflow) is set.
 * Data will still be written, potentially overwriting existing data.
 *
 * After a successful write:
 * Write pointer (wrptr) is incremented with wrap-around handling.
 * Fill level is incremented.
 * FIFO is marked as not empty.
 *
 * FIFO is marked full when fill level reaches buffer size.
 *
 * Circular buffer behavior:
 * Write pointer wraps to zero when it reaches buffer size.
 *
 * Caller should ensure FIFO is not full before calling this function if data overwrite is not desired.	
 */

void fifo_write(fifo_t* pf, uint8_t data) {
	if(pf->is_full) {
		pf->err_overflow = 1;
	}

	pf->buf[pf->wrptr] = data;
	pf->wrptr++;
	pf->fill++;
  pf->is_empty = false;
	
	if (pf->fill == pf->bufsize) {
		pf->is_full = true;
	}

	if (pf->wrptr >= pf->bufsize) {
		pf->wrptr = 0;
	}
}

#if 0
/**
 * @brief write byte by byte in fifo
 *
 * @param [in,out] f
 * Pointer to FIFO structure where data will be written.
 *
 * @param [in] buf
 * Pointer to the buffer containing data to be written into the FIFO.
 *
 * @param [in] nr_bytes
 * Number of bytes to be written into the FIFO.
 *
 * @return
 * None.
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note
 * This function internally calls fifo_write() for each byte.
 *
 * If the FIFO becomes full during the operation:
 * Overflow error flag (err_overflow) may be set by fifo_write().
 * Additional data may overwrite existing data depending on implementation.
 *
 * No boundary checking is performed before writing all bytes.
 * Caller should ensure sufficient space in FIFO if overwrite is not desired.
 *
 * Suitable for bulk data transfer into FIFO buffers.
 *
 * Execution time is proportional to nr_bytes.
 */

void fifo_write_bytes(fifo_t* f, uint8_t* buf, uint8_t nr_bytes) {
	uint8_t i;

	for(i=0;i<nr_bytes;i++) {
		fifo_write(f,buf[i]);
	}
}
#endif
