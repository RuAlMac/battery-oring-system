cmake_minimum_required(VERSION 3.16)
project(MIL_ORing_Monitoring_Config_Tool VERSION 0.1 LANGUAGES CXX)

# Default install location: a "dist/BatteryTool" folder inside the build folder, so "install" never needs admin
# rights (CMake's default on Windows is C:/Program Files). Override with --prefix or -DCMAKE_INSTALL_PREFIX=...
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT OR CMAKE_INSTALL_PREFIX MATCHES "[Pp]rogram [Ff]iles")
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/dist/BatteryTool" CACHE PATH "Install prefix" FORCE)
    message(STATUS "Install prefix set to ${CMAKE_INSTALL_PREFIX}")
endif()

# Qt GUI for the Battery ORing & Monitoring V2 board stack.
# Builds the same way on Linux and Windows (and macOS):
#   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
#   cmake --build build --config Release

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets SerialPort Network Svg)

qt_add_executable(BatteryTool
    resources/resources.qrc
    src/main.cpp
    src/branding.cpp
    src/branding.h
    src/protocol.cpp
    src/protocol.h
    src/serial_link.cpp
    src/serial_link.h
    src/settings_panel.cpp
    src/settings_panel.h
    src/gauges.cpp
    src/status_chips.cpp
    src/plot_widgets.cpp
    src/plot_widgets.h
    src/plot_panel.cpp
    src/plot_panel.h
    src/status_chips.h
    src/gauges.h
    src/monitor_panel.cpp
    src/monitor_panel.h
    src/simulate_panel.cpp
    src/simulate_panel.h
    src/terminal_panel.cpp
    src/terminal_panel.h
    src/serial_settings_panel.cpp
    src/serial_settings_panel.h
    src/main_window.cpp
    src/main_window.h
)

target_link_libraries(BatteryTool PRIVATE Qt6::Widgets Qt6::SerialPort Qt6::Network Qt6::Svg)

if(WIN32)
    set_target_properties(BatteryTool PROPERTIES WIN32_EXECUTABLE ON)

    # icon of the .exe file in Explorer (the running window icon is set in code)
    target_sources(BatteryTool PRIVATE resources/app_icon.rc)

    # Optional: copy the Qt DLLs next to the exe so it runs on its own
    get_target_property(_qmake_exe Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_exe}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET BatteryTool POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-translations "$<TARGET_FILE:BatteryTool>")
    endif()
endif()

# ------------------------------------------------------------------------------------------
# Install / packaging rules (used by "cmake --install" and by CPack)
#
#   cmake -S . -B build-rel -DCMAKE_BUILD_TYPE=Release
#   cmake --build build-rel
#   cmake --install build-rel --prefix dist/BatteryTool     (Windows: this folder IS the portable app)
#   cd build-rel && cpack                                   (portable .zip on Windows, .deb + .tar.gz on Linux)
#
# Windows : BatteryTool.exe + Qt DLLs + plugins all land in the install folder (via windeployqt)
# Linux   : bin/BatteryTool, a .desktop launcher and icon (Qt itself comes from the system / .deb dependencies)
# ------------------------------------------------------------------------------------------
include(GNUInstallDirs)

if(WIN32)
    install(TARGETS BatteryTool RUNTIME DESTINATION .)
else()
    install(TARGETS BatteryTool RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(FILES packaging/linux/batterytool.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications OPTIONAL)
    install(FILES resources/mil_logo_light.svg
            DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
            RENAME batterytool.svg OPTIONAL)
endif()

# Copies the Qt runtime (DLLs, platform / style / image plugins) next to the installed exe.
# Windows only (needs Qt 6.3+). On Linux, Qt comes from the system / .deb dependencies, or linuxdeploy bundles it for an AppImage.
if(WIN32 AND COMMAND qt_generate_deploy_app_script)
    if(Qt6_VERSION VERSION_GREATER_EQUAL 6.5)
        # Qt 6.5+ : OUTPUT_SCRIPT (FILENAME_VARIABLE was removed in newer releases such as 6.12)
        qt_generate_deploy_app_script(
            TARGET BatteryTool
            OUTPUT_SCRIPT BATTERYTOOL_DEPLOY_SCRIPT
            NO_UNSUPPORTED_PLATFORM_ERROR
            NO_TRANSLATIONS)
    else()
        # Qt 6.3 / 6.4
        qt_generate_deploy_app_script(
            TARGET BatteryTool
            FILENAME_VARIABLE BATTERYTOOL_DEPLOY_SCRIPT
            NO_UNSUPPORTED_PLATFORM_ERROR)
    endif()
    install(SCRIPT ${BATTERYTOOL_DEPLOY_SCRIPT})
endif()

# Documentation / licence files shipped with the app (only the ones that exist)
foreach(_doc README.md LICENSE LICENSE.txt)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_doc}")
        if(WIN32)
            install(FILES ${_doc} DESTINATION .)
        else()
            install(FILES ${_doc} DESTINATION ${CMAKE_INSTALL_DOCDIR})
        endif()
    endif()
endforeach()

# CPack: "cpack" in the build folder makes the distributable archive/package
set(CPACK_PACKAGE_NAME "BatteryTool")
set(CPACK_PACKAGE_VENDOR "MIL - SubjuGator 9")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Battery ORing & Monitoring V2 configuration and monitoring tool")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_FILE_NAME "BatteryTool-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}")
if(WIN32)
    set(CPACK_GENERATOR ZIP)
    set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY ON)
else()
    set(CPACK_GENERATOR DEB TGZ)
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "MIL SubjuGator 9 team")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    set(CPACK_DEBIAN_PACKAGE_SECTION "electronics")
endif()
include(CPack)
