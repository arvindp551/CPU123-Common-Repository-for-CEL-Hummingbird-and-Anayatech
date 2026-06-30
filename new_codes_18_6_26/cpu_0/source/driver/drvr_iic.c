#include "common_header.h"
#include "drvr_iic.h"
#include "stdio.h"

static void i2c_isr_write(i2c_struct_t* i2c, uint32_t status);
static void i2c_isr_read(i2c_struct_t* i2c, uint32_t status);
static void i2c_isr_write_read(i2c_struct_t* i2c, uint32_t status);

extern volatile I2C_FUNC s_I2C0HandlerFn;

void i2c_ctor(i2c_struct_t* i2c) {
	/* Enable I2C0 peripheral clock */
	CLK_EnableModuleClock(I2C0_MODULE);
	/* Set PC multi-function pins for I2C0 SDA and SCL */
	SYS->GPB_MFPL &= ~(SYS_GPB_MFPL_PB4MFP_Msk | SYS_GPB_MFPL_PB5MFP_Msk);
	SYS->GPB_MFPL |= (SYS_GPB_MFPL_PB4MFP_I2C0_SDA | SYS_GPB_MFPL_PB5MFP_I2C0_SCL);
	/* Open I2C module and set bus clock */
	I2C_Open(i2c->inst, 100000);
	/* Enable I2C interrupt */
	I2C_EnableInt(i2c->inst);
	NVIC_EnableIRQ(I2C0_IRQn);
    I2C_EnableTimeout(i2c->inst, 0);

}

uint8_t i2c_write(i2c_struct_t* i2c, uint8_t* p_wrbuf, uint16_t wrlen) {
	if(i2c->is_busy) return 1;
	g_I2C_i32ErrCode = 0;
	i2c->timeout = I2C_TIMEOUT;
	i2c->is_busy = 1;
	i2c->todo = I2C_WRITE;
	i2c->indx = 0;
	i2c->p_wrbuf = p_wrbuf;
	i2c->nwr_bytes = wrlen;
	i2c->p_rdbuf = NULL;
	i2c->nrd_bytes = 0;
	s_I2C0HandlerFn = (I2C_FUNC)i2c_isr_write;

	I2C_START(i2c->inst); /* Send START */
	return 0;
}

uint8_t i2c_read(i2c_struct_t* i2c, uint8_t* p_rdbuf, uint16_t rdlen) {
	if(i2c->is_busy) return 1;
	g_I2C_i32ErrCode = 0;
	i2c->timeout = I2C_TIMEOUT;
	i2c->is_busy = 1;
	i2c->todo = I2C_READ;
	i2c->indx = 0;
	i2c->p_wrbuf = NULL;
	i2c->nwr_bytes = 0;
	i2c->p_rdbuf = p_rdbuf;
	i2c->nrd_bytes = rdlen;
	s_I2C0HandlerFn = (I2C_FUNC)i2c_isr_read;

	I2C_START(i2c->inst); /* Send START */
	return 0;
}

uint8_t i2c_write_read(i2c_struct_t* i2c, uint8_t* p_wrbuf, uint16_t wrlen, uint8_t* p_rdbuf, uint16_t rdlen) {
	if(i2c->is_busy) return 1;
	g_I2C_i32ErrCode = 0;
	i2c->timeout = I2C_TIMEOUT;
	i2c->is_busy = 1;
	i2c->todo = I2C_WRITE_READ;
	i2c->indx = 0;
	i2c->p_wrbuf = p_wrbuf;
	i2c->nwr_bytes = wrlen;
	i2c->p_rdbuf = p_rdbuf;
	i2c->nrd_bytes = rdlen;
	s_I2C0HandlerFn = (I2C_FUNC)i2c_isr_write_read;

	I2C_START(i2c->inst); /* Send START */
	return 0;
}


void i2c_isr_write(i2c_struct_t* i2c, uint32_t status) {
	switch(status) {
		case 0x08u:
			/* Write SLA+W to Register I2CDAT */
			I2C_SET_DATA(i2c->inst, (uint8_t)(i2c->slave_addr << 1u | 0x00u));
			I2C_SET_CONTROL_REG(i2c->inst, I2C_CTL_SI); /* Clear SI */
			break;
		case 0x18u: /* Slave Address ACK */
		case 0x28u:
			if(i2c->nwr_bytes) {
				I2C_SET_DATA(i2c->inst, i2c->p_wrbuf[i2c->indx]); /* Write Data to I2CDAT */
				i2c->nwr_bytes--;
				i2c->indx++;
				I2C_SET_CONTROL_REG(i2c->inst, I2C_CTL_SI); /* Clear SI */
			}
			else {
				i2c->is_busy = 0u;
				i2c->is_err = 0u;
				I2C_SET_CONTROL_REG(i2c->inst, I2C_CTL_STO_SI); /* Clear SI and send STOP*/
			}
			break;
		case 0x20u:                                           /* Slave Address NACK */
		case 0x30u:                                           /* Master transmit data NACK */
			I2C_SET_CONTROL_REG(i2c->inst, I2C_CTL_STO_SI); /* Clear SI and send STOP*/
			i2c->is_busy = 0u;
			i2c->is_err = 1u;
			break;
		case 0x38u:                                           /* Arbitration Lost */
		default:                                             /* Unknow status */
			I2C_SET_CONTROL_REG(i2c->inst, I2C_CTL_STO_SI); /* Clear SI and send STOP*/
			i2c->is_busy = 0u;
			i2c->is_err = 1u;
			break;
	}
}

void i2c_isr_read(i2c_struct_t* i2c, uint32_t status) {
	switch(status) {
		case 0x08u:
			/* Write SLA+R to Register I2CDAT */
			I2C_SET_DATA(i2c->inst, (uint8_t)(i2c->slave_addr << 1u | 0x01u));
			I2C_SET_CONTROL_REG(i2c->inst, I2C_CTL_SI); /* Clear SI */
			break;
		case 0x40u:                              /* Slave Address ACK */
			I2C_SET_CONTROL_REG(i2c->inst, I2C_CTL_SI_AA); /* Clear SI and set ACK */
			break;
		case 0x48u:                              /* Slave Address NACK */
			I2C_SET_CONTROL_REG(i2c->inst, I2C_CTL_STO_SI); /* Clear SI and send STOP*/
			i2c->is_err = 1u;
			i2c->is_busy = 0u;
			break;
		case 0x50u:
			i2c->p_rdbuf[i2c->indx] = (unsigned char) I2C_GET_DATA(i2c->inst);    /* Receive Data */
			i2c->nrd_bytes--;
			i2c->indx++;
			if(i2c->nrd_bytes > 1)
				I2C_SET_CONTROL_REG(i2c->inst, I2C_CTL_SI_AA); /* Clear SI and set ACK */
			else
				I2C_SET_CONTROL_REG(i2c->inst, I2C_CTL_SI); /* Clear SI */
			break;
		case 0x58u:
			i2c->p_rdbuf[i2c->indx] = (unsigned char) I2C_GET_DATA(i2c->inst);    /* Receive Data */
			i2c->is_busy = 0u;
			I2C_SET_CONTROL_REG(i2c->inst, I2C_CTL_STO_SI); /* Clear SI and send STOP*/
			break;
		case 0x38u:                              /* Arbitration Lost */
		default:                                 /* Unknow status */
			I2C_SET_CONTROL_REG(i2c->inst, I2C_CTL_STO_SI); /* Clear SI and send STOP*/
			i2c->is_err = 1u;
			i2c->is_busy = 0u;
			break;
	}
}

void i2c_isr_write_read(i2c_struct_t* i2c, uint32_t status) {
	switch(status) {
		case 0x08u:
			/* Write SLA+W to Register I2CDAT */
			I2C_SET_DATA(i2c->inst, (uint8_t)(i2c->slave_addr << 1u | 0x00u));
			I2C_SET_CONTROL_REG(i2c->inst, I2C_CTL_SI); /* Clear SI */
			break;
		case 0x18u: /* Slave Address ACK */
		case 0x28u:
			if(i2c->nwr_bytes) {
				I2C_SET_DATA(i2c->inst, i2c->p_wrbuf[i2c->indx]); /* Write Data to I2CDAT */
				i2c->nwr_bytes--;
				i2c->indx++;
				I2C_SET_CONTROL_REG(i2c->inst, I2C_CTL_SI); /* Clear SI */
			}
			else {
				I2C_SET_CONTROL_REG(i2c->inst, I2C_CTL_STA_SI); /* Send repeat START */
				i2c->indx = 0;
			}
			break;
		case 0x20u:                          /* Slave Address NACK */
		case 0x30u:                          /* Master transmit data NACK */
			I2C_SET_CONTROL_REG(i2c->inst, I2C_CTL_STO_SI); /* Clear SI and send STOP*/
			i2c->is_err = 1u;
			break;
		case 0x40u:                          /* Slave Address ACK */
				I2C_SET_CONTROL_REG(i2c->inst, I2C_CTL_SI_AA); /* Clear SI and set ACK */
			break;
		case 0x48u:                          /* Slave Address NACK */
			I2C_SET_CONTROL_REG(i2c->inst, I2C_CTL_STO_SI); /* Clear SI and send STOP*/
			i2c->is_err = 1u;
			i2c->is_busy = 0u;
			break;
		case 0x50u:
			i2c->p_rdbuf[i2c->indx] = (unsigned char) I2C_GET_DATA(i2c->inst);    /* Receive Data */
			i2c->nrd_bytes--;
			i2c->indx++;
			if(i2c->nrd_bytes > 1)
				I2C_SET_CONTROL_REG(i2c->inst, I2C_CTL_SI_AA); /* Clear SI and set ACK */
			else
				I2C_SET_CONTROL_REG(i2c->inst, I2C_CTL_SI); /* Clear SI */
			break;
		case 0x58u:
			i2c->p_rdbuf[i2c->indx] = (unsigned char) I2C_GET_DATA(i2c->inst);    /* Receive Data */
			I2C_SET_CONTROL_REG(i2c->inst, I2C_CTL_STO_SI); /* Clear SI and send STOP*/
			i2c->is_busy = 0u;
			break;
		case 0x38u:                              /* Arbitration Lost */
		default:                                 /* Unknow status */
			I2C_SET_CONTROL_REG(i2c->inst, I2C_CTL_STO_SI); /* Clear SI and send STOP*/
			i2c->is_err = 1u;
			i2c->is_busy = 0u;
			break;
	}
}
