/******************<Dependencies>*****************/
#include "communication/pico_spi.h"
/******************</Dependencies>*****************/


/******************<I2C Functions>*****************/

/*******init_pico_spi*******
 * Description
    > initializes a SPI bus in master mode for
        communicating with other devices
 * Arguments
    > spi_inst_t* spi_bus: SPI bus to initialize
    > uint baudrate: baudrate to use for this SPI bus
    > uint8_t MISO_PIN: pin used for MISO
    > uint8_t MOSI_PIN: pin used for MOSI
    > uint8_t SCK_PIN: pin used for SCK
* Returns
    > N/A
*/
void init_pico_spi(spi_inst_t* spi_bus, uint baudrate, uint8_t miso_pin, uint8_t mosi_pin, uint8_t sck_pin){

    spi_set_format(spi_bus, 8U, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST); // default SPI format

    spi_init(spi_bus, baudrate);
    gpio_set_function(miso_pin, GPIO_FUNC_SPI);
    gpio_set_function(mosi_pin, GPIO_FUNC_SPI);
    gpio_set_function(sck_pin, GPIO_FUNC_SPI);
}



/*******pico_spi_transfer_data*******
 * Description
    > reads/writes data from a SPI bus
 * Arguments
    > spi_inst_t* spi_bus: SPI bus from which to read/write data
    > uint8_t cs_pin: CS pin to set low
    > uint8_t* tx_data: pointer to data to transmit
    > uint8_t* rx_data: pointer to location to store received data
    > uint16_t transfer_size: number of bytes to read/write
 * Returns
    > int8_t: anything other than 0 indicates an SPI data transmission error
*/
int8_t pico_spi_transfer_data(spi_inst_t* spi_bus, uint8_t cs_pin, uint8_t* tx_data, uint8_t* rx_data, uint16_t transfer_size){

    // activate targeted SPI device
        gpio_put(cs_pin, 0);

    // read/write data
        spi_write_read_blocking(spi_bus, tx_data, rx_data, transfer_size);

    // deactivate targeted SPI device
        gpio_put(cs_pin, 1);

    // return
        return 0;
}
/******************</I2C Functions>*****************/
