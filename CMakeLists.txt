cmake_minimum_required(VERSION 3.8)
project(grafika)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -pedantic-errors")

set(SOURCE_FILES main.cpp)
add_executable(grafika ${SOURCE_FILES})

target_link_libraries(grafika "-lGL -lGLEW -lGLU -lglut")
