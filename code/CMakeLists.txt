cmake_minimum_required(VERSION 3.12)

include(pico_sdk_import.cmake)

project(wristRehabMechanism-top)

pico_sdk_init()

add_subdirectory(freertos)
add_subdirectory(freeact)
add_subdirectory(ProjectFiles)
add_subdirectory(pio_stepper)
add_subdirectory(ProjectFiles_motorTest)