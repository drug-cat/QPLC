cmake_minimum_required(VERSION 3.16)
project(QPLC
    VERSION 1.0.0
    DESCRIPTION "QPLC — IEC 61131-3 ladder logic compiler and tools"
    LANGUAGES CXX
)

# ── C++ standard ─────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Build settings ───────────────────────────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ── Compiler warnings ────────────────────────────────────────────────────────
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ── Sources ──────────────────────────────────────────────────────────────────
add_executable(qplc
    src/main.cpp
    src/lexer/lexer.cpp
    src/parser/parser.cpp
    src/config/config_parser.cpp
    src/semantic/semantic_analyzer.cpp
    src/codegen/ladder_generator.cpp
    src/codegen/scl_generator.cpp
    src/lsp/lsp_server.cpp
)

target_include_directories(qplc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Static link on Windows so the .exe runs from any terminal without needing
# ucrt64/bin DLLs to be on PATH.
if(WIN32)
    target_link_options(qplc PRIVATE
        -static
        -static-libgcc
        -static-libstdc++
    )
endif()

# ── Install rules ────────────────────────────────────────────────────────────
include(GNUInstallDirs)

install(TARGETS qplc
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Optional: install man page if present
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/output/qplc.1")
    install(FILES "output/qplc.1"
        DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
    )
endif()

# ── Package config (find_package(QPLC)) ──────────────────────────────────────
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/qplcConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/qplcConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QPLC
)
