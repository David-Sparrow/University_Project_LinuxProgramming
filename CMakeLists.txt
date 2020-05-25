cmake_minimum_required(VERSION 3.16)
project(Linux_1 C)

set(CMAKE_C_STANDARD 11)

add_executable(parasite parasite.c)
add_executable(OdbiorcaPonaglen OdbiorcaPonaglen.c)
add_executable(provider provider.c)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(provider Threads::Threads)
target_link_libraries(provider m)


