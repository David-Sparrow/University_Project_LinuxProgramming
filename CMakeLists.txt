cmake_minimum_required(VERSION 3.16)
project(Pasozyt C)

set(CMAKE_C_STANDARD 11)

add_executable(Pasozyt parasite.c)
add_executable(OdbiorcaPonaglen OdbiorcaPonaglen.c)