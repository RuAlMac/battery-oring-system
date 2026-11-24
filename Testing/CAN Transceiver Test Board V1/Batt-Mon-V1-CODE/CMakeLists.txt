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

project(Batt-Mon-V1-CODE C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(Batt-Mon-V1-CODE main.c )


# Add all .c files here
target_sources(Batt-Mon-V1-CODE PRIVATE
    peripherals/src/bq769x.c
    communication/src/pico_i2c.c
)

pico_set_program_name(Batt-Mon-V1-CODE "Batt-Mon-V1-CODE")
pico_set_program_version(Batt-Mon-V1-CODE "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(Batt-Mon-V1-CODE 0)
pico_enable_stdio_usb(Batt-Mon-V1-CODE 1)

# Add the standard library to the build
target_link_libraries(Batt-Mon-V1-CODE
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(Batt-Mon-V1-CODE PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(Batt-Mon-V1-CODE 
        hardware_i2c
        )

pico_add_extra_outputs(Batt-Mon-V1-CODE)

