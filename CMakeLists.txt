cmake_minimum_required(VERSION 3.16)

project(epanet-lsx LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(EPANET_DIR ${CMAKE_SOURCE_DIR}/build/EPANET)

# The extensions loader uses dlopen/dlsym (${CMAKE_DL_LIBS}; empty on Win32/macOS)
# and pthread_once (Threads) to load sidecar extensions.
find_package(Threads REQUIRED)

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
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/vendor)

target_link_libraries(epanet2 PRIVATE m ${CMAKE_DL_LIBS} Threads::Threads)

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

target_link_libraries(runepanet PRIVATE epanet2 m ${CMAKE_DL_LIBS})

# Each test binary links the full EPANET engine + LSX sources directly
enable_testing()

set(CEST_DIR ${CMAKE_SOURCE_DIR}/vendor)   # vendored cest header (vendor/cest)

# ASan/LSan on the test targets so create/destroy pair tests catch leaks.
set(SAN_COMPILE -fsanitize=address)
set(SAN_LINK -fsanitize=address)
if(NOT CMAKE_SYSTEM_NAME MATCHES "Darwin")
    list(APPEND SAN_LINK -static-libasan)
endif()

# The engine + LSX C sources compiled once into a sanitized static library that
# every test links against (same set as the shared library; the CLI main lives
# in run/main.c, which is not part of these globs). Compiling them here instead
# of per-test avoids rebuilding the whole engine for each test binary.
set(TEST_LIB_SOURCES ${EPANET_SOURCES} ${LSX_SOURCES})

add_library(epanet_lsx_test STATIC EXCLUDE_FROM_ALL ${TEST_LIB_SOURCES})
target_include_directories(epanet_lsx_test PUBLIC
    ${CEST_DIR}                      # the 'cest' header
    ${CMAKE_SOURCE_DIR}/include      # LSX public headers
    ${CMAKE_SOURCE_DIR}/src          # LSX headers
    ${CMAKE_SOURCE_DIR}/vendor       # Lua amalgam + vendored headers
    ${EPANET_DIR}/include            # EPANET toolkit headers
    ${EPANET_DIR}/src
    ${EPANET_DIR}/src/util)
# The engine sources compile as C99; sanitizers match the test binaries.
target_compile_options(epanet_lsx_test PRIVATE
    -g -O0
    $<$<COMPILE_LANGUAGE:C>:-std=gnu99>
    ${SAN_COMPILE})
set_target_properties(epanet_lsx_test PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(epanet_lsx_test PUBLIC m ${CMAKE_DL_LIBS} Threads::Threads)

file(GLOB_RECURSE TEST_FILES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/tests/*.test.cpp)

add_custom_target(build_tests)

foreach(TEST_FILE ${TEST_FILES})
    get_filename_component(FILE_NAME ${TEST_FILE} NAME_WE)
    set(EXEC_NAME test_${FILE_NAME})
    add_executable(${EXEC_NAME} EXCLUDE_FROM_ALL ${TEST_FILE})
    target_include_directories(${EXEC_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/tests/helpers)
    # .test.cpp compiles as C++20; sanitizers match the shared static library.
    target_compile_options(${EXEC_NAME} PRIVATE
        -g -O0
        $<$<COMPILE_LANGUAGE:CXX>:-std=c++20>
        ${SAN_COMPILE})
    target_link_options(${EXEC_NAME} PRIVATE ${SAN_LINK})
    target_link_libraries(${EXEC_NAME} PRIVATE epanet_lsx_test m)
    target_compile_definitions(${EXEC_NAME} PRIVATE
        FIXTURES_DIR="${CMAKE_SOURCE_DIR}/tests/fixtures")
    set_target_properties(${EXEC_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)
    add_test(NAME ${EXEC_NAME} COMMAND ${EXEC_NAME})
    add_dependencies(build_tests ${EXEC_NAME})
endforeach()
