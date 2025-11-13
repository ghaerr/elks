#pragma once

#include <stdint.h>

/**
 * Inits the SPI hardware (peripheral or bitbang via GPIO).
 */
void spi_init_ll(void);

/**
 * De-asserts (sets to logic 0) the SPI CS line.
 */
void spi_cs_0(void);

/**
 * Asserts (sets to logic 1) the SPI CS line.
 */
void spi_cs_1(void);

/**
 * Sets the SPI clock line to logic 0.
 */
void spi_sck_0(void);

/**
 * Transmit an entire byte via SPI. Discards the slave's output data.
 */
void spi_transmit(uint8_t data);

/**
 * Read an entire byte while also sending zeros to the slave device.
 * 
 * returns: read byte from the device.
 */
uint8_t spi_receive(void);

/**
 * Sends N bytes filled of all logical ones (0xff).
 * 
 * bytes: count of bytes to send.
 */
void spi_send_ffs(uint16_t bytes);

/**
 * Reads one block from SD card via hardware SPI into buffer
 * 
 * buf: offset address of receive buffer
 * seg: segment adress of receive buffer
 * count: number of bytes to read into buffer (has to be 512)
 */
void spi_read_block(char *buf, ramdesc_t seg, uint16_t count);

/**
 * Write one block from buffer to SD card via hardware SPI
 * 
 * buf: offset address of write buffer
 * seg: segment adress of write buffer
 * count: number of bytes to write from buffer (has to be 512)
 */
void spi_write_block(char *buf, ramdesc_t seg, uint16_t count);
