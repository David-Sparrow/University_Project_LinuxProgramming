cmake_minimum_required(VERSION 3.16)
project(Linux_1 C)

set(CMAKE_C_STANDARD 11)

add_executable(Pasozyt parasite.c)
add_executable(OdbiorcaPonaglen OdbiorcaPonaglen.c)
add_executable(Provider provider.c)