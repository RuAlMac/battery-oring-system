/******************<Dependencies>*****************/
#include "peripherals/onboard_memory.h"
/******************</Dependencies>*****************/



/******************<Private Variables>*****************/
// record EEPROM write-enable state
static bool write_enabled = false;
/******************</Private Variables>*****************/


/******************<Private Functions>*****************/
/*******eeprom_select*******
 * Description
        > drives CS HIGH to select EEPROM
 * Arguments
        > bool state: true=select EEPROM, false=deselect EEPROM
 * Returns
        > N/A
*/
static void eeprom_select(bool state){
    gpio_put(ONBOARD_MEM_CS_PIN, state);
    busy_wait_us(1);
}


/*******eeprom_send_command*******
 * Description
        > sends a 16-bit instruction (WEN, WDS, ERAL)
        > leading zeros are ignored by EEPROM until the start bit
 * Arguments
        > uint16_t command: instruction word
 * Returns
        > N/A
*/
static void eeprom_send_command(uint16_t command){
    uint8_t bytes[2] = {(uint8_t)(command >> 8), (uint8_t)(command & 0xFF)};
    eeprom_select(true);
    spi_write_blocking(ONBOARD_MEM_SPI_BUS, bytes, 2);
    eeprom_select(false);
}


/*******eeprom_wait_ready*******
 * Description
        > waits for EEPROM to finish an internal write/erase
        > after falling edge of CS ends a write instruction, raising CS
            again makes MISO show busy (low) / ready (high)
 * Arguments
        > uint32_t timeout_ms: max time to wait
 * Returns
        > bool: true=EEPROM became ready, false=EEPROM timed out
*/
static bool eeprom_wait_ready(uint32_t timeout_ms){
    absolute_time_t timeout = make_timeout_time_ms(timeout_ms);
    bool ready = false;

    eeprom_select(true);
    while (!ready){
        if (gpio_get(ONBOARD_MEM_MISO_PIN)){ready = true;}
        else if (time_reached(timeout)){break;}
    }
    eeprom_select(false);

    return ready;
}
/******************</Private Functions>*****************/



/******************<Public Functions>*****************/

/*******init_onboard_memory*******
 * Description
        > inits spi bus and CS pin for onboard EEPROM
        > SPI mode 0 (CPOL=0, CPHA=0), 8-bit frames, MSB first
        > EEPROM is left write-protected after init
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void init_onboard_memory(){
    // init relevant GPIO
        gpio_init(ONBOARD_MEM_CS_PIN);
        gpio_put(ONBOARD_MEM_CS_PIN, 0);
        gpio_set_dir(ONBOARD_MEM_CS_PIN, GPIO_OUT);

    // init SPI bus
        init_rp2350_spi_master(
            ONBOARD_MEM_SPI_BUS,
            ONBOARD_MEM_MISO_PIN,
            ONBOARD_MEM_MOSI_PIN,
            ONBOARD_MEM_SCK_PIN,
            ONBOARD_MEM_SPI_BAUDRATE
        );

        spi_set_format(ONBOARD_MEM_SPI_BUS, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    // ensure writes are disabled
        onboard_memory_set_write_enable(false);
}



/*******onboard_memory_set_write_enable*******
 * Description
        > enables/disables writing to the EEPROM (WEN/WDS)
        > EEPROM powers up write-disabled and remains in the
            chosen state until changed or powered off
 * Arguments
        > bool enable: true=allow write/erase, false=block write/erase
 * Returns
        > N/A
*/
void onboard_memory_set_write_enable(bool enable){
    if (enable){eeprom_send_command(EEPROM_CMD_WRITE_ENABLE);}
    else {eeprom_send_command(EEPROM_CMD_WRITE_DISABLE);}

    write_enabled = enable;
}


/*******onboard_memory_write*******
 * Description
        > writes bytes to EEPROM starting at an address
        > write must be enabled first
        > each byte is a separate write instruction
 * Arguments
        > uint16_t addr: first addr to write (0 to 2047)
        > uint8_t* src: pointer to data to write
        > uint16_t size: number of bytes to write
 * Returns
        > bool: true=successful write, false=unsuccessful
*/
bool onboard_memory_write(uint16_t addr, uint8_t* src, uint16_t size){
    // validate inputs
        if (src == NULL || size == 0){return false;}

        if ((uint32_t)(addr) + size > EEPROM_SIZE_BYTES){return false;}

        if (!write_enabled){return false;}

    // write each byte
        for (uint16_t i = 0; i < size; i++){

            // build 24-bit data frame: 2 pad zeros, start bit, opcode 01, 11 addr bits, 8 data bits
                uint32_t frame = 0x280000u
                               | ((uint32_t)((addr + i) & EEPROM_ADDR_MASK) << 8)
                               | src[i];

                uint8_t bytes[3] = {
                    (uint8_t)(frame >> 16),
                    (uint8_t)(frame >> 8),
                    (uint8_t)(frame & 0xFF)
                };


            // send instruction, CS falling edge starts internal write
                eeprom_select(true);
                spi_write_blocking(ONBOARD_MEM_SPI_BUS, bytes, 3);
                eeprom_select(false);

            // wait for write to finish
                if (!eeprom_wait_ready(EEPROM_WRITE_TIMEOUT_MS)){return false;}
        }

    // if all bytes written successfully, return true
        return true;
}



/*******onboard_memory_read*******
 * Description
        > reads bytes from the EEPROM starting at an address
        > uses the EEPROM's sequential read (address auto-increments while clocking)
 * Arguments
        > uint16_t addr: first addr to read (0 to 2047)
        > uint8_t* dest: location to store data
        > uint16_t size: number of bytes to read
 * Returns
        > bool: true=successful read, false=unsuccessful
*/
bool onboard_memory_read(uint16_t addr, uint8_t* dest, uint16_t size){
    // validate inputs
        if (dest == NULL || size == 0){return false;}

        if ((uint32_t)(addr) + size > EEPROM_SIZE_BYTES){return false;}

    // build 16-bit instruction
    // frame = 1 pad zero, start bit, opcode 10, 11 addr bits, 1 trailing zero.
    // The EEPROM outputs a dummy 0 bit as A0 is clocked in; the trailing bit
    // lets that dummy bit be shifted out during the write so the read that
    // follows starts exactly on D7.
        uint16_t command = 0x6000 | ((addr & EEPROM_ADDR_MASK) << 1);
        uint8_t tx[2] = {(uint8_t)(command >> 8), (uint8_t)(command & 0xFF)};

        eeprom_select(true);
        spi_write_blocking(ONBOARD_MEM_SPI_BUS, tx, 2);
        spi_read_blocking(ONBOARD_MEM_SPI_BUS, 0x00, dest, size);
        eeprom_select(false);

    return true;
}



/*******onboard_memory_clear*******
 * Description
        > erases the entire EEPROM (ERAL)
        > sets every byte to 0xFF
 * Arguments
        > N/A
 * Returns
        > bool: true=successful erasure, false=unsuccessful
*/
bool onboard_memory_clear(){
    // check if write_enabled is true
        if (!write_enabled){return false;}

    // send instructions
        eeprom_send_command(EEPROM_CMD_ERASE_ALL);

    // wait for erase to finish
        return eeprom_wait_ready(EEPROM_ERASE_ALL_TIMEOUT_MS);
}

/******************</Public Functions>*****************/
