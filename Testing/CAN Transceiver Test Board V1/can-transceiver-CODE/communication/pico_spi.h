#ifndef PICO_SPI_H
#define PICO_SPI_H

/******************<Summary>*****************
 * Name: pico_spi.h
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
void init_pico_spi(spi_inst_t* spi_bus, uint baudrate, uint8_t miso_pin, uint8_t mosi_pin, uint8_t sck_pin);
int8_t pico_spi_transfer_data(spi_inst_t* spi_bus, uint8_t cs_pin, uint8_t* tx_data, uint8_t* rx_data, uint16_t transfer_size);
/******************</Functions>*****************/



#endif // PICO_SPI_H