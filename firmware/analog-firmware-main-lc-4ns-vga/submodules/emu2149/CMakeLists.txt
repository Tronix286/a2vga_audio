cmake_minimum_required(VERSION 3.22)

project(emu2149n)

if (NOT BUILD_SHARED_LIBS)
 add_definitions(-DVR_EMU_2149_STATIC)
endif()

add_library(emu2149n)

if (WIN32)
  if (BUILD_SHARED_LIBS)
     add_definitions(-DVR_EMU_2149_COMPILING_DLL)
   endif()
endif()

target_sources(emu2149n PRIVATE emu2149.c PUBLIC emu2149.h)
target_include_directories (emu2149n PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
