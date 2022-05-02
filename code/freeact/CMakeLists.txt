set(PICO_SDK_FREEACT_SOURCE .)

add_library(freeact
    ${PICO_SDK_FREEACT_SOURCE}/FreeAct.c
)

target_include_directories(freeact PUBLIC
    .
    ${PICO_SDK_FREEACT_SOURCE}/include
)

target_link_libraries(freeact
    freertos
)