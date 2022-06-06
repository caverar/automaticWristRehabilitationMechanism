set(PIO_STEPPER_SOURCE .)

# add_library(freeact
#    ${PIO_I2C_SOURCE}/pio_i2c.c
# )

add_library(pio_stepper
    src/pio_stepper.c
)

target_include_directories(pio_stepper PUBLIC
    include
)

target_link_libraries(pio_stepper 
    pico_stdlib 
    hardware_pio
)

pico_generate_pio_header(pio_stepper ${CMAKE_CURRENT_SOURCE_DIR}/src/driver/stepper.pio)



