

/**********<SPI Pins>**********/
#define CAN_TRANSCEIVER_SPI_BUS     spi0

#define SPI_BAUDRATE                8500000 // init at 8.5 MHz, SCK <= 0.85 * (FSYSCLK/2) per MCP2518 datasheet
#define MOSI0_PIN                   19
#define MISO0_PIN                   16
#define SCK0_PIN                    18

#define CAN_CS_PIN                  20
#define CAN_INT_PIN                 21
/**********</SPI Pins>**********/