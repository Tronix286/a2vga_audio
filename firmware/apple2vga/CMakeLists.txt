cmake_minimum_required(VERSION 3.16)

# Pull in SDK (must be before project)
include(cmake/pico_sdk_import.cmake)

project(applevga)
set(CMAKE_C_STANDARD 11)

set(APPLE_MODEL "IIPLUS" CACHE STRING "The model of Apple II to support")
message("Target Apple model is ${APPLE_MODEL}")
add_definitions(-DAPPLE_MODEL_${APPLE_MODEL}=1 -DPICO_FLASH_SPI_CLKDIV=4 -DOVERCLOCKED=1)

pico_sdk_init()

add_compile_options(-Wall -Wextra -Wnull-dereference -Wno-unused-parameter -Wno-unused-function)


add_executable(applevga)
pico_set_binary_type(applevga copy_to_ram)
pico_set_linker_script(applevga ${CMAKE_CURRENT_SOURCE_DIR}/memmap_copy_to_ram_custom.ld)

pico_generate_pio_header(applevga
    ${CMAKE_CURRENT_SOURCE_DIR}/vga.pio)
pico_generate_pio_header(applevga
    ${CMAKE_CURRENT_SOURCE_DIR}/abus.pio)

file(GLOB TEXTFONT_SRC_FILES textfont/*.c)
target_sources(applevga PUBLIC
    main.c
    abus.c
    buffers.c
    colors.c
    config.c
    device_regs.c
    render.c
    render_hires.c
    render_lores.c
    render_testpat.c
    render_text.c
    ${TEXTFONT_SRC_FILES}
    vga.c
    videx_vterm.c
)
target_include_directories(applevga PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(applevga PUBLIC
    pico_multicore
    pico_stdlib
    hardware_dma
    hardware_flash
    hardware_pio
)

pico_add_extra_outputs(applevga)

pico_define_boot_stage2(slower_boot2_vga ${PICO_DEFAULT_BOOT_STAGE2_FILE})
target_compile_definitions(slower_boot2_vga PRIVATE PICO_FLASH_SPI_CLKDIV=4)

pico_set_boot_stage2(applevga slower_boot2_vga)
