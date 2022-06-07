# Set name of project (as PROJECT_NAME) and C/C++ Standards
project(wrist_mechanism C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Creates a pico-sdk subdirectory in our project for the libraries
pico_sdk_init()

include_directories(include)

# point out the CMake, where to find the executable source file
add_executable(${PROJECT_NAME}
    src/main.c
    src/bsp.c
    src/blinky_AO.c
    src/printer_AO.c
    src/dev_hd44780.c
    src/UI_AO.c
    src/Motors_AO.c
    src/AS5600.c
)

# Create map/bin/hex/uf2 files.
pico_add_extra_outputs(${PROJECT_NAME})

# Pull in our pico_stdlib which pulls in commonly used features (gpio, timer-delay etc)
target_link_libraries(
    ${PROJECT_NAME}
    pico_stdlib
    hardware_i2c
    hardware_timer
    hardware_adc
    hardware_pio
    freertos
    freeact
    pio_stepper
)


# SDK configurations
pico_enable_stdio_usb(${PROJECT_NAME} 0)
pico_enable_stdio_uart(${PROJECT_NAME} 1)