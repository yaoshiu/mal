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

find_library(READLINE_LIBRARY NAMES readline)

if(READLINE_LIBRARY)
  message(STATUS "Found GNU Readline: ${READLINE_LIBRARY}")
else()
  message(FATAL_ERROR "GNU Readline library not found")
endif()

find_library(HISTORY_LIBRARY NAMES history)

if(HISTORY_LIBRARY)
  message(STATUS "Found GNU History: ${HISTORY_LIBRARY}")
else()
  message(FATAL_ERROR "GNU History library not found")
endif()

add_executable(step0_repl src/step0_repl.c)

target_link_libraries(step0_repl PRIVATE ${READLINE_LIBRARY} ${HISTORY_LIBRARY})

add_executable(step1_read_print src/step1_read_print.c src/reader.c src/types.c
                                src/printer.c src/SipHash/siphash.c)

find_library(PCRE_LIBRARY NAMES pcre)

if(PCRE_LIBRARY)
  message(STATUS "Found PCRE: ${PCRE_LIBRARY}")
else()
  message(FATAL_ERROR "PCRE library not found")
endif()

target_link_libraries(
  step1_read_print PRIVATE ${READLINE_LIBRARY} ${HISTORY_LIBRARY}
                           ${PCRE_LIBRARY})
