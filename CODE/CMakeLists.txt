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
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(game_controller C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# ─────────────────────────────────────────────────────────────────────────────
# Third-party dependency: pico-littlefs (flash-backed config persistence)
# Must be cloned as a git submodule before building:
#   git submodule add https://github.com/lurk101/pico-littlefs.git lib/pico-littlefs
#   git submodule update --init --recursive
# See README.md for full setup instructions.
# ─────────────────────────────────────────────────────────────────────────────
if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/lib/pico-littlefs/CMakeLists.txt)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/pico-littlefs)
else()
    message(WARNING
        "pico-littlefs submodule not found at lib/pico-littlefs — "
        "config_store.c will fail to compile until you run:\n"
        "  git submodule add https://github.com/lurk101/pico-littlefs.git lib/pico-littlefs\n"
        "  git submodule update --init --recursive")
endif()

# Add executable. Default name is the project name, version 0.1
add_executable(game_controller
    src/main.c
    src/hid.c
    src/inputs.c
    src/triggers.c
    src/leds.c
    src/deadzone.c
    src/config_store.c
    src/usb_descriptors.c
)

pico_set_program_name(game_controller "game_controller")
pico_set_program_version(game_controller "1.0")

# Generate the PIO header from ws2812.pio at build time (pioasm runs
# automatically; the generated ws2812.pio.h is placed in the build tree
# and included by leds.c via "ws2812.pio.h").
pico_generate_pio_header(game_controller ${CMAKE_CURRENT_LIST_DIR}/src/ws2812.pio)

# Disable UART stdio (no serial console wired up) and enable USB stdio only
# for printf-style debug output over the same USB connection, separate from
# the HID report traffic.
pico_enable_stdio_uart(game_controller 0)
pico_enable_stdio_usb(game_controller 0)
# NOTE: USB stdio is left OFF by default because this device's USB
# connection is dedicated to the HID gamepad interface; enabling CDC stdio
# here would add a second USB interface (CDC) alongside HID, which some
# games/launchers refuse to enumerate correctly. Flip to 1 only for debug
# builds, and remember to also set CFG_TUD_CDC to 1 in tusb_config.h.

target_include_directories(game_controller PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/src
)

target_link_libraries(game_controller
    pico_stdlib
    pico_unique_id
    hardware_adc
    hardware_i2c
    hardware_pio
    hardware_clocks
    tinyusb_device
    tinyusb_board
)

if(TARGET pico_littlefs)
    target_link_libraries(game_controller pico_littlefs)
endif()

# Link against the math library for sqrtf() used in deadzone.c
target_link_libraries(game_controller m)

# create map/bin/hex/uf2 file in addition to ELF
pico_add_extra_outputs(game_controller)
