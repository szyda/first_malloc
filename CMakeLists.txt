cmake_minimum_required(VERSION 3.17)

project(first_malloc C)

set(CMAKE_C_STANDARD 11)

add_executable(first_malloc
        "heap.c"
        "main.c"
)
