#ifndef RP2350_SPI_H
#define RP2350_SPI_H

/******************<Summary>*****************
 * Name: rp2350_spi.h
 * Purpose:
 *      > Functions for SPI communication
 * Written / updated by:
 *      > Russell
******************</Summary>*****************/


/******************<Dependencies>*****************/
#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"

#include "hardware/spi.h"

#include "pins.h"
/******************</Dependencies>*****************/


/******************<Defines>*****************/
/******************</Defines>*****************/


/******************<Functions>*****************/
void init_rp2350_spi_master(spi_inst_t* spi_bus, uint8_t miso_pin, uint8_t mosi_pin, uint8_t sck_pin, uint baudrate);
void rp2350_spi_write(spi_inst_t* spi_bus, uint8_t* src, uint8_t size);
/******************</Functions>*****************/



#endif // RP2350_SPI_H