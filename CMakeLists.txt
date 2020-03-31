cmake_minimum_required(VERSION 3.15)
project(Project C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(ARM-tank-wars
        address_map_arm.h
        main.c)
