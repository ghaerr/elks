// Low level I2C driver using bit-banging for DS3231 RTC
//
// 14. Nov. 2025 swausd

#pragma once

void i2c_init(void);
void i2c_start(void);
void i2c_stop(void);
void i2c_nak(void);
unsigned int i2c_ack(void);
unsigned int i2c_read(void);
void i2c_write(unsigned int data);
