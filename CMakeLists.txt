cmake_minimum_required(VERSION 3.14)
project(Asteroids)

# Set C++ options
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic -Wunused")

# Gather sources
file(GLOB SOURCES ${CMAKE_SOURCE_DIR}/*.hpp *.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})

# Find and link libs

find_package(glfw3 3.3 REQUIRED)
target_link_libraries(${PROJECT_NAME} glfw)

find_package(OpenGL REQUIRED)
target_link_libraries(${PROJECT_NAME} OpenGL::GL)

find_package(GLEW REQUIRED)
target_link_libraries(${PROJECT_NAME} GLEW::GLEW)