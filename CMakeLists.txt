cmake_minimum_required(VERSION 3.16)

project(epanet-lsx LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(EPANET_DIR ${CMAKE_SOURCE_DIR}/build/EPANET)

file(GLOB_RECURSE EPANET_SOURCES CONFIGURE_DEPENDS ${EPANET_DIR}/src/*.c)
list(FILTER EPANET_SOURCES EXCLUDE REGEX "/outfile/src/epanet_output\\.c$")
file(GLOB LSX_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.c)

# Library (libepanet2)
add_library(epanet2 SHARED ${EPANET_SOURCES} ${LSX_SOURCES})

target_include_directories(epanet2 PRIVATE
    ${EPANET_DIR}/include
    ${EPANET_DIR}/src
    ${EPANET_DIR}/src/util
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(epanet2 PRIVATE m)

set_target_properties(epanet2 PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build
)

# Command line executable (runepanet)
add_executable(runepanet ${EPANET_DIR}/run/main.c)

target_include_directories(runepanet PRIVATE ${EPANET_DIR}/include)

set_target_properties(runepanet PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

if(APPLE)
    set(RUNEPANET_RPATH "@loader_path")
else()
    set(RUNEPANET_RPATH "$ORIGIN")
endif()
set_target_properties(runepanet PROPERTIES
    BUILD_RPATH "${RUNEPANET_RPATH}"
    INSTALL_RPATH "${RUNEPANET_RPATH}")

if(MSVC)
    target_compile_definitions(runepanet PRIVATE _CRT_SECURE_NO_DEPRECATE)
    target_compile_options(runepanet PRIVATE -MT)
endif()

target_link_libraries(runepanet PRIVATE epanet2 m)
