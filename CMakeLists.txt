cmake_minimum_required(VERSION 2.8.0)

## FIXME: the DayOS gcc does not like -rdynamic, but CMake uses that
## when checking functionality.
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)

project(DayOSKernel)

set(CMAKE_MODULE_PATH
  ${CMAKE_SOURCE_DIR}/CMake
  ${CMAKE_MODULE_PATH}
)

set(ARCH x86)

file(GLOB HEADERS *.h)
file(GLOB SOURCES *.c*)

if(${ARCH} STREQUAL "x86")
	file(GLOB_RECURSE ARCH_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/hal.x86/*.h)
	file(GLOB_RECURSE ARCH_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/hal.x86/*.c*)
	file(GLOB_RECURSE ARCH_ASM ${CMAKE_CURRENT_SOURCE_DIR}/hal.x86/*.asm)

	
	set(CMAKE_C_FLAGS "-m32 -Wall -nostartfiles -nostdlib -g -pedantic-errors")
	set(CMAKE_CXX_FLAGS "-m32 -std=c++0x -fno-use-cxa-atexit -nostartfiles -fno-builtin -fno-rtti -fno-exceptions -fno-leading-underscore -Wall -Wextra -pedantic-errors")

	include_directories("include" "hal.x86/include" "sys")

	set(CMAKE_ASM_NASM_OBJECT_FORMAT "elf32")
endif()

enable_language(ASM_NASM)
add_executable(kernel ${SOURCES} ${ARCH_ASM} ${ARCH_SOURCES} ${HEADERS} ${ARCH_HEADERS})

#target_link_libraries(kernel dayos)

#set(CMAKE_CXX_LINK_EXECUTABLE "")
message("LINKSTUFF: " ${CMAKE_C_LINK_EXECUTABLE})
set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_C_COMPILER> <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS>  -o <TARGET> -ldayos")
#STRING(REPLACE "${CMAKE_C_LINK_EXECUTABLE}" "-rdynamic" " " OUTVAR)
#set(CMAKE_C_LINK_EXECUTABLE ${OUTVAR})

#set(CMAKE_EXE_LINKER_FLAGS -T${CMAKE_CURRENT_SOURCE_DIR}/kernel.ld -Wno-error=unknown-error)
set_target_properties(kernel PROPERTIES LINK_FLAGS "-T${CMAKE_CURRENT_SOURCE_DIR}/kernel.ld")
