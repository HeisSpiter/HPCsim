cmake_minimum_required(VERSION 2.8.12)
include(CheckIncludeFiles)

project(HPCsim)

check_include_files(stdint.h HAVE_STDINT_H)
if (HAVE_STDINT_H)
    add_definitions(-DHAVE_STDINT_H)
endif()
find_package(Threads REQUIRED)

if(NOT DEFINED USE_PILOT_THREAD)
    set(USE_PILOT_THREAD FALSE)
endif()

if (USE_PILOT_THREAD)
    message("-- Using pilot thread jobs")
    add_definitions(-DUSE_PILOT_THREAD)
endif()

include_directories(SDK)
add_subdirectory(HPCsim)
add_subdirectory(examples)
