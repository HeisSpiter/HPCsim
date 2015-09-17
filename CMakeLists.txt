cmake_minimum_required(VERSION 2.8.12)
include(CheckIncludeFiles)

project(HPCsim)

check_include_files(stdint.h HAVE_STDINT_H)
if (HAVE_STDINT_H)
    add_definitions(-DHAVE_STDINT_H)
endif()
find_package(Threads REQUIRED)

include_directories(SDK)
add_subdirectory(HPCsim)
add_subdirectory(examples)
