cmake_minimum_required(VERSION 3.28)
project(prescal)

include_directories(include)

set (
    SOURCES
    main.c
    src/engine.c
    src/configuration.c
    ds/linkedlist.c
)

add_executable(prescal ${SOURCES})