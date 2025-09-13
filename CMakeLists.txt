cmake_minimum_required(VERSION 3.16)
project(RAGger VERSION 0.1.0 LANGUAGES CXX C)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type configuration
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

# Memory analysis tools
option(RAGGER_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(RAGGER_ENABLE_MSAN "Enable MemorySanitizer" OFF)
option(RAGGER_ENABLE_TSAN "Enable ThreadSanitizer" OFF)
option(RAGGER_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

if(RAGGER_ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
    message(STATUS "AddressSanitizer enabled")
endif()

if(RAGGER_ENABLE_MSAN)
    add_compile_options(-fsanitize=memory -fno-omit-frame-pointer)
    add_link_options(-fsanitize=memory)
    message(STATUS "MemorySanitizer enabled")
endif()

if(RAGGER_ENABLE_TSAN)
    add_compile_options(-fsanitize=thread)
    add_link_options(-fsanitize=thread)
    message(STATUS "ThreadSanitizer enabled")
endif()

if(RAGGER_ENABLE_UBSAN)
    add_compile_options(-fsanitize=undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=undefined)
    message(STATUS "UndefinedBehaviorSanitizer enabled")
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# vcpkg integration
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
endif()

# Find required packages
find_package(Threads REQUIRED)

# Core dependencies
find_package(SQLite3 REQUIRED)
find_package(spdlog QUIET)
find_package(nlohmann_json QUIET)

# Optional GUI dependencies
find_package(glfw3 QUIET)
find_package(OpenGL QUIET)
find_package(imgui QUIET)

# Testing dependencies
find_package(GTest QUIET)
find_package(benchmark QUIET)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/api
    ${SQLite3_INCLUDE_DIRS}
)

# Core library (static)
add_library(ragger-core STATIC
    src/core/EventBus.cpp
    src/core/PluginManager.cpp
    src/core/IndexManager.cpp
    src/core/ContextEngine.cpp
    src/core/ConfigManager.cpp
    src/core/ComprehensiveContextGenerator.cpp
    src/core/Logger.cpp
    src/core/FileUtils.cpp
)

target_include_directories(ragger-core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/api
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    PRIVATE
        ${SQLite3_INCLUDE_DIRS}
)

target_link_libraries(ragger-core
    PUBLIC
        Threads::Threads
)

if(SQLite3_FOUND)
    target_link_libraries(ragger-core PUBLIC ${SQLite3_LIBRARIES})
    target_compile_definitions(ragger-core PUBLIC RAGGER_HAS_SQLITE)
endif()

if(spdlog_FOUND)
    target_link_libraries(ragger-core PRIVATE spdlog::spdlog)
    target_compile_definitions(ragger-core PUBLIC RAGGER_HAS_SPDLOG)
endif()

if(nlohmann_json_FOUND)
    target_link_libraries(ragger-core PRIVATE nlohmann_json::nlohmann_json)
    target_compile_definitions(ragger-core PUBLIC RAGGER_HAS_NLOHMANN_JSON)
endif()

# Enable interprocedural optimization for release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported)
    if(ipo_supported)
        set_target_properties(ragger-core PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
endif()

# Main application (console version for now)
add_executable(ragger
    src/main.cpp
)

target_link_libraries(ragger
    PRIVATE
        ragger-core
)

# GUI Library
add_library(ragger-gui-lib STATIC
    src/ui/GUI.cpp
)

target_include_directories(ragger-gui-lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/api
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core
)

target_link_libraries(ragger-gui-lib
    PUBLIC
        ragger-core
)


# Basic GUI Application (stub implementation)
add_executable(ragger-gui
    src/ui/ragger-gui.cpp
)

target_link_libraries(ragger-gui
    PRIVATE
        ragger-gui-lib
        ragger-core
)

target_include_directories(ragger-gui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/api
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
)

# Console Interface (enhanced version with comprehensive context)
add_executable(ragger-console
    src/ui/console_interface.cpp
)

target_link_libraries(ragger-console
    PRIVATE
        ragger-core
)

target_include_directories(ragger-console PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/api
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
)

# Simple Console Interface (basic version)
add_executable(ragger-simple-console
    src/ui/simple_console.cpp
)

target_link_libraries(ragger-simple-console
    PRIVATE
        ragger-core
)

target_include_directories(ragger-simple-console PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/api
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
)

# Working GUI Application (with Dear ImGui)
add_executable(ragger-working-gui
    src/ui/working_gui.cpp
)

target_link_libraries(ragger-working-gui
    PRIVATE
        ragger-core
        glfw
        GL
)

# Check for Dear ImGui for working GUI
find_path(IMGUI_WORKING_INCLUDE_DIR imgui.h
    PATHS /usr/include /usr/local/include
    PATH_SUFFIXES imgui
)

find_path(IMGUI_WORKING_IMPL_GLFW_DIR imgui_impl_glfw.h
    PATHS /usr/include /usr/local/include
    PATH_SUFFIXES imgui/backends
)

if(IMGUI_WORKING_INCLUDE_DIR AND IMGUI_WORKING_IMPL_GLFW_DIR)
    target_link_libraries(ragger-working-gui PRIVATE imgui stb)
    target_include_directories(ragger-working-gui PRIVATE ${IMGUI_WORKING_INCLUDE_DIR} ${IMGUI_WORKING_IMPL_GLFW_DIR})
    target_compile_definitions(ragger-working-gui PRIVATE RAGGER_HAS_IMGUI)
    message(STATUS "Working GUI: Dear ImGui found - GUI mode enabled")
else()
    message(STATUS "Working GUI: Dear ImGui not found - console mode only")
endif()

target_include_directories(ragger-working-gui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/api
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
)

# Simple GUI Application (with fallback to console mode)
add_executable(ragger-simple-gui
    src/ui/simple_gui.cpp
)

target_link_libraries(ragger-simple-gui
    PRIVATE
        ragger-core
)

# Try to find GLFW and OpenGL for simple GUI
find_package(glfw3 QUIET)
find_package(OpenGL QUIET)

# Check for Dear ImGui headers
find_path(IMGUI_INCLUDE_DIR imgui.h
    PATHS /usr/include /usr/local/include
    PATH_SUFFIXES imgui
)

find_path(IMGUI_IMPL_GLFW_DIR imgui_impl_glfw.h
    PATHS /usr/include /usr/local/include
    PATH_SUFFIXES imgui/backends
)

if(glfw3_FOUND AND OpenGL_FOUND AND IMGUI_INCLUDE_DIR AND IMGUI_IMPL_GLFW_DIR)
    target_link_libraries(ragger-simple-gui PRIVATE glfw OpenGL::GL imgui stb)
    target_include_directories(ragger-simple-gui PRIVATE ${IMGUI_INCLUDE_DIR} ${IMGUI_IMPL_GLFW_DIR})
    target_compile_definitions(ragger-simple-gui PRIVATE RAGGER_HAS_IMGUI)
    message(STATUS "Simple GUI: GLFW, OpenGL, and Dear ImGui found - GUI mode enabled")
else()
    message(STATUS "Simple GUI: GLFW/OpenGL/Dear ImGui not found - console mode only")
endif()

target_include_directories(ragger-simple-gui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/api
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
)

# TODO: Add Dear ImGui support later
# Find Dear ImGui
find_package(PkgConfig REQUIRED)
pkg_check_modules(IMGUI REQUIRED imgui)

if(IMGUI_FOUND)
    target_link_libraries(ragger-gui-lib PRIVATE ${IMGUI_LIBRARIES})
    target_include_directories(ragger-gui-lib PRIVATE ${IMGUI_INCLUDE_DIRS})
    target_compile_definitions(ragger-gui-lib PRIVATE RAGGER_HAS_IMGUI)
endif()

# Plugins (basic versions for now)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/plugins/parsers/cpp_parser/CppParserPlugin.cpp")
    add_library(cpp_parser SHARED
        plugins/parsers/cpp_parser/CppParserPlugin.cpp
    )
    target_link_libraries(cpp_parser PRIVATE ragger-core)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/plugins/rankers/bm25_ranker/BM25RankerPlugin.cpp")
    add_library(bm25_ranker SHARED
        plugins/rankers/bm25_ranker/BM25RankerPlugin.cpp
    )
    target_link_libraries(bm25_ranker PRIVATE ragger-core)
endif()

# Additional plugins
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/plugins/parsers/treesitter_parser/TreeSitterParserPlugin.cpp")
    add_library(treesitter_parser SHARED
        plugins/parsers/treesitter_parser/TreeSitterParserPlugin.cpp
    )
    target_link_libraries(treesitter_parser PRIVATE ragger-core)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/plugins/rankers/graph_ranker/GraphRankerPlugin.cpp")
    add_library(graph_ranker SHARED
        plugins/rankers/graph_ranker/GraphRankerPlugin.cpp
    )
    target_link_libraries(graph_ranker PRIVATE ragger-core)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/plugins/clients/lsp_client/LSPClientPlugin.cpp")
    add_library(lsp_client SHARED
        plugins/clients/lsp_client/LSPClientPlugin.cpp
    )
    target_link_libraries(lsp_client PRIVATE ragger-core)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/plugins/integrations/git_integration/GitIntegrationPlugin.cpp")
    add_library(git_integration SHARED
        plugins/integrations/git_integration/GitIntegrationPlugin.cpp
    )
    target_link_libraries(git_integration PRIVATE ragger-core)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/plugins/discovery/test_discovery/TestDiscoveryPlugin.cpp")
    add_library(test_discovery SHARED
        plugins/discovery/test_discovery/TestDiscoveryPlugin.cpp
    )
    target_link_libraries(test_discovery PRIVATE ragger-core)
endif()

# Context Plugins (Development-Lifecycle-Aware)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/plugins/context/git_context/GitContextPlugin.cpp")
    add_library(git_context SHARED
        plugins/context/git_context/GitContextPlugin.cpp
    )
    target_link_libraries(git_context PRIVATE ragger-core)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/plugins/context/static_analysis/StaticAnalysisPlugin.cpp")
    add_library(static_analysis SHARED
        plugins/context/static_analysis/StaticAnalysisPlugin.cpp
    )
    target_link_libraries(static_analysis PRIVATE ragger-core)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/plugins/context/documentation/DocsPlugin.cpp")
    add_library(documentation SHARED
        plugins/context/documentation/DocsPlugin.cpp
    )
    target_link_libraries(documentation PRIVATE ragger-core)
endif()

# CLI Tool
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/tools/ragger-dev.cpp")
    add_executable(ragger-dev
        src/tools/ragger-dev.cpp
    )
    target_link_libraries(ragger-dev
        PRIVATE
            ragger-core
    )
    target_include_directories(ragger-dev PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/api
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    )
endif()

# Installation
install(TARGETS ragger
    RUNTIME DESTINATION bin
)

# Install GUI application
if(TARGET ragger-gui)
    install(TARGETS ragger-gui
        RUNTIME DESTINATION bin
    )
endif()

# Install plugins
install(TARGETS cpp_parser bm25_ranker treesitter_parser graph_ranker 
               lsp_client git_integration test_discovery
    LIBRARY DESTINATION lib/ragger/plugins
)

# Install CLI tool
if(TARGET ragger-dev)
    install(TARGETS ragger-dev
        RUNTIME DESTINATION bin
    )
endif()

# Install configuration files
install(FILES config/ragger.conf
    DESTINATION etc/ragger
)

# Install documentation
install(FILES README.md
    DESTINATION share/doc/ragger
)

# Build Summary
message(STATUS "")
message(STATUS "=== RAGger Build Summary ===")
message(STATUS "Core Library: ragger-core")
message(STATUS "GUI Library: ragger-gui-lib")
message(STATUS "")
message(STATUS "Available Executables:")
message(STATUS "  - ragger-gui (basic GUI - stub implementation)")
message(STATUS "  - ragger-simple-gui (simple GUI with console fallback)")
message(STATUS "  - ragger-working-gui (full GUI with Dear ImGui)")
message(STATUS "  - ragger-console (enhanced console with comprehensive context)")
message(STATUS "  - ragger-simple-console (basic console interface)")
message(STATUS "  - ragger-dev (development tool)")
message(STATUS "  - ragger-tests (test suite)")
message(STATUS "  - ragger-benchmarks (performance benchmarks)")
message(STATUS "")
message(STATUS "Available Plugins:")
message(STATUS "  - cpp_parser (C++ code parsing)")
message(STATUS "  - bm25_ranker (BM25 ranking algorithm)")
message(STATUS "  - treesitter_parser (multi-language parsing)")
message(STATUS "  - graph_ranker (graph-based ranking)")
message(STATUS "  - lsp_client (Language Server Protocol)")
message(STATUS "  - git_integration (Git repository integration)")
message(STATUS "  - test_discovery (automated test detection)")
message(STATUS "  - git_context (Git historical context)")
message(STATUS "  - static_analysis (project standards analysis)")
message(STATUS "  - documentation (documentation integration)")
message(STATUS "")
message(STATUS "Build completed successfully!")
message(STATUS "")

# Testing and Benchmarking
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    add_subdirectory(tests)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/CMakeLists.txt")
    add_subdirectory(benchmarks)
endif()

# Packaging
set(CPACK_PACKAGE_NAME "RAGger")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "High-performance C++ RAG pre-processor for AI-powered code analysis")
set(CPACK_PACKAGE_VENDOR "RAGger Development Team")
set(CPACK_PACKAGE_CONTACT "ragger-dev@example.com")

# Set package type
set(CPACK_GENERATOR "DEB;RPM;TGZ")

# Debian package settings
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "RAGger Development Team")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libsqlite3-0, libssl3")
set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")

# RPM package settings
set(CPACK_RPM_PACKAGE_LICENSE "MIT")
set(CPACK_RPM_PACKAGE_GROUP "Development/Tools")
set(CPACK_RPM_PACKAGE_REQUIRES "sqlite, openssl")

# Include CPack
include(CPack)

# Generate compile_commands.json for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Print configuration summary
message(STATUS "RAGger Configuration:")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Qt6 Support: ${Qt6_FOUND}")
message(STATUS "  SQLite3: ${SQLite3_FOUND}")
message(STATUS "  spdlog: ${spdlog_FOUND}")
message(STATUS "  nlohmann/json: ${nlohmann_json_FOUND}")
message(STATUS "  Install Prefix: ${CMAKE_INSTALL_PREFIX}")
