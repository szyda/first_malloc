cmake_minimum_required(VERSION 3.17)
project(easy_malloc C)

# Przyjmij standard C11
set(CMAKE_C_STANDARD 11)

# Utwórz target 'project1' i dodaj do niego pliki źródlowe:
add_executable(easy_malloc main.c)
