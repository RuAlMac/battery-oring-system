#ifndef MCP2518_API_H
#define MCP2518_API_H

/******************<Summary>*****************
 * Name: mcp2518_api.h
 * Purpose:
 *      > Functions for communicating with MCP2518
 * Written / updated by:
 *      > Pier836 (https://github.com/Pier836/MCP2518FD-on-Raspberry-Pico/blob/main/drv_canfdspi_register.h)
 *      > Russell
******************</Summary>*****************/


/******************<Dependencies>*****************/
#include "MCP2518/mcp2518_config.h"
#include "MCP2518/mcp2518_registers.h"
#include "MCP2518/mcp2518_defines.h"

#include "communication/pico_spi.h"
#include "pins.h"
/******************</Dependencies>*****************/



/******************<Defines>*****************/
#define CRCBASE    0xFFFF
#define CRCUPPER   1
/******************</Defines>*****************/



int8_t DRV_CANFDSPI_Reset(mcp2518_config* spi_config);


// *****************************************************************************
// *****************************************************************************
// Section: SPI Access Functions

// *****************************************************************************
//! SPI Read Byte

int8_t DRV_CANFDSPI_ReadByte(mcp2518_config* spi_config, uint16_t address,
        uint8_t *rxd);

// *****************************************************************************
//! SPI Write Byte

int8_t DRV_CANFDSPI_WriteByte(mcp2518_config* spi_config, uint16_t address,
        uint8_t txd);

// *****************************************************************************
//! SPI Read Word

int8_t DRV_CANFDSPI_ReadWord(mcp2518_config* spi_config, uint16_t address,
        uint32_t *rxd);

// *****************************************************************************
//! SPI Write Word

int8_t DRV_CANFDSPI_WriteWord(mcp2518_config* spi_config, uint16_t address,
        uint32_t txd);

/// *****************************************************************************
//! SPI Read Word

int8_t DRV_CANFDSPI_ReadHalfWord(mcp2518_config* spi_config, uint16_t address,
        uint16_t *rxd);

// *****************************************************************************
//! SPI Write Word

int8_t DRV_CANFDSPI_WriteHalfWord(mcp2518_config* spi_config, uint16_t address,
        uint16_t txd);

// *****************************************************************************
//! SPI Read Byte Array

int8_t DRV_CANFDSPI_ReadByteArray(mcp2518_config* spi_config, uint16_t address,
        uint8_t *rxd, uint16_t nBytes);

// *****************************************************************************
//! SPI Write Byte Array

int8_t DRV_CANFDSPI_WriteByteArray(mcp2518_config* spi_config, uint16_t address,
        uint8_t *txd, uint16_t nBytes);

// *****************************************************************************
//! SPI SFR Write Byte Safe
/*!
 * Writes Byte to SFR at address using SPI CRC. Byte gets only written if CRC matches.
 *
 * Remark: The function doesn't check if the address is an SFR address.
 */

int8_t DRV_CANFDSPI_WriteByteSafe(mcp2518_config* spi_config, uint16_t address,
        uint8_t txd);

// *****************************************************************************
//! SPI RAM Write Word Safe
/*!
 * Writes Word to RAM at address using SPI CRC. Word gets only written if CRC matches.
 *
 * Remark: The function doesn't check if the address is a RAM address.
 */

int8_t DRV_CANFDSPI_WriteWordSafe(mcp2518_config* spi_config, uint16_t address,
        uint32_t txd);

// *****************************************************************************
//! SPI Read Byte Array with CRC

int8_t DRV_CANFDSPI_ReadByteArrayWithCRC(mcp2518_config* spi_config, uint16_t address,
        uint8_t *rxd, uint16_t nBytes, bool fromRam, bool* crcIsCorrect);

// *****************************************************************************
//! SPI Write Byte Array with CRC

int8_t DRV_CANFDSPI_WriteByteArrayWithCRC(mcp2518_config* spi_config, uint16_t address,
        uint8_t *txd, uint16_t nBytes, bool fromRam);

// *****************************************************************************
//! SPI Read Word Array

int8_t DRV_CANFDSPI_ReadWordArray(mcp2518_config* spi_config, uint16_t address,
        uint32_t *rxd, uint16_t nWords);

// *****************************************************************************
//! SPI Write Word Array

int8_t DRV_CANFDSPI_WriteWordArray(mcp2518_config* spi_config, uint16_t address,
        uint32_t *txd, uint16_t nWords);


// *****************************************************************************
// *****************************************************************************
// Section: Configuration

// *****************************************************************************
//! CAN Control register configuration

int8_t DRV_CANFDSPI_Configure(mcp2518_config* spi_config, CAN_CONFIG* config);

// *****************************************************************************
//! Reset Configure object to reset values
int8_t DRV_CANFDSPI_ConfigureObjectReset(CAN_CONFIG* config);


// *****************************************************************************
// *****************************************************************************
// Section: Operating mode

// *****************************************************************************
//! Select Operation Mode

int8_t DRV_CANFDSPI_OperationModeSelect(mcp2518_config* spi_config,
        CAN_OPERATION_MODE opMode);

// *****************************************************************************
//! Get Operation Mode

CAN_OPERATION_MODE DRV_CANFDSPI_OperationModeGet(mcp2518_config* spi_config);

// *****************************************************************************
//! Enable Low Power Mode

int8_t DRV_CANFDSPI_LowPowerModeEnable(mcp2518_config* spi_config);

// *****************************************************************************
//! Disable Low Power Mode

int8_t DRV_CANFDSPI_LowPowerModeDisable(mcp2518_config* spi_config);


// *****************************************************************************
// *****************************************************************************
// Section: CAN Transmit

// *****************************************************************************
//! Configure Transmit FIFO

int8_t DRV_CANFDSPI_TransmitChannelConfigure(mcp2518_config* spi_config,
        CAN_FIFO_CHANNEL channel, CAN_TX_FIFO_CONFIG* config);

// *****************************************************************************
//! Reset TransmitChannelConfigure object to reset values

int8_t DRV_CANFDSPI_TransmitChannelConfigureObjectReset(CAN_TX_FIFO_CONFIG* config);

// *****************************************************************************
//! Configure Transmit Queue

int8_t DRV_CANFDSPI_TransmitQueueConfigure(mcp2518_config* spi_config,
        CAN_TX_QUEUE_CONFIG* config);

// *****************************************************************************
//! Reset TransmitQueueConfigure object to reset values

int8_t DRV_CANFDSPI_TransmitQueueConfigureObjectReset(CAN_TX_QUEUE_CONFIG* config);

// *****************************************************************************
//! TX Channel Load
/*!
 * Loads data into Transmit channel
 * Requests transmission, if flush==true
 */

int8_t DRV_CANFDSPI_TransmitChannelLoad(mcp2518_config* spi_config,
        CAN_FIFO_CHANNEL channel, CAN_TX_MSGOBJ* txObj,
        uint8_t *txd, uint32_t txdNumBytes, bool flush);

// *****************************************************************************
//! TX Queue Load

/*!
 * Loads data into Transmit Queue
 * Requests transmission, if flush==true
 */

static inline int8_t DRV_CANFDSPI_TransmitQueueLoad(mcp2518_config* spi_config,
        CAN_TX_MSGOBJ* txObj,
        uint8_t *txd, uint32_t txdNumBytes, bool flush)
{
    return DRV_CANFDSPI_TransmitChannelLoad(spi_config, CAN_TXQUEUE_CH0, txObj, txd, txdNumBytes, flush);
}

// *****************************************************************************
//! TX Channel Flush
/*!
 * Set TXREG of one channel
 */

int8_t DRV_CANFDSPI_TransmitChannelFlush(mcp2518_config* spi_config,
        CAN_FIFO_CHANNEL channel);

// *****************************************************************************
//! Transmit Channel Status Get

int8_t DRV_CANFDSPI_TransmitChannelStatusGet(mcp2518_config* spi_config,
        CAN_FIFO_CHANNEL channel, CAN_TX_FIFO_STATUS* status);

// *****************************************************************************
//! Transmit FIFO Reset

int8_t DRV_CANFDSPI_TransmitChannelReset(mcp2518_config* spi_config,
        CAN_FIFO_CHANNEL channel);

// *****************************************************************************
//! Transmit FIFO Update
/*!
 * Sets UINC of the transmit channel.
 * Requests transmission, if flush==true
 */

int8_t DRV_CANFDSPI_TransmitChannelUpdate(mcp2518_config* spi_config,
        CAN_FIFO_CHANNEL channel, bool flush);

// *****************************************************************************
//! TX Queue Flush

/*!
 * Set TXREG of one channel
 */

static inline int8_t DRV_CANFDSPI_TransmitQueueFlush(mcp2518_config* spi_config)
{
    return DRV_CANFDSPI_TransmitChannelFlush(spi_config, CAN_TXQUEUE_CH0);
}

// *****************************************************************************
//! Transmit Queue Status Get

static inline int8_t DRV_CANFDSPI_TransmitQueueStatusGet(mcp2518_config* spi_config,
        CAN_TX_FIFO_STATUS* status)
{
    return DRV_CANFDSPI_TransmitChannelStatusGet(spi_config, CAN_TXQUEUE_CH0, status);
}

// *****************************************************************************
//! Transmit Queue Reset

static inline int8_t DRV_CANFDSPI_TransmitQueueReset(mcp2518_config* spi_config)
{
    return DRV_CANFDSPI_TransmitChannelReset(spi_config, CAN_TXQUEUE_CH0);
}

// *****************************************************************************
//! Transmit Queue Update

/*!
 * Sets UINC of the transmit channel.
 * Requests transmission, if flush==true
 */

static inline int8_t DRV_CANFDSPI_TransmitQueueUpdate(mcp2518_config* spi_config, bool flush)
{
    return DRV_CANFDSPI_TransmitChannelUpdate(spi_config, CAN_TXQUEUE_CH0, flush);
}

// *****************************************************************************
//! Request transmissions using TXREQ register

int8_t DRV_CANFDSPI_TransmitRequestSet(mcp2518_config* spi_config,
        CAN_TXREQ_CHANNEL txreq);

// *****************************************************************************
//! Get TXREQ register

int8_t DRV_CANFDSPI_TransmitRequestGet(mcp2518_config* spi_config,
        uint32_t* txreq);

// *****************************************************************************
//! Abort transmission of single FIFO

int8_t DRV_CANFDSPI_TransmitChannelAbort(mcp2518_config* spi_config,
        CAN_FIFO_CHANNEL channel);

// *****************************************************************************
//! Abort transmission of TXQ

static inline int8_t DRV_CANFDSPI_TransmitQueueAbort(mcp2518_config* spi_config)
{
    return DRV_CANFDSPI_TransmitChannelAbort(spi_config, CAN_TXQUEUE_CH0);
}

// *****************************************************************************
//! Abort All transmissions

int8_t DRV_CANFDSPI_TransmitAbortAll(mcp2518_config* spi_config);

// *****************************************************************************
//! Set Transmit Bandwidth Sharing Delay
int8_t DRV_CANFDSPI_TransmitBandWidthSharingSet(mcp2518_config* spi_config,
        CAN_TX_BANDWITH_SHARING txbws);


// *****************************************************************************
// *****************************************************************************
// Section: CAN Receive

// *****************************************************************************
//! Filter Object Configuration
/*!
 * Configures ID of filter object
 */

int8_t DRV_CANFDSPI_FilterObjectConfigure(mcp2518_config* spi_config,
        CAN_FILTER filter, CAN_FILTEROBJ_ID* id);

// *****************************************************************************
//! Filter Mask Configuration
/*!
 * Configures Mask of filter object
 */

int8_t DRV_CANFDSPI_FilterMaskConfigure(mcp2518_config* spi_config,
        CAN_FILTER filter, CAN_MASKOBJ_ID* mask);

// *****************************************************************************
//! Link Filter to FIFO
/*!
 * Initializes the Pointer from Filter to FIFO
 * Enables or disables the Filter
 */

int8_t DRV_CANFDSPI_FilterToFifoLink(mcp2518_config* spi_config,
        CAN_FILTER filter, CAN_FIFO_CHANNEL channel, bool enable);

// *****************************************************************************
//! Filter Enable

int8_t DRV_CANFDSPI_FilterEnable(mcp2518_config* spi_config, CAN_FILTER filter);

// *****************************************************************************
//! Filter Disable

int8_t DRV_CANFDSPI_FilterDisable(mcp2518_config* spi_config, CAN_FILTER filter);

// *****************************************************************************
//! Set Device Net Filter Count
int8_t DRV_CANFDSPI_DeviceNetFilterCountSet(mcp2518_config* spi_config,
        CAN_DNET_FILTER_SIZE dnfc);

// *****************************************************************************
//! Configure Receive FIFO

int8_t DRV_CANFDSPI_ReceiveChannelConfigure(mcp2518_config* spi_config,
        CAN_FIFO_CHANNEL channel, CAN_RX_FIFO_CONFIG* config);

// *****************************************************************************
//! Reset ReceiveChannelConfigure object to reset value

int8_t DRV_CANFDSPI_ReceiveChannelConfigureObjectReset(CAN_RX_FIFO_CONFIG* config);

// *****************************************************************************
//! Receive Channel Status Get

int8_t DRV_CANFDSPI_ReceiveChannelStatusGet(mcp2518_config* spi_config,
        CAN_FIFO_CHANNEL channel, CAN_RX_FIFO_STATUS* status);

// *****************************************************************************
//! Get Received Message
/*!
 * Reads Received message from channel
 */

int8_t DRV_CANFDSPI_ReceiveMessageGet(mcp2518_config* spi_config,
        CAN_FIFO_CHANNEL channel, CAN_RX_MSGOBJ* rxObj,
        uint8_t *rxd, uint8_t nBytes);

// *****************************************************************************
//! Receive FIFO Reset

int8_t DRV_CANFDSPI_ReceiveChannelReset(mcp2518_config* spi_config,
        CAN_FIFO_CHANNEL channel);

// *****************************************************************************
//! Receive FIFO Update
/*!
 * Sets UINC of the receive channel.
 */

int8_t DRV_CANFDSPI_ReceiveChannelUpdate(mcp2518_config* spi_config,
        CAN_FIFO_CHANNEL channel);


// *****************************************************************************
// *****************************************************************************
// Section: Transmit Event FIFO

// *****************************************************************************
//! Transmit Event FIFO Status Get

int8_t DRV_CANFDSPI_TefStatusGet(mcp2518_config* spi_config,
        CAN_TEF_FIFO_STATUS* status);

// *****************************************************************************
//! Get Transmit Event FIFO Message
/*!
 * Reads Transmit Event FIFO message
 */

int8_t DRV_CANFDSPI_TefMessageGet(mcp2518_config* spi_config,
        CAN_TEF_MSGOBJ* tefObj);

// *****************************************************************************
//! Transmit Event FIFO Reset

int8_t DRV_CANFDSPI_TefReset(mcp2518_config* spi_config);

// *****************************************************************************
//! Transmit Event FIFO Update
/*!
 * Sets UINC of the TEF.
 */

int8_t DRV_CANFDSPI_TefUpdate(mcp2518_config* spi_config);

// *****************************************************************************
//! Configure Transmit Event FIFO

int8_t DRV_CANFDSPI_TefConfigure(mcp2518_config* spi_config, CAN_TEF_CONFIG* config);

// *****************************************************************************
//! Reset TefConfigure object to reset value

int8_t DRV_CANFDSPI_TefConfigureObjectReset(CAN_TEF_CONFIG* config);


// *****************************************************************************
// *****************************************************************************
// Section: Module Events

// *****************************************************************************
//! Module Event Get
/*!
 * Reads interrupt Flags
 */

int8_t DRV_CANFDSPI_ModuleEventGet(mcp2518_config* spi_config,
        CAN_MODULE_EVENT* flags);

// *****************************************************************************
//! Module Event Enable
/*!
 * Enables interrupts
 */

int8_t DRV_CANFDSPI_ModuleEventEnable(mcp2518_config* spi_config,
        CAN_MODULE_EVENT flags);

// *****************************************************************************
//! Module Event Disable
/*!
 * Disables interrupts
 */

int8_t DRV_CANFDSPI_ModuleEventDisable(mcp2518_config* spi_config,
        CAN_MODULE_EVENT flags);

// *****************************************************************************
//! Module Event Clear
/*!
 * Clears interrupt Flags
 */

int8_t DRV_CANFDSPI_ModuleEventClear(mcp2518_config* spi_config,
        CAN_MODULE_EVENT flags);

// *****************************************************************************
//! Get RX Code

int8_t DRV_CANFDSPI_ModuleEventRxCodeGet(mcp2518_config* spi_config,
        CAN_RXCODE* rxCode);

// *****************************************************************************
//! Get TX Code

int8_t DRV_CANFDSPI_ModuleEventTxCodeGet(mcp2518_config* spi_config,
        CAN_TXCODE* txCode);

// *****************************************************************************
//! Get Filter Hit

int8_t DRV_CANFDSPI_ModuleEventFilterHitGet(mcp2518_config* spi_config,
        CAN_FILTER* filterHit);

// *****************************************************************************
//! Get ICODE

int8_t DRV_CANFDSPI_ModuleEventIcodeGet(mcp2518_config* spi_config,
        CAN_ICODE* icode);

// *****************************************************************************
// *****************************************************************************
// Section: Transmit FIFO Events

// *****************************************************************************
//! Transmit FIFO Event Get
/*!
 * Reads Transmit FIFO interrupt Flags
 */

int8_t DRV_CANFDSPI_TransmitChannelEventGet(mcp2518_config* spi_config,
        CAN_FIFO_CHANNEL channel, CAN_TX_FIFO_EVENT* flags);

// *****************************************************************************
//! Transmit Queue Event Get

/*!
 * Reads Transmit Queue interrupt Flags
 */

static inline int8_t DRV_CANFDSPI_TransmitQueueEventGet(mcp2518_config* spi_config,
        CAN_TX_FIFO_EVENT* flags)
{
    return DRV_CANFDSPI_TransmitChannelEventGet(spi_config, CAN_TXQUEUE_CH0, flags);
}

// *****************************************************************************
//! Get pending interrupts of all transmit FIFOs

int8_t DRV_CANFDSPI_TransmitEventGet(mcp2518_config* spi_config, uint32_t* txif);

// *****************************************************************************
//! Get pending TXATIF of all transmit FIFOs

int8_t DRV_CANFDSPI_TransmitEventAttemptGet(mcp2518_config* spi_config,
        uint32_t* txatif);

// *****************************************************************************
//! Transmit FIFO spi_config->cs_pin Get
/*!
 * Reads Transmit FIFO spi_config->cs_pin
 */

int8_t DRV_CANFDSPI_TransmitChannelIndexGet(mcp2518_config* spi_config,
        CAN_FIFO_CHANNEL channel, uint8_t* idx);

// *****************************************************************************
//! Transmit FIFO Event Enable
/*!
 * Enables Transmit FIFO interrupts
 */

int8_t DRV_CANFDSPI_TransmitChannelEventEnable(mcp2518_config* spi_config,
        CAN_FIFO_CHANNEL channel, CAN_TX_FIFO_EVENT flags);

// *****************************************************************************
//! Transmit FIFO Event Disable
/*!
 * Disables Transmit FIFO interrupts
 */

int8_t DRV_CANFDSPI_TransmitChannelEventDisable(mcp2518_config* spi_config,
        CAN_FIFO_CHANNEL channel, CAN_TX_FIFO_EVENT flags);

// *****************************************************************************
//! Transmit FIFO Event Clear
/*!
 * Clears Transmit FIFO Attempts Exhausted interrupt Flag
 */

int8_t DRV_CANFDSPI_TransmitChannelEventAttemptClear(mcp2518_config* spi_config,
        CAN_FIFO_CHANNEL channel);


// *****************************************************************************
//! Transmit Queue spi_config->cs_pin Get

/*!
 * Reads Transmit Queue spi_config->cs_pin
 */

static inline int8_t DRV_CANFDSPI_TransmitQueueIndexGet(mcp2518_config* spi_config, uint8_t* idx)
{
    return DRV_CANFDSPI_TransmitChannelIndexGet(spi_config, CAN_TXQUEUE_CH0, idx);
}

// *****************************************************************************
//! Transmit Queue Event Enable

/*!
 * Enables Transmit Queue interrupts
 */

static inline int8_t DRV_CANFDSPI_TransmitQueueEventEnable(mcp2518_config* spi_config,
        CAN_TX_FIFO_EVENT flags)
{
    return DRV_CANFDSPI_TransmitChannelEventEnable(spi_config, CAN_TXQUEUE_CH0, flags);
}

// *****************************************************************************
//! Transmit Queue Event Disable

/*!
 * Disables Transmit FIFO interrupts
 */

static inline int8_t DRV_CANFDSPI_TransmitQueueEventDisable(mcp2518_config* spi_config,
        CAN_TX_FIFO_EVENT flags)
{
    return DRV_CANFDSPI_TransmitChannelEventDisable(spi_config, CAN_TXQUEUE_CH0, flags);
}

// *****************************************************************************
//! Transmit Queue Event Clear

/*!
 * Clears Transmit FIFO Attempts Exhausted interrupt Flag
 */

static inline int8_t DRV_CANFDSPI_TransmitQueueEventAttemptClear(mcp2518_config* spi_config)
{
    return DRV_CANFDSPI_TransmitChannelEventAttemptClear(spi_config, CAN_TXQUEUE_CH0);
}


// *****************************************************************************
// *****************************************************************************
// Section: Receive FIFO Events

// *****************************************************************************
//! Receive FIFO Event Get
/*!
 * Reads Receive FIFO interrupt Flags
 */

int8_t DRV_CANFDSPI_ReceiveChannelEventGet(mcp2518_config* spi_config,
        CAN_FIFO_CHANNEL channel, CAN_RX_FIFO_EVENT* flags);

// *****************************************************************************
//! Get pending interrupts of all receive FIFOs

int8_t DRV_CANFDSPI_ReceiveEventGet(mcp2518_config* spi_config, uint32_t* rxif);

// *****************************************************************************
//!Get pending RXOVIF of all receive FIFOs

int8_t DRV_CANFDSPI_ReceiveEventOverflowGet(mcp2518_config* spi_config, uint32_t* rxovif);

// *****************************************************************************
//! Receive FIFO spi_config->cs_pin Get
/*!
 * Reads Receive FIFO spi_config->cs_pin
 */

int8_t DRV_CANFDSPI_ReceiveChannelIndexGet(mcp2518_config* spi_config,
        CAN_FIFO_CHANNEL channel, uint8_t* idx);

// *****************************************************************************
//! Receive FIFO Event Enable
/*!
 * Enables Receive FIFO interrupts
 */

int8_t DRV_CANFDSPI_ReceiveChannelEventEnable(mcp2518_config* spi_config,
        CAN_FIFO_CHANNEL channel, CAN_RX_FIFO_EVENT flags);

// *****************************************************************************
//! Receive FIFO Event Disable
/*!
 * Disables Receive FIFO interrupts
 */

int8_t DRV_CANFDSPI_ReceiveChannelEventDisable(mcp2518_config* spi_config,
        CAN_FIFO_CHANNEL channel, CAN_RX_FIFO_EVENT flags);

// *****************************************************************************
//! Receive FIFO Event Clear
/*!
 * Clears Receive FIFO Overflow interrupt Flag
 */

int8_t DRV_CANFDSPI_ReceiveChannelEventOverflowClear(mcp2518_config* spi_config,
        CAN_FIFO_CHANNEL channel);


// *****************************************************************************
// *****************************************************************************
// Section: Transmit Event FIFO Events

// *****************************************************************************
//! Transmit Event FIFO Event Get
/*!
 * Reads Transmit Event FIFO interrupt Flags
 */

int8_t DRV_CANFDSPI_TefEventGet(mcp2518_config* spi_config,
        CAN_TEF_FIFO_EVENT* flags);

// *****************************************************************************
//! Transmit Event FIFO Event Enable
/*!
 * Enables Transmit Event FIFO interrupts
 */

int8_t DRV_CANFDSPI_TefEventEnable(mcp2518_config* spi_config,
        CAN_TEF_FIFO_EVENT flags);

// *****************************************************************************
//! Transmit Event FIFO Event Disable
/*!
 * Disables Transmit Event FIFO interrupts
 */

int8_t DRV_CANFDSPI_TefEventDisable(mcp2518_config* spi_config,
        CAN_TEF_FIFO_EVENT flags);

// *****************************************************************************
//! Transmit Event FIFO Event Clear
/*!
 * Clears Transmit Event FIFO Overflow interrupt Flag
 */

int8_t DRV_CANFDSPI_TefEventOverflowClear(mcp2518_config* spi_config);


// *****************************************************************************
// *****************************************************************************
// Section: Error Handling

// *****************************************************************************
//! Transmit Error Count Get

int8_t DRV_CANFDSPI_ErrorCountTransmitGet(mcp2518_config* spi_config,
        uint8_t* tec);

// *****************************************************************************
//! Receive Error Count Get

int8_t DRV_CANFDSPI_ErrorCountReceiveGet(mcp2518_config* spi_config,
        uint8_t* rec);

// *****************************************************************************
//! Error State Get

int8_t DRV_CANFDSPI_ErrorStateGet(mcp2518_config* spi_config,
        CAN_ERROR_STATE* flags);

// *****************************************************************************
//! Error Counts and Error State Get
/*!
 * Returns content of complete CiTREC
 */

int8_t DRV_CANFDSPI_ErrorCountStateGet(mcp2518_config* spi_config,
        uint8_t* tec, uint8_t* rec, CAN_ERROR_STATE* flags);

// *****************************************************************************
//! Get Bus Diagnostic Registers: all data at once, since we want to keep them in synch

int8_t DRV_CANFDSPI_BusDiagnosticsGet(mcp2518_config* spi_config,
        CAN_BUS_DIAGNOSTIC* bd);

// *****************************************************************************
//! Clear Bus Diagnostic Registers

int8_t DRV_CANFDSPI_BusDiagnosticsClear(mcp2518_config* spi_config);


// *****************************************************************************
// *****************************************************************************
// Section: ECC

// *****************************************************************************
//! Enable ECC

int8_t DRV_CANFDSPI_EccEnable(mcp2518_config* spi_config);

// *****************************************************************************
//! Disable ECC

int8_t DRV_CANFDSPI_EccDisable(mcp2518_config* spi_config);

// *****************************************************************************
//! ECC Event Get

int8_t DRV_CANFDSPI_EccEventGet(mcp2518_config* spi_config,
        CAN_ECC_EVENT* flags);

// *****************************************************************************
//! Set ECC Parity

int8_t DRV_CANFDSPI_EccParitySet(mcp2518_config* spi_config,
        uint8_t parity);

// *****************************************************************************
//! Get ECC Parity

int8_t DRV_CANFDSPI_EccParityGet(mcp2518_config* spi_config,
        uint8_t* parity);

// *****************************************************************************
//! Get ECC Error Address

int8_t DRV_CANFDSPI_EccErrorAddressGet(mcp2518_config* spi_config,
        uint16_t* a);

// *****************************************************************************
//! ECC Event Enable

int8_t DRV_CANFDSPI_EccEventEnable(mcp2518_config* spi_config,
        CAN_ECC_EVENT flags);

// *****************************************************************************
//! ECC Event Disable

int8_t DRV_CANFDSPI_EccEventDisable(mcp2518_config* spi_config,
        CAN_ECC_EVENT flags);

// *****************************************************************************
//! ECC Event Clear

int8_t DRV_CANFDSPI_EccEventClear(mcp2518_config* spi_config,
        CAN_ECC_EVENT flags);

// *****************************************************************************
//! Initialize RAM

int8_t DRV_CANFDSPI_RamInit(mcp2518_config* spi_config, uint8_t d);


// *****************************************************************************
// *****************************************************************************
// Section: CRC

// *****************************************************************************
//! CRC Event Enable

int8_t DRV_CANFDSPI_CrcEventEnable(mcp2518_config* spi_config,
        CAN_CRC_EVENT flags);

// *****************************************************************************
//! CRC Event Disable

int8_t DRV_CANFDSPI_CrcEventDisable(mcp2518_config* spi_config,
        CAN_CRC_EVENT flags);

// *****************************************************************************
//! CRC Event Clear

int8_t DRV_CANFDSPI_CrcEventClear(mcp2518_config* spi_config,
        CAN_CRC_EVENT flags);

// *****************************************************************************
//! CRC Event Get

int8_t DRV_CANFDSPI_CrcEventGet(mcp2518_config* spi_config, CAN_CRC_EVENT* flags);

// *****************************************************************************
//! Get CRC Value from device

int8_t DRV_CANFDSPI_CrcValueGet(mcp2518_config* spi_config, uint16_t* crc);


// *****************************************************************************
// *****************************************************************************
// Section: Time Stamp

// *****************************************************************************
//! Time Stamp Enable

int8_t DRV_CANFDSPI_TimeStampEnable(mcp2518_config* spi_config);

// *****************************************************************************
//! Time Stamp Disable

int8_t DRV_CANFDSPI_TimeStampDisable(mcp2518_config* spi_config);

// *****************************************************************************
//! Time Stamp Get

int8_t DRV_CANFDSPI_TimeStampGet(mcp2518_config* spi_config, uint32_t* ts);

// *****************************************************************************
//! Time Stamp Set

int8_t DRV_CANFDSPI_TimeStampSet(mcp2518_config* spi_config, uint32_t ts);

// *****************************************************************************
//! Time Stamp Mode Configure

int8_t DRV_CANFDSPI_TimeStampModeConfigure(mcp2518_config* spi_config,
        CAN_TS_MODE mode);

// *****************************************************************************
//! Time Stamp Prescaler Set

int8_t DRV_CANFDSPI_TimeStampPrescalerSet(mcp2518_config* spi_config,
        uint16_t ps);


// *****************************************************************************
// *****************************************************************************
// Section: Oscillator and Bit Time

// *****************************************************************************
//! Enable oscillator to wake-up from sleep

int8_t DRV_CANFDSPI_OscillatorEnable(mcp2518_config* spi_config);

// *****************************************************************************
//! Set Oscillator Control

int8_t DRV_CANFDSPI_OscillatorControlSet(mcp2518_config* spi_config,
        CAN_OSC_CTRL ctrl);

int8_t DRV_CANFDSPI_OscillatorControlObjectReset(CAN_OSC_CTRL* ctrl);


// *****************************************************************************
//! Get Oscillator Status

int8_t DRV_CANFDSPI_OscillatorStatusGet(mcp2518_config* spi_config,
        CAN_OSC_STATUS* status);

// *****************************************************************************
//! Configure Bit Time registers (based on CAN clock speed)

int8_t DRV_CANFDSPI_BitTimeConfigure(mcp2518_config* spi_config,
        CAN_BITTIME_SETUP bitTime, CAN_SSP_MODE sspMode,
        CAN_SYSCLK_SPEED clk);

// *****************************************************************************
//! Configure Nominal bit time for 40MHz system clock

int8_t DRV_CANFDSPI_BitTimeConfigureNominal40MHz(mcp2518_config* spi_config,
        CAN_BITTIME_SETUP bitTime);

// *****************************************************************************
//! Configure Data bit time for 40MHz system clock

int8_t DRV_CANFDSPI_BitTimeConfigureData40MHz(mcp2518_config* spi_config,
        CAN_BITTIME_SETUP bitTime, CAN_SSP_MODE sspMode);

// *****************************************************************************
//! Configure Nominal bit time for 20MHz system clock

int8_t DRV_CANFDSPI_BitTimeConfigureNominal20MHz(mcp2518_config* spi_config,
        CAN_BITTIME_SETUP bitTime);

// *****************************************************************************
//! Configure Data bit time for 20MHz system clock

int8_t DRV_CANFDSPI_BitTimeConfigureData20MHz(mcp2518_config* spi_config,
        CAN_BITTIME_SETUP bitTime, CAN_SSP_MODE sspMode);

// *****************************************************************************
//! Configure Nominal bit time for 10MHz system clock

int8_t DRV_CANFDSPI_BitTimeConfigureNominal10MHz(mcp2518_config* spi_config,
        CAN_BITTIME_SETUP bitTime);

// *****************************************************************************
//! Configure Data bit time for 10MHz system clock

int8_t DRV_CANFDSPI_BitTimeConfigureData10MHz(mcp2518_config* spi_config,
        CAN_BITTIME_SETUP bitTime, CAN_SSP_MODE sspMode);


// *****************************************************************************
// *****************************************************************************
// Section: GPIO

// *****************************************************************************
//! Initialize GPIO Mode

int8_t DRV_CANFDSPI_GpioModeConfigure(mcp2518_config* spi_config,
        GPIO_PIN_MODE gpio0, GPIO_PIN_MODE gpio1);

// *****************************************************************************
//! Initialize GPIO Direction

int8_t DRV_CANFDSPI_GpioDirectionConfigure(mcp2518_config* spi_config,
        GPIO_PIN_DIRECTION gpio0, GPIO_PIN_DIRECTION gpio1);

// *****************************************************************************
//! Enable Transceiver Standby Control

int8_t DRV_CANFDSPI_GpioStandbyControlEnable(mcp2518_config* spi_config);

// *****************************************************************************
//! Disable Transceiver Standby Control

int8_t DRV_CANFDSPI_GpioStandbyControlDisable(mcp2518_config* spi_config);

// *****************************************************************************
//! Configure Open Drain Interrupts

int8_t DRV_CANFDSPI_GpioInterruptPinsOpenDrainConfigure(mcp2518_config* spi_config,
        GPIO_OPEN_DRAIN_MODE mode);

// *****************************************************************************
//! Configure Open Drain TXCAN

int8_t DRV_CANFDSPI_GpioTransmitPinOpenDrainConfigure(mcp2518_config* spi_config,
        GPIO_OPEN_DRAIN_MODE mode);

// *****************************************************************************
//! GPIO Output Pin Set

int8_t DRV_CANFDSPI_GpioPinSet(mcp2518_config* spi_config,
        GPIO_PIN_POS pos, GPIO_PIN_STATE latch);

// *****************************************************************************
//! GPIO Input Pin Read

int8_t DRV_CANFDSPI_GpioPinRead(mcp2518_config* spi_config,
        GPIO_PIN_POS pos, GPIO_PIN_STATE* state);

// *****************************************************************************
//! Configure CLKO Pin

int8_t DRV_CANFDSPI_GpioClockOutputConfigure(mcp2518_config* spi_config,
        GPIO_CLKO_MODE mode);


// *****************************************************************************
// *****************************************************************************
// Section: Miscellaneous

// *****************************************************************************
//! DLC to number of actual data bytes conversion

uint32_t DRV_CANFDSPI_DlcToDataBytes(CAN_DLC dlc);

// *****************************************************************************
//! FIFO spi_config->cs_pin Get

int8_t DRV_CANFDSPI_FifoGet(mcp2518_config* spi_config,
        CAN_FIFO_CHANNEL channel, uint8_t* mi);

// *****************************************************************************
//! Calculate CRC16

uint16_t DRV_CANFDSPI_CalculateCRC16(uint8_t* data, uint16_t size);

// *****************************************************************************
//! Data bytes to DLC conversion

CAN_DLC DRV_CANFDSPI_DataBytesToDlc(uint8_t n);


#endif // MCP2518_API_H