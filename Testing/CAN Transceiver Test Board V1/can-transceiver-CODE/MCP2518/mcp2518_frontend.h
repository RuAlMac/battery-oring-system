
#ifndef MCP2518_FRONTEND_H
#define MCP2518_FRONTEND_H

/******************<Summary>*****************
 * Name: mcp2518_frontend.h
 * Purpose:
 *      > High-level functions for use of MCP2518
 *      > THESE ARE THE FUNCTIONS THAT SHOULD BE CALLED IN MAIN!
 * Written / updated by:
 *      > Pier836 (https://github.com/Pier836/MCP2518FD-on-Raspberry-Pico/blob/main/)
 *      > Russell
******************</Summary>*****************/


/******************<Dependencies>*****************/
#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"

#include "communication/pico_spi.h"

#include "MCP2518/mcp2518_config.h"
#include "MCP2518/mcp2518_api.h"

#include "pins.h"
/******************</Dependencies>*****************/



/******************<Defines>*****************/
/******************</Defines>*****************/



/******************<Public Functions>*****************/
void init_mcp2518(spi_inst_t* spi_bus, uint baudrate, uint8_t miso_pin, uint8_t mosi_pin, uint8_t sck_pin, uint8_t cs_pin, uint8_t int_pin, mcp2518_config* config);
/******************</Public Functions>*****************/


/*
void APP_CANFDSPI_Init(void);

//! Add message to transmit FIFO
void APP_TransmitMessageQueue(void);

//! Decode received messages
APP_STATES APP_ReceiveMessage_Tasks(void);

//! Transmit switch state
//void APP_TransmitSwitchState(void);

//! Periodically send message with requested payload
//void APP_PayLoad_Tasks(void);

//! Test SPI access
uint8_t APP_TestRegisterAccess(void);

//! Test RAM access
uint8_t APP_TestRamAccess(void);

*/

#endif // MCP2518_FRONTEND_H
