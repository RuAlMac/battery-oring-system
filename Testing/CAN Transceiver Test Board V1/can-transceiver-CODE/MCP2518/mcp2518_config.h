#ifndef MCP2518_CONFIG_H
#define MCP2518_CONFIG_H

/******************<Summary>*****************
 * Name: mcp2518_config.h
 * Purpose:
 *      > Setting up MCP2518 for operation
 * Written / updated by:
 *      > Pier836 (https://github.com/Pier836/MCP2518FD-on-Raspberry-Pico/blob/main/drv_canfdspi_register.h)
 *      > Russell
******************</Summary>*****************/


/******************<Dependencies>*****************/
#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"

#include "communication/pico_spi.h"

#include "pins.h"
/******************</Dependencies>*****************/



/******************<Defines>*****************/
#define MAX_TXQUEUE_ATTEMPTS            50      // max number times MCP2518 tries to transmit data

// Message IDs
#define TX_REQUEST_ID                   0x300
#define TX_RESPONSE_ID                  0x301
#define PAYLOAD_ID                      0x101

// Receive Channels
#define APP_RX_FIFO                     CAN_FIFO_CH1

// Transmit Channels
#define APP_TX_FIFO                     CAN_FIFO_CH2

// Size of SPI transmit/receive buffers
#define SPI_DEFAULT_BUFFER_LENGTH       96

#define Nop() asm("nop")
/******************</Defines>*****************/



/******************<Operation States>*****************/

/*******operation_state*******
 * Description
    > defines the valid operational states for the MCP2518
    > these states determine transceiver behavior
*/
typedef enum {
    // Initialization
    STATE_INIT = 0,
    STATE_INIT_TXOBJ,

    // Transmit and Receive
    STATE_TRANSMIT,
    STATE_RECEIVE,

    // Test SPI access
    STATE_TEST_RAM_ACCESS,
    STATE_TEST_REGISTER_ACCESS,

    // Switch monitoring
    STATE_SWITCH_CHANGED
} APP_STATES;
/******************</Operation States>*****************/



/******************<Structs>*****************/

/*******mcp2518_config*******
 * Description
    > stores data relevant to MCP2518 operation
 * Data
    > spi_inst_t* spi_bus: SPI bus that the device is connected to
    > uint baudrate: baudrate to use for SPI communication
    > uint8_t miso_pin: pin used for contorller in, peripheral out
    > uint8_t mosi_pin: pin used for controller out, peripheral in
    > uint8_t sck_pin: pin used for serial clock
    > uint8_t cs_pin: pin used for chip select
    > uint8_t int_pin: pin used for hardware interrupt
    > operation_state_t active_state: the operation state the transceiver is actively using
*/
typedef struct {
    spi_inst_t* spi_bus;
    uint baudrate;
    uint8_t miso_pin;
    uint8_t mosi_pin;
    uint8_t sck_pin;
    uint8_t cs_pin;
    uint8_t int_pin;
} mcp2518_config;


typedef struct {
	bool S1;
} APP_SwitchState;


typedef struct {
	bool On;
	uint8_t Dlc;
	bool Mode;
	uint8_t Counter;
	uint8_t Delay;
	bool BRS;
} APP_Payload;


typedef struct {
    /* The application's current state */
    APP_STATES state;

    /* TODO: Define any additional data used by the application. */


} APP_DATA;

/******************</Structs>*****************/


#endif // MCP2518_CONFIG_H