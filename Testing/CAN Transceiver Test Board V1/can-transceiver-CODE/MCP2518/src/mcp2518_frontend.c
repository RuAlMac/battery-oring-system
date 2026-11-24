
// Include files
#include "MCP2518/mcp2518_frontend.h"


// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************

APP_DATA appData;

CAN_CONFIG config_can1;
CAN_OPERATION_MODE opMode_can1;

// Transmit objects
CAN_TX_FIFO_CONFIG txConfig_can1;
CAN_TX_FIFO_EVENT txFlags_can1;
CAN_TX_MSGOBJ txObj_can1;
uint8_t txd_can1[MAX_DATA_BYTES];

// Receive objects
CAN_RX_FIFO_CONFIG rxConfig_can1;
REG_CiFLTOBJ fObj_can1;
REG_CiMASK mObj_can1;
CAN_RX_FIFO_EVENT rxFlags_can1;
CAN_RX_MSGOBJ rxObj_can1;
uint8_t rxd_can1[MAX_DATA_BYTES];
CAN_BUS_DIAGNOSTIC busDiagnostics_can1;
uint8_t tec_can1;
uint8_t rec_can1;
CAN_ERROR_STATE errorFlags_can1;

REG_t reg;

APP_SwitchState lastSwitchState;

APP_Payload payload;

uint8_t ledCount = 0, ledState = 0;

uint8_t i;


APP_STATES APP_ReceiveMessage_Tasks(void)
{
    APP_STATES nextState;

    // Normally we go to APP_STATE_TRANSMIT
    Nop();
    Nop();
    nextState = APP_STATE_TRANSMIT;
#ifdef ENABLE_CAN1
    // Check if FIFO is not empty
#ifdef APP_USE_RX_INT
    if (APP_RX_INT()) {

#else
    DRV_CANFDSPI_ReceiveChannelEventGet(CAN1, APP_RX_FIFO, &rxFlags_can1);

	if (rxFlags_can1 & CAN_RX_FIFO_NOT_EMPTY_EVENT) 
	{
#endif
		APP_LED_Set(APP_RX_LED);

		// Get message
		DRV_CANFDSPI_ReceiveMessageGet(CAN1, APP_RX_FIFO, &rxObj_can1, rxd_can1, MAX_DATA_BYTES);

		printf("CAN1 ID %02x ", rxObj_can1.bF.id.SID);
		printf("DLC %02x Data ", rxObj_can1.bF.ctrl.DLC);
		 for (i = 0; i < 8; i++) 
		 {
			printf("%02x ", rxd_can1[i]);
		 }
		 printf("\n");
	}

        APP_LED_Clear(APP_RX_LED);
#endif // ENABLE_CAN1	

#ifdef ENABLE_CAN2				
    // Check if FIFO is not empty
#ifdef APP_USE_RX_INT
    if (APP_RX_INT()) {

#else
    DRV_CANFDSPI_ReceiveChannelEventGet(CAN2, APP_RX_FIFO, &rxFlags_can2);

	if (rxFlags_can2 & CAN_RX_FIFO_NOT_EMPTY_EVENT) 
	{
#endif
		APP_LED_Set(APP_RX_LED);

		// Get message
		DRV_CANFDSPI_ReceiveMessageGet(CAN2, APP_RX_FIFO, &rxObj_can2, rxd_can2, MAX_DATA_BYTES);

		printf("CAN2 ID %02x ", rxObj_can2.bF.id.SID);
		printf("DLC %02x Data ", rxObj_can2.bF.ctrl.DLC);
		 for (i = 0; i < 8; i++) 
		 {
			printf("%02x ", rxd_can2[i]);
		 }
		 printf("\n");
	}

        APP_LED_Clear(APP_RX_LED);
#endif // ENABLE_CAN2	

    return nextState;
}

void APP_TransmitMessageQueue(void)
{
    APP_LED_Set(APP_TX_LED);

#ifdef ENABLE_CAN1
    uint8_t attempts_can1 = MAX_TXQUEUE_ATTEMPTS;

    // Check if FIFO is not full
    do {
        DRV_CANFDSPI_TransmitChannelEventGet(CAN1, APP_TX_FIFO, &txFlags_can1);
        if (attempts_can1 == 0) {
            Nop();
            Nop();
            DRV_CANFDSPI_ErrorCountStateGet(CAN1, &tec_can1, &rec_can1, &errorFlags_can1);
            return;
        }
        attempts_can1--;
    }
    while (!(txFlags_can1 & CAN_TX_FIFO_NOT_FULL_EVENT));

    // Load message and transmit
    uint8_t n_can1 = DRV_CANFDSPI_DlcToDataBytes(txObj_can1.bF.ctrl.DLC);

    DRV_CANFDSPI_TransmitChannelLoad(CAN1, APP_TX_FIFO, &txObj_can1, txd_can1, n_can1, true);
#endif //ENABLE_CAN1

#ifdef ENABLE_CAN2
    uint8_t attempts_can2 = MAX_TXQUEUE_ATTEMPTS;

    // Check if FIFO is not full
    do {
        DRV_CANFDSPI_TransmitChannelEventGet(CAN2, APP_TX_FIFO, &txFlags_can2);
        if (attempts_can2 == 0) {
            Nop();
            Nop();
            DRV_CANFDSPI_ErrorCountStateGet(CAN2, &tec_can2, &rec_can2, &errorFlags_can2);
            return;
        }
        attempts_can2--;
    }
    while (!(txFlags_can2 & CAN_TX_FIFO_NOT_FULL_EVENT));

    // Load message and transmit
    uint8_t n_can2 = DRV_CANFDSPI_DlcToDataBytes(txObj_can2.bF.ctrl.DLC);

    DRV_CANFDSPI_TransmitChannelLoad(CAN2, APP_TX_FIFO, &txObj_can2, txd_can2, n_can2, true);
#endif //ENABLE_CAN2
    APP_LED_Clear(APP_TX_LED);
}

/*void APP_TransmitSwitchState(void)
{
    APP_SwitchState newSwitchState;

    // Check if switch has changed
    Nop();
    //Pier dovrebbe essere la gestione dell'interrupt
   // newSwitchState.S1 = APP_S1_READ();

    bool switchChanged = newSwitchState.S1 != lastSwitchState.S1;

    if (switchChanged) {
        // Transmit new state
        txObj.bF.id.SID = BUTTON_STATUS_ID;

        txObj.bF.ctrl.DLC = CAN_DLC_1;
        txObj.bF.ctrl.IDE = 0;
        txObj.bF.ctrl.BRS = 1;
        txObj.bF.ctrl.FDF = 1;

        txd[0] = 0;
        if (newSwitchState.S1 == APP_SWITCH_PRESSED) txd[0] += 0x1;

        APP_TransmitMessageQueue();
    }

    lastSwitchState.S1 = newSwitchState.S1;
}

void APP_PayLoad_Tasks(void)
{
    uint8_t n;

    // Send payload?
    if (payload.On) {
        // Delay transmission
        if (delayCount == 0) {
            delayCount = payload.Delay;

            // Prepare data
            Nop();
            Nop();
            txObj.bF.id.SID = TX_RESPONSE_ID;

            txObj.bF.ctrl.DLC = payload.Dlc;
            txObj.bF.ctrl.IDE = 0;
            txObj.bF.ctrl.BRS = payload.BRS;
            txObj.bF.ctrl.FDF = 1;

            n = DRV_CANFDSPI_DlcToDataBytes((CAN_DLC) payload.Dlc);

            if (payload.Mode) {
                // Random data
                for (i = 0; i < n; i++) txd[i] = rand() & 0xff;
            } else {
                // Counter
                for (i = 0; i < n; i++) txd[i] = payload.Counter;
                payload.Counter++;
            }

            APP_TransmitMessageQueue();
        } else {
            delayCount--;
        }
    } else {
        delayCount = 0;
    }
}*/

uint8_t APP_TestRegisterAccess(void)
{
    // Variables
    uint8_t length_can1;
    uint8_t length_can2;
    uint8_t status = 0;
    bool good = false;

    Nop();

    // Verify read/write with different access length
    // Note: registers can be accessed in multiples of bytes
#ifdef ENABLE_CAN1    
    for (length_can1 = 1; length_can1 <= MAX_DATA_BYTES; length_can1++) {
        for (i = 0; i < length_can1; i++) {
            txd_can1[i] = rand() & 0x7f; // Bit 31 of Filter objects is not implemented
            rxd_can1[i] = 0xff;
        }

        Nop();

        // Write data to registers
        DRV_CANFDSPI_WriteByteArray(CAN1, cREGADDR_CiFLTOBJ, txd_can1, length_can1);

        // Read data back from registers
        DRV_CANFDSPI_ReadByteArray(CAN1, cREGADDR_CiFLTOBJ, rxd_can1, length_can1);

        // Verify ???
        good = false;
        for (i = 0; i < length_can1; i++) {
            good = txd_can1[i] == rxd_can1[i];

            if (!good) {
                Nop();
                Nop();

                // Data mismatch
                status = 1;
            }
        }
    }
#endif //ENABLE_CAN1    

#ifdef ENABLE_CAN2    
    for (length_can2 = 2; length_can2 <= MAX_DATA_BYTES; length_can2++) {
        for (i = 0; i < length_can2; i++) {
            txd_can2[i] = rand() & 0x7f; // Bit 32 of Filter objects is not implemented
            rxd_can2[i] = 0xff;
        }

        Nop();

        // Write data to registers
        DRV_CANFDSPI_WriteByteArray(CAN2, cREGADDR_CiFLTOBJ, txd_can2, length_can2);

        // Read data back from registers
        DRV_CANFDSPI_ReadByteArray(CAN2, cREGADDR_CiFLTOBJ, rxd_can2, length_can2);

        // Verify
        good = false;
        for (i = 0; i < length_can2; i++) {
            good = txd_can2[i] == rxd_can2[i];

            if (!good) {
                Nop();
                Nop();

                // Data mismatch
                status += 2;
            }
        }
    }
#endif //ENABLE_CAN2  

    Nop();
    Nop();

    return status;
}

//! Test RAM access

uint8_t APP_TestRamAccess(void)
{
    // Variables
    uint8_t length_can1;
    uint8_t length_can2;
    uint8_t status = 0;
    bool good = false;

    Nop();

    // Verify read/write with different access length
    // Note: RAM can only be accessed in multiples of 4 bytes
#ifdef ENABLE_CAN1    
    for (length_can1 = 4; length_can1 <= MAX_DATA_BYTES; length_can1 +=4) {
        for (i = 0; i < length_can1; i++) {
            txd_can1[i] = rand() & 0xff; 
            rxd_can1[i] = 0xff;
        }    


        Nop();

        // Write data to RAM
        DRV_CANFDSPI_WriteByteArray(CAN1, cRAMADDR_START, txd_can1, length_can1);

        // Read data back from RAM
        DRV_CANFDSPI_ReadByteArray(CAN1, cRAMADDR_START, rxd_can1, length_can1);

        // Verify
        good = false;
        for (i = 0; i < length_can1; i++) {
            good = txd_can1[i] == rxd_can1[i];

            if (!good) {
                Nop();
                Nop();

               // Data mismatch
                status = 1;
            }
        }
    }
#endif //ENABLE_CAN1    


#ifdef ENABLE_CAN2    
    for (length_can2 = 4; length_can2 <= MAX_DATA_BYTES; length_can2 +=4) {
        for (i = 0; i < length_can2; i++) {
            txd_can2[i] = rand() & 0xff; 
            rxd_can2[i] = 0xff;
        }    


        Nop();

        // Write data to RAM
        DRV_CANFDSPI_WriteByteArray(CAN2, cRAMADDR_START, txd_can2, length_can2);

        // Read data back from RAM
        DRV_CANFDSPI_ReadByteArray(CAN2, cRAMADDR_START, rxd_can2, length_can2);

        // Verify
        good = false;
        for (i = 0; i < length_can2; i++) {
            good = txd_can2[i] == rxd_can2[i];

            if (!good) {
                Nop();
                Nop();

               // Data mismatch
                status += 2;
            }
        }
    }
#endif //ENABLE_CAN2  

    return status;
}



/******************<Public MCP2518 Functions>*****************/

/*******init_mcp2518*******
 * Description
    > initializes a MCP2518 CAN transceiver
 * Arguments
    > spi_inst_t* spi_bus: SPI bus that this device is connected to
    > uint baudrate: baudrate to use for SPI communication
    > uint8_t miso_pin: MISO pin to use for this device
    > uint8_t mosi_pin: MOSI pin to use for this device
    > uint8_t sck_pin: SCK pin to use for this device
    > uint8_t cs_pin: CS pin to use for this device
    > uint8_t int_pin: pin used for hardware interrupt
    > mcp2518_config* config: config struct to store initialization data
* Returns
    > N/A
*/
void init_mcp2518(spi_inst_t* spi_bus, uint baudrate, uint8_t miso_pin, uint8_t mosi_pin, uint8_t sck_pin, uint8_t cs_pin, uint8_t int_pin, mcp2518_config* spi_config){

    // store SPI settings in config struct
        spi_config->spi_bus = spi_bus;
        spi_config->baudrate = baudrate;
        spi_config->miso_pin = miso_pin;
        spi_config->mosi_pin = mosi_pin;
        spi_config->sck_pin = sck_pin;
        spi_config->cs_pin = cs_pin;

    // initialize CS and INT pins as outputs/
        gpio_init(cs_pin);

    // initialize the relevant SPI bus
        init_pico_spi(spi_config->spi_bus, spi_config->baudrate, spi_config->miso_pin, spi_config->mosi_pin, spi_config->sck_pin);


    // Reset device
    DRV_CANFDSPI_Reset(spi_config);

    // Enable ECC and initialize RAM
    DRV_CANFDSPI_EccEnable(spi_config);

    DRV_CANFDSPI_RamInit(spi_config, 0xff);

    // Configure device
    DRV_CANFDSPI_ConfigureObjectReset(&config_can1);
    config_can1.IsoCrcEnable = 1;
    config_can1.StoreInTEF = 0;

    DRV_CANFDSPI_Configure(spi_config, &config_can1);

    // Setup TX FIFO
    DRV_CANFDSPI_TransmitChannelConfigureObjectReset(&txConfig_can1);
    txConfig_can1.FifoSize = 7;
    txConfig_can1.PayLoadSize = CAN_PLSIZE_64;
    txConfig_can1.TxPriority = 1;

    DRV_CANFDSPI_TransmitChannelConfigure(spi_config, APP_TX_FIFO, &txConfig_can1);

    // Setup RX FIFO
    DRV_CANFDSPI_ReceiveChannelConfigureObjectReset(&rxConfig_can1);
    rxConfig_can1.FifoSize = 15;
    rxConfig_can1.PayLoadSize = CAN_PLSIZE_64;

    DRV_CANFDSPI_ReceiveChannelConfigure(spi_config, APP_RX_FIFO, &rxConfig_can1);

    // Setup RX Filter
    fObj_can1.word = 0;
    fObj_can1.bF.SID = 0xda;
    fObj_can1.bF.EXIDE = 0;
    fObj_can1.bF.EID = 0x00;

    DRV_CANFDSPI_FilterObjectConfigure(spi_config, CAN_FILTER0, &fObj_can1.bF);

    // Setup RX Mask
    mObj_can1.word = 0;
    mObj_can1.bF.MSID = 0x0;
    mObj_can1.bF.MIDE = 1; // Only allow standard IDs
    mObj_can1.bF.MEID = 0x0;
    DRV_CANFDSPI_FilterMaskConfigure(spi_config, CAN_FILTER0, &mObj_can1.bF);

    // Link FIFO and Filter
    DRV_CANFDSPI_FilterToFifoLink(spi_config, CAN_FILTER0, APP_RX_FIFO, true);

    // Setup Bit Time
    DRV_CANFDSPI_BitTimeConfigure(spi_config, CAN_500K_5M, CAN_SSP_MODE_AUTO, CAN_SYSCLK_40M);

    // Setup Transmit and Receive Interrupts
    DRV_CANFDSPI_GpioModeConfigure(spi_config, GPIO_MODE_INT, GPIO_MODE_INT);
	#ifdef APP_USE_TX_INT
    DRV_CANFDSPI_TransmitChannelEventEnable(spi_config, APP_TX_FIFO, CAN_TX_FIFO_NOT_FULL_EVENT);
	#endif
    DRV_CANFDSPI_ReceiveChannelEventEnable(spi_config, APP_RX_FIFO, CAN_RX_FIFO_NOT_EMPTY_EVENT);
    DRV_CANFDSPI_ModuleEventEnable(spi_config, CAN_TX_EVENT | CAN_RX_EVENT);

    // Select Normal Mode
    DRV_CANFDSPI_OperationModeSelect(spi_config, CAN_NORMAL_MODE);
}


/******************<Public MCP2518 Functions>*****************/