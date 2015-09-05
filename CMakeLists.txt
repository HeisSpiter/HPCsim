cmake_minimum_required(VERSION 2.8.12)
project(HPCsim)
find_package(Threads REQUIRED)

include_directories(SDK)
add_subdirectory(HPCsim)
add_subdirectory(examples)
