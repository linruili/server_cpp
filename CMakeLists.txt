cmake_minimum_required(VERSION 3.7)
project(server)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp)
find_package(OpenCV REQUIRED)

add_executable(server ${SOURCE_FILES})

target_link_libraries(server ${OpenCV_LIBS})