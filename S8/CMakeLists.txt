cmake_minimum_required(VERSION 3.29)
project(temp_S8 C)

set(CMAKE_C_STANDARD 99)

add_executable(temp_S8 src/S8_person.c
        src/S8_administration.c)
