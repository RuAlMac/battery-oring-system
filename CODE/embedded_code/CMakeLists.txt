# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.1)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(CODE C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable and version information
add_executable(CODE main.c )
pico_set_program_name(CODE "CODE")
pico_set_program_version(CODE "0.1")


# Add all .c files here
target_sources(CODE PRIVATE
    communication/src/pico_i2c.c
    communication/src/rp2350_spi.c
    communication/src/serial_interface.c

    peripherals/src/battery_monitoring.c
    peripherals/src/battery_settings.c
    peripherals/src/indication_board.c
    peripherals/src/onboard_memory.c
    peripherals/src/onboard_peripherals.c
    
    libraries/bq769x/bq769x.c
)

# Target project directories
target_include_directories(CODE PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/peripherals
        ${CMAKE_CURRENT_LIST_DIR}/communication
)


# UART/USB Operation
pico_enable_stdio_uart(CODE 0)      # debug over UART
pico_enable_stdio_usb(CODE 1)       # debug over USB
target_compile_definitions(CODE PRIVATE PICO_STDIO_USB_STDOUT_TIMEOUT_US=20000)  # do not stall long if host stops reading


# Target required libraries
target_link_libraries(CODE 
        pico_stdlib
        hardware_i2c
        hardware_pwm
        hardware_spi
        hardware_adc
        )

pico_add_extra_outputs(CODE)

