/***************************************************************************//**
 * @file     fifo.c
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
 * uint8_t fifo_init(fifo_t* pf, uint8_t* pbuf, int bufsize); <br>
 * uint8_t fifo_reset(fifo_t* pf); <br>
 * uint8_t fifo_read(fifo_t* pf, uint8_t* data); <br>
 * uint8_t fifo_write(fifo_t* pf, uint8_t data); <br>
 * void fifo_write_bytes(fifo_t* f, uint8_t* buf, uint8_t nr_bytes); <br>
 * uint8_t fifo_read_isr(fifo_t* pf, uint8_t* data); <br>
 * uint8_t fifo_write_isr(fifo_t* pf, uint8_t data);
 ******************************************************************************/

#include <stdlib.h>
#include "fifo.h"
#include "isp_user.h"

/**
 * @brief Initializes a FIFO (First-In-First-Out) buffer structure.
 *
 * This function sets up the FIFO control structure by initializing read/write
 * pointers, status flags, error indicators, and assigning the buffer memory.
 * The FIFO is initialized to an empty state.
 *
 * @param [in,out] pf Pointer to FIFO structure to be initialized.
 * @param [in] pbuf Pointer to memory buffer allocated for FIFO storage.
 * @param [in] bufsize Size of the FIFO buffer in bytes.
 *
 * @return uint8_t Always returns 0 indicating successful initialization.
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note The buffer memory (pbuf) must be pre-allocated and valid.
 *       After initialization, FIFO is empty and ready for read/write operations.
 */

uint8_t fifo_init(fifo_t* pf, uint8_t* pbuf, int bufsize) {
	pf->rdptr = 0U;
	pf->wrptr = 0U;
	pf->is_full = 0U;
	pf->is_empty = 1U;
	pf->err_underrun = 0U;
	pf->err_overflow = 0U;
	pf->fill = 0U;

	pf->bufsize = bufsize;
	pf->buf = pbuf;

	return 0;
}

/**
 * @brief Resets the FIFO buffer to its initial empty state.
 *
 * This function clears the FIFO by resetting read/write pointers,
 * status flags, error indicators, and fill level. The associated
 * buffer memory is not modified, only the control structure is reset.
 *
 * @param [in,out] pf Pointer to FIFO structure to be reset.
 *
 * @return uint8_t Always returns 0 indicating successful reset.
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note This operation discards any existing data in the FIFO.
 *       Use with caution if pending data needs to be preserved.
 */
 
uint8_t fifo_reset(fifo_t* pf) {
	pf->rdptr = 0;
	pf->wrptr = 0;
	pf->is_full = 0;
	pf->is_empty = 1;
	pf->err_underrun = 0;
	pf->err_overflow = 0;
	pf->fill = 0;

	return 0;
}
 
/**
 * @brief Reads a byte from the FIFO buffer.
 *
 * Retrieves one byte from the FIFO and updates internal pointers,
 * checksum, and status flags. If the FIFO is empty, an underrun
 * error is flagged and no data is read.
 *
 * @param [in,out] pf Pointer to FIFO structure from which data is to be read.
 * @param [out] data Pointer to variable where the read byte will be stored.
 *
 * @return uint8_t Returns 0 on successful read,
 *                 returns 1 if FIFO is empty (underrun condition).
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note Interrupts are temporarily disabled while updating the fill level
 *       to avoid race conditions with ISR-based FIFO operations.
 *       The function also updates a running checksum (cksum_rd).
 */

uint8_t fifo_read(fifo_t* pf, uint8_t* data) {
	if(pf->is_empty) {
		pf->err_underrun = 1;
		return 1;
	}

	*data = pf->buf[pf->rdptr];
	pf->cksum_rd += *data;
	pf->rdptr++;

	//fill is updated in fifo_write_isr also. Disable irq before update to avoid race condition 
	__disable_irq();
	pf->fill--;
	__enable_irq();

	pf->is_full = 0U;

	if (pf->fill == 0U) {
		pf->is_empty = 1;
	}

	if (pf->rdptr >= pf->bufsize) {
		pf->rdptr = 0;
	}

	return 0;
}

 /**
 * @brief Writes a byte to the FIFO buffer.
 *
 * Adds a single byte to the FIFO and updates internal write pointer,
 * checksum, fill count, and status flags. If the FIFO is full, an
 * overflow error is flagged and the write operation is not performed.
 *
 * @param [in,out] pf Pointer to FIFO structure where data is to be written.
 * @param [in] data Byte value to be written into the FIFO.
 *
 * @return uint8_t Returns 0 on successful write,
 *                 returns 1 if FIFO is full (overflow condition).
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note Interrupts are temporarily disabled while updating the fill level
 *       to prevent race conditions with ISR-based FIFO operations.
 *       The function maintains a running checksum (cksum_wr).
 */

uint8_t fifo_write(fifo_t* pf, uint8_t data) {
	if(pf->is_full) {
		pf->err_overflow = 1;
		return 1;
	}

	pf->cksum_wr += data;
	pf->buf[pf->wrptr] = data;
	pf->wrptr++;

	//fill is updated in fifo_write_isr also. Disable irq before update to avoid race condition 
	__disable_irq();
	pf->fill++;
	__enable_irq();

  	pf->is_empty = 0;
	
	if (pf->fill == pf->bufsize) {
		pf->is_full = 1;
	}

	if (pf->wrptr >= pf->bufsize) {
		pf->wrptr = 0;
	}
	return 0;
}

/**
 * @brief Writes multiple bytes to the FIFO buffer.
 *
 * Iteratively writes a sequence of bytes from the provided buffer into
 * the FIFO using fifo_write(). Each byte is written in order.
 *
 * @param [in,out] f Pointer to FIFO structure where data is to be written.
 * @param [in] buf Pointer to buffer containing bytes to be written.
 * @param [in] nr_bytes Number of bytes to write into the FIFO.
 *
 * @return void
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note This function does not perform bulk write optimization and relies
 *       on fifo_write() for each byte. Overflow handling is managed inside
 *       fifo_write().
 */
 
void fifo_write_bytes(fifo_t* f, uint8_t* buf, uint8_t nr_bytes) {
	uint8_t i;

	for(i=0;i<nr_bytes;i++) {
		fifo_write(f,buf[i]);
	}
}

/**
 * @brief Reads a byte from the FIFO buffer (ISR-safe version).
 *
 * Retrieves a single byte from the FIFO and updates internal pointers,
 * checksum, and status flags. This version is intended for use inside
 * interrupt service routines (ISR) and does not disable interrupts while
 * updating shared variables.
 *
 * @param [in,out] pf Pointer to FIFO structure from which data is to be read.
 * @param [out] data Pointer to variable where the read byte will be stored.
 *
 * @return uint8_t Returns 0 on successful read,
 *                 returns 1 if FIFO is empty (underrun condition).
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note Unlike the non-ISR version, this function does not use interrupt
 *       protection while updating the fill level. It must only be used in
 *       ISR context where concurrency is controlled. Updates checksum (cksum_rd).
 */
 
uint8_t fifo_read_isr(fifo_t* pf, uint8_t* data) {
	if(pf->is_empty) {
		pf->err_underrun = 1;
		return 1;
	}

	*data = pf->buf[pf->rdptr];
	pf->cksum_rd += *data;
	pf->rdptr++;
	pf->fill--;
	pf->is_full = 0U;

	if (pf->fill == 0U) {
		pf->is_empty = 1;
	}

	if (pf->rdptr >= pf->bufsize) {
		pf->rdptr = 0;
	}

	return 0;
}

/**
 * @brief Writes a byte to the FIFO buffer (ISR-safe version).
 *
 * Adds a single byte to the FIFO and updates internal write pointer,
 * checksum, fill count, and status flags. This version is intended for
 * use within interrupt service routines (ISR) and does not perform any
 * interrupt protection while updating shared variables.
 *
 * @param [in,out] pf Pointer to FIFO structure where data is to be written.
 * @param [in] data Byte value to be written into the FIFO.
 *
 * @return uint8_t Returns 0 on successful write,
 *                 returns 1 if FIFO is full (overflow condition).
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note This function should only be used in ISR context where concurrent
 *       access is controlled. It does not disable interrupts during updates.
 *       Maintains a running checksum (cksum_wr).
 */

uint8_t fifo_write_isr(fifo_t* pf, uint8_t data) {
	if(pf->is_full) {
		pf->err_overflow = 1;
		return 1;
	}

	pf->cksum_wr += data;
	pf->buf[pf->wrptr] = data;
	pf->wrptr++;
	pf->fill++;
  	pf->is_empty = 0;
	
	if (pf->fill == pf->bufsize) {
		pf->is_full = 1;
	}

	if (pf->wrptr >= pf->bufsize) {
		pf->wrptr = 0;
	}
	return 0;
}
