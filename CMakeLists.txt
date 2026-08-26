cmake_minimum_required(VERSION 3.10)
project(QPLC CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(qplc
    src/main.cpp
    src/lexer/lexer.cpp
    src/parser/parser.cpp
    src/config/config_parser.cpp
    src/semantic/semantic_analyzer.cpp
    src/codegen/ladder_generator.cpp
)

target_include_directories(qplc PRIVATE src)

# لینک استاتیک در ویندوز: exe بدون نیاز به DLLهای ucrt64\bin از هر ترمینالی اجرا می‌شود
if(WIN32)
    target_link_options(qplc PRIVATE -static -static-libgcc -static-libstdc++)
endif()