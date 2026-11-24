#include <stdio.h>
#include "pico/stdlib.h"


#include "MCP2518/mcp2518_frontend.h"

#include "pins.h"

int main()
{
    stdio_init_all();

    // create instance of mcp2518 config struct
        mcp2518_config MCP2518_config;

    // initialize MCP2518
        init_mcp2518(CAN_TRANSCEIVER_SPI_BUS, SPI_BAUDRATE, MISO0_PIN, MOSI0_PIN, SCK0_PIN, CAN_CS_PIN, CAN_INT_PIN, &MCP2518_config);

    while (true) {
        printf("Hello, world!\n");
        sleep_ms(1000);
    }
}
