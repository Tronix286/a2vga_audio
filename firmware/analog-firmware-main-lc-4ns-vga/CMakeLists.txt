cmake_minimum_required(VERSION 3.16)

if(${CMAKE_CURRENT_BINARY_DIR} MATCHES "-gs")
    set(PICO_BOARD pico)
    set(BINARY_TAGS "${BINARY_TAGS}-gs")
    message(STATUS "Building for V2 AnalogGS")
elseif(${CMAKE_CURRENT_BINARY_DIR} MATCHES "-lc")
    set(PICO_BOARD pico)
    set(BINARY_TAGS "${BINARY_TAGS}-lc")
    message(STATUS "Building for V2 Analog LC")
elseif(${CMAKE_CURRENT_BINARY_DIR} MATCHES "-wifi")
    set(PICO_BOARD pico_w)
    set(BINARY_TAGS "${BINARY_TAGS}-wifi")
    message(STATUS "Building for V2 Analog with WiFi")
else()
    message(FATAL_ERROR "You must specify -lc or -wifi board type.")
endif()

if(${CMAKE_CURRENT_BINARY_DIR} MATCHES "-4ns")
    set(BINARY_TAGS "${BINARY_TAGS}-4ns")
    message(STATUS "SYSCLOCK will be 378MHz")
elseif(${CMAKE_CURRENT_BINARY_DIR} MATCHES "-8ns")
    set(BINARY_TAGS "${BINARY_TAGS}-8ns")
    message(STATUS "SYSCLOCK will be 126MHz")
else()
    message(FATAL_ERROR "You must specify -4ns (252MHz) or -8ns (126MHz) speed.")
endif()

if(${CMAKE_CURRENT_BINARY_DIR} MATCHES "-vga")
    set(BINARY_TAGS "${BINARY_TAGS}-vga")
    message(STATUS "VGA Function Enabled")
elseif(${CMAKE_CURRENT_BINARY_DIR} MATCHES "-z80")
    set(BINARY_TAGS "${BINARY_TAGS}-z80")
    message(STATUS "Z80 Function Enabled")
else()
    message(FATAL_ERROR "You must specify -vga or -z80 function.")
endif()

# Pull in SDK (must be before project)
include(cmake/pico_sdk_import.cmake)

project(v2-analog)
set(CMAKE_C_STANDARD 11)

pico_sdk_init()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DPICO_USE_MALLOC_MUTEX=1")

add_executable(v2-analog${BINARY_TAGS})

if(${CMAKE_CURRENT_BINARY_DIR} MATCHES "-gs")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DANALOG_GS=1")
endif()

if(${CMAKE_CURRENT_BINARY_DIR} MATCHES "-wifi")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DANALOG_WIFI=1")
endif()

if(${CMAKE_CURRENT_BINARY_DIR} MATCHES "-4ns")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DCONFIG_SYSCLOCK=378 -DPICO_FLASH_SPI_CLKDIV=4 -DOVERCLOCKED=1")
if(${CMAKE_CURRENT_BINARY_DIR} MATCHES "-gs")
    pico_generate_pio_header(v2-analog${BINARY_TAGS}
        ${CMAKE_CURRENT_SOURCE_DIR}/common/abus-gs-4ns.pio)
else()
    pico_generate_pio_header(v2-analog${BINARY_TAGS}
        ${CMAKE_CURRENT_SOURCE_DIR}/common/abus-4ns.pio)
endif(${CMAKE_CURRENT_BINARY_DIR} MATCHES "-gs")
elseif(${CMAKE_CURRENT_BINARY_DIR} MATCHES "-8ns")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DCONFIG_SYSCLOCK=126")
if(${CMAKE_CURRENT_BINARY_DIR} MATCHES "-gs")
    pico_generate_pio_header(v2-analog${BINARY_TAGS}
        ${CMAKE_CURRENT_SOURCE_DIR}/common/abus-gs-8ns.pio)
else()
    pico_generate_pio_header(v2-analog${BINARY_TAGS}
        ${CMAKE_CURRENT_SOURCE_DIR}/common/abus-8ns.pio)
endif(${CMAKE_CURRENT_BINARY_DIR} MATCHES "-gs")
endif()

if(${CMAKE_CURRENT_BINARY_DIR} MATCHES "-vga")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DFUNCTION_VGA=1")
if(${CMAKE_CURRENT_BINARY_DIR} MATCHES "-gs")
    pico_generate_pio_header(v2-analog${BINARY_TAGS}
        ${CMAKE_CURRENT_SOURCE_DIR}/vga/vga12.pio)
else()
    pico_generate_pio_header(v2-analog${BINARY_TAGS}
        ${CMAKE_CURRENT_SOURCE_DIR}/vga/vga9.pio)
endif(${CMAKE_CURRENT_BINARY_DIR} MATCHES "-gs")
elseif(${CMAKE_CURRENT_BINARY_DIR} MATCHES "-z80")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DFUNCTION_Z80=1")
endif(${CMAKE_CURRENT_BINARY_DIR} MATCHES "-vga")

add_subdirectory(submodules/emu2149)
add_subdirectory(audio)

target_sources(v2-analog${BINARY_TAGS} PUBLIC
    common/main.c
    common/abus.c
    common/config.c
    common/dmacopy.c
    common/buffers.c
    common/flash.c
    audio/audinterface.c
)

if(${CMAKE_CURRENT_BINARY_DIR} MATCHES "-vga")
target_sources(v2-analog${BINARY_TAGS} PUBLIC
    vga/vgamain.c
    vga/businterface.c
    vga/vgabuf.c
    vga/render.c
    vga/render_hires.c
    vga/render_lores.c
    vga/render_text.c
    vga/render_80col.c
    vga/render_dhgr.c
    vga/render_dgr.c
    vga/render_shr.c
    vga/render_test.c
    vga/vgaout.c
)
elseif(${CMAKE_CURRENT_BINARY_DIR} MATCHES "-z80")
target_sources(v2-analog${BINARY_TAGS} PUBLIC
    z80/businterface.c
    z80/z80main.c
    z80/usb_descriptors.c
)
endif()

target_include_directories(v2-analog${BINARY_TAGS} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(v2-analog${BINARY_TAGS} PUBLIC
    pico_multicore
    pico_stdlib
    pico_unique_id
    hardware_resets
    hardware_irq
    hardware_dma
    hardware_pio
    hardware_flash
    audion
)

if(${CMAKE_CURRENT_BINARY_DIR} MATCHES "-z80")
target_link_libraries(v2-analog${BINARY_TAGS} PUBLIC
    tinyusb_device
    tinyusb_board
    hardware_uart
)
endif()

if(${PICO_BOARD} MATCHES "pico_w")
target_link_libraries(v2-analog${BINARY_TAGS} PUBLIC
    pico_cyw43_arch_lwip_poll
)
endif()

pico_enable_stdio_usb(v2-analog${BINARY_TAGS} 0)
pico_enable_stdio_uart(v2-analog${BINARY_TAGS} 0)

pico_set_linker_script(v2-analog${BINARY_TAGS} ${PROJECT_SOURCE_DIR}/delayed_copy.ld)

pico_add_extra_outputs(v2-analog${BINARY_TAGS})

pico_define_boot_stage2(slower_boot2_analog ${PICO_DEFAULT_BOOT_STAGE2_FILE})
target_compile_definitions(slower_boot2_analog PRIVATE PICO_FLASH_SPI_CLKDIV=4)

pico_set_boot_stage2(v2-analog${BINARY_TAGS} slower_boot2_analog)