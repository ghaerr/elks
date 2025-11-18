// DS3231 RTC device driver
//
// 14. Nov. 2025 swausd

#include "ds3231.h"
#include "i2c-ll.h"


// Initialize the I2C bus
void ds3231_init(void)
{
   i2c_init();

   ds3231_write(REG_CNTRL, 0x00);
   ds3231_write(REG_STAT,  0x08);
}


// Read from a DS3231 register
unsigned int ds3231_read(unsigned int clock_reg)
{
   unsigned int val = 0;

   i2c_start();

   i2c_write(DS3231_WRITE);

   if (i2c_ack())
   {
      i2c_write(clock_reg);

      if (i2c_ack())
      {
         i2c_start();
         i2c_write(DS3231_READ);

         if (i2c_ack())
            val = i2c_read();
      }
   }

   i2c_nak();
   i2c_stop();

   return val;
}


// Write to a DS3231 register
void ds3231_write(unsigned int clock_reg, unsigned int val)
{
   i2c_start();

   i2c_write(DS3231_WRITE);

   if (i2c_ack())
   {
      i2c_write(clock_reg);

      if (i2c_ack())
      {
         i2c_write(val);
         i2c_ack();
      }
   }

   i2c_stop();
}
