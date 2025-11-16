// High level DS3231 RTC driver
//
// 14. Nov. 2025 swausd

#pragma once

/* I2C address and registers to access DS3231 */
#define RTC_ADDRESS        0x68                           // Slave address of ds3231

#define DS3231_READ        ((RTC_ADDRESS << 1) | 0x01)    // Read address for RTC
#define DS3231_WRITE       ((RTC_ADDRESS << 1) & 0xFE)    // Write address for RTC

#define REG_SEC            0x00
#define REG_MIN            0x01
#define REG_HOUR           0x02
#define REG_DAY            0x03
#define REG_DATE           0x04
#define REG_MONTH          0x05
#define REG_YEAR           0x06
#define REG_CNTRL          0x0E
#define REG_STAT           0x0F

void ds3231_init(void);
unsigned char ds3231_read(unsigned char clock_reg);
void ds3231_write(unsigned char clock_reg, unsigned char val);

