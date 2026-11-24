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
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(Control-V1-CODE C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(Control-V1-CODE main.c )

pico_set_program_name(Control-V1-CODE "Control-V1-CODE")
pico_set_program_version(Control-V1-CODE "0.1")

# Add all .c files here
target_sources(Control-V1-CODE PRIVATE
    communication/src/pico_i2c.c

    libraries/bq769x/bq769x.c

    peripherals/src/battery_monitoring.c
	peripherals/src/safety_devices.c
)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(Control-V1-CODE 0)
pico_enable_stdio_usb(Control-V1-CODE 1)

# Add the standard library to the build
target_link_libraries(Control-V1-CODE
        pico_stdlib
        hardware_gpio
        hardware_spi
        hardware_i2c
        hardware_pwm
        hardware_adc
        )

# Add the standard include files to the build
target_include_directories(Control-V1-CODE PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(Control-V1-CODE 
        hardware_i2c
        )

pico_add_extra_outputs(Control-V1-CODE)

