cmake_minimum_required(VERSION 3.14)
project(Asteroids)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic -Wshadow -Wunused -O3")

file(GLOB SOURCES ${CMAKE_SOURCE_DIR}/*.hpp *.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})