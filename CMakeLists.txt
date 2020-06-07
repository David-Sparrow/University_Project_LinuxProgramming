cmake_minimum_required(VERSION 3.16)
project(Linux_1 C)

set(CMAKE_C_STANDARD 11)

add_executable(parasite parasite.c)
add_executable(provider provider.c)
add_executable(familiada familiada.c)

target_link_libraries(provider m)


