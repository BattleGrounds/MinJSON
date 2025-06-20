cmake_minimum_required(VERSION 3.20)
project(MinJSON LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(MinJSON INTERFACE)
target_include_directories(MinJSON INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
