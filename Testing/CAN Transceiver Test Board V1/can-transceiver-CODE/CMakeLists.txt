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
set(sdkVersion 2.2.0)
set(toolchainVersion RISCV_ZCB_RPI_2_2_0_3)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(can-transceiver-CODE C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(can-transceiver-CODE main.c )

# Add all .c files here
target_sources(can-transceiver-CODE PRIVATE
    communication/src/pico_spi.c

    MCP2518/src/mcp2518_api.c
    MCP2518/src/mcp2518_config.c
    MCP2518/src/mcp2518_frontend.c
)

pico_set_program_name(can-transceiver-CODE "can-transceiver-CODE")
pico_set_program_version(can-transceiver-CODE "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(can-transceiver-CODE 0)
pico_enable_stdio_usb(can-transceiver-CODE 1)

# Add the standard library to the build
target_link_libraries(can-transceiver-CODE
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(can-transceiver-CODE PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(can-transceiver-CODE 
        hardware_spi
        )

pico_add_extra_outputs(can-transceiver-CODE)

