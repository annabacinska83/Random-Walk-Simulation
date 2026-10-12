cmake_minimum_required(VERSION 3.10)
project(Simulacia_IPC)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

find_package(Threads REQUIRED)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

include_directories(include)

add_library(common STATIC
        src/simulacia.c
        src/mapa.c
        src/chodec.c
)


add_executable(server src/server.c)


target_link_libraries(server PRIVATE
        common
        m
        Threads::Threads
        rt
)


add_executable(client
        src/klient.c
        src/main.c
        src/inputHandler.c
)

target_link_libraries(client PRIVATE common Threads::Threads m)