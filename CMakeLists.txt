cmake_minimum_required(VERSION 3.10)
project(QPLC CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(qplc
    src/main.cpp
    src/lexer/lexer.cpp
)

target_include_directories(qplc PRIVATE src)