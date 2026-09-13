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

# ── Optional: QPLC ↔ PLCSIM Advanced bridge (Windows only) ───────────────────
# Builds a small C DLL exposing a flat C API that QPLC.PlcSimAdapter (C#)
# P/Invokes. Requires the Siemens SimulationRuntimeApi.h header; on machines
# without PLCSIM Advanced the target is simply skipped.
if(WIN32)
    # "ProgramFiles(x86)" cannot be read via $ENV{...} in CMake syntax,
    # so we try the common default locations directly.
    set(_plcsim_api_root "C:/Program Files (x86)/Common Files/Siemens/PLCSIMADV/API")

    set(QPLC_PLCSIM_API_HEADER "" CACHE FILEPATH "Path to SimulationRuntimeApi.h (Siemens PLCSIM Advanced)")
    if(NOT QPLC_PLCSIM_API_HEADER)
        file(GLOB _api_dirs "${_plcsim_api_root}/*")
        if(_api_dirs)
            list(SORT _api_dirs)
            list(REVERSE _api_dirs)   # highest version first
            list(GET _api_dirs 0 _api_latest)
            set(QPLC_PLCSIM_API_HEADER "${_api_latest}/SimulationRuntimeApi.h")
        endif()
    endif()

    if(EXISTS "${QPLC_PLCSIM_API_HEADER}")
        add_library(qplc_plcsim_bridge SHARED
            QPLC.PlcSimAdapter/native/qplc_plcsim_bridge.cpp
        )
        target_compile_definitions(qplc_plcsim_bridge PRIVATE
            QPLC_PLCSIM_API_HEADER="${QPLC_PLCSIM_API_HEADER}"
        )
        target_include_directories(qplc_plcsim_bridge PUBLIC
            QPLC.PlcSimAdapter/native
        )
        target_link_libraries(qplc_plcsim_bridge PRIVATE Shlwapi)
        if(MSVC)
            target_compile_options(qplc_plcsim_bridge PRIVATE /W4)
        else()
            target_compile_options(qplc_plcsim_bridge PRIVATE -Wall -Wextra)
        endif()
        message(STATUS "qplc_plcsim_bridge: PLCSIM Advanced API found — bridge enabled")
    else()
        message(STATUS "qplc_plcsim_bridge: SimulationRuntimeApi.h not found — bridge skipped")
    endif()
else()
    message(STATUS "qplc_plcsim_bridge: PLCSIM Advanced not detected — bridge skipped")
endif()

# Static link on Windows so the .exe runs from any terminal without needing
# ucrt64/bin DLLs to be on PATH.
if(WIN32)
    target_link_options(qplc PRIVATE
        -static
        -static-libgcc
        -static-libstdc++
    )
endif()

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
