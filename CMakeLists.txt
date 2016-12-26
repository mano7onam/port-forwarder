cmake_minimum_required(VERSION 3.6)
project(PortForwarder)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp Connection.cpp Buffer.cpp )
add_executable(PortForwarder ${SOURCE_FILES})