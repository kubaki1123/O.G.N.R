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
set(PICO_BOARD pico2_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(O.G.N.R C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Włączamy pobieranie zawartości z internetu
include(FetchContent)

# Mówimy skąd pobrać bibliotekę RadioLib
FetchContent_Declare(
    RadioLib
    GIT_REPOSITORY https://github.com/jgromes/RadioLib.git
    GIT_TAG        master
)

# Rozkazujemy ją pobrać i przygotować do użycia
FetchContent_MakeAvailable(RadioLib)

# Add executable. Default name is the project name, version 0.1

add_executable(O.G.N.R O.G.N.R.cpp lora_mesh.cpp server.cpp)

pico_set_program_name(O.G.N.R "O.G.N.R")
pico_set_program_version(O.G.N.R "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(O.G.N.R 0)
pico_enable_stdio_usb(O.G.N.R 1)

# Add the standard library to the build
target_link_libraries(O.G.N.R 
    pico_stdlib 
    RadioLib
    hardware_spi 
    hardware_pwm 
    hardware_timer 
    hardware_clocks 
    pico_multicore
    pico_cyw43_arch_lwip_threadsafe_background
)

# Add the standard include files to the build
target_include_directories(O.G.N.R PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)


pico_add_extra_outputs(O.G.N.R)

