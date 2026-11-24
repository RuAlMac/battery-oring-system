/******************<Dependencies>*****************/
#include "communication/rp2350_spi.h"
/******************</Dependencies>*****************/


/******************<I2C Functions>*****************/

/*******init_rp2350_spi*******
    * Description
        > initializes a SPI bus in master mode
    * Arguments
        > spi_inst_t* spi_bus: SPI bus to init (spi0 or spi1)
        >    uint8_t miso_pin: valid GPIO pin for use as MISO
        >    uint8_t mosi_pin: valid GPIO pin for use as MOSI
        >     uint8_t sck_pin: valid GPIO pin for use as SPI
        >       uint baudrate: baudrate for this SPI bus
    * Returns
        > N/A
*/
void init_rp2350_spi_master(
    spi_inst_t* spi_bus,
    uint8_t miso_pin,
    uint8_t mosi_pin,
    uint8_t sck_pin,
    uint baudrate
){
    spi_init(spi_bus, baudrate);
    gpio_set_function(miso_pin, GPIO_FUNC_SPI);
    gpio_set_function(mosi_pin, GPIO_FUNC_SPI);
    gpio_set_function(sck_pin, GPIO_FUNC_SPI);
}


/*******rp2350_spi_write*******
    * Description
        > writes data to a device over SPI
        > assumes that the device's CS pin is already pulled low
    * Arguments
        > spi_inst_t* spi_bus: SPI bus to init (spi0 or spi1)
        >        uint8_t* src: pointer to data array
        >        uint8_t size: number of items to write
    * Returns
        > N/A
*/
void rp2350_spi_write(spi_inst_t* spi_bus, uint8_t* src, uint8_t size){
    spi_write_blocking(spi_bus, src, size);
}
/******************</I2C Functions>*****************/
