#ifndef DRVR_IIC_H__
#define DRVR_IIC_H__

#include "common_header.h"
#include "i2c.h"

#define I2C_NOP 0
#define I2C_WRITE 1
#define I2C_READ 2
#define I2C_WRITE_READ 3

typedef struct {
	volatile uint8_t is_busy;
	uint8_t is_err;
	I2C_T* inst;
	uint8_t slave_addr;
	uint8_t* p_wrbuf;  // both reg addr and data should be written in this
	uint16_t  nwr_bytes;
	uint8_t* p_rdbuf;
	uint16_t  nrd_bytes;
	uint8_t todo;
	uint32_t timeout;
	uint8_t  indx;
} i2c_struct_t;

typedef void (*I2C_FUNC)(i2c_struct_t* i2c, uint32_t u32Status);

void i2c_ctor(i2c_struct_t* i2c);
uint8_t i2c_write(i2c_struct_t* i2c, uint8_t* p_wrbuf, uint16_t wrlen);
uint8_t i2c_read(i2c_struct_t* i2c, uint8_t* p_rdbuf, uint16_t rdlen);
uint8_t i2c_write_read(i2c_struct_t* i2c, uint8_t* p_wrbuf, uint16_t wrlen, uint8_t* p_rdbuf, uint16_t rdlen);

#endif
