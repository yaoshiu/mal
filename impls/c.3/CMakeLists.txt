cmake_minimum_required(VERSION 3.27)
project(mal.c.3)

option(STATIC_ANALYSIS "Enable static analysis" OFF)
if(STATIC_ANALYSIS)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fanalyzer")
  message(STATUS "Static analysis enabled")
else()
  message(STATUS "Static analysis disabled")
endif()

option(DEBUG "Enable debug" OFF)
if(DEBUG)
  set(CMAKE_BUILD_TYPE "Debug")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -Wall -Wextra -Wpedantic")
  message(STATUS "Debug enabled")
else()
  set(CMAKE_BUILD_TYPE "Release")
  message(STATUS "Debug disabled")
endif()

find_package(PkgConfig REQUIRED)

pkg_check_modules(readline REQUIRED IMPORTED_TARGET readline)
link_libraries(PkgConfig::readline)
include_directories(${readline_INCLUDE_DIRS})

add_executable(step0_repl src/step0_repl.c)

pkg_check_modules(pcre REQUIRED IMPORTED_TARGET libpcre)
link_libraries(PkgConfig::pcre)
include_directories(${pcre_INCLUDE_DIRS})

add_library(mal src/reader.c src/printer.c src/types.c src/env.c
                src/SipHash/siphash.c)
link_libraries(mal)

add_executable(step1_read_print src/step1_read_print.c)

add_executable(step2_eval src/step2_eval.c)

add_executable(step3_env src/step3_env.c)
