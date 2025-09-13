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
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find required packages
find_package(Threads REQUIRED)

# SQLite3 - try to find system version, fallback to none
find_package(SQLite3)

# spdlog - try to find system version, fallback to none
find_package(spdlog)

# nlohmann/json - try to find system version, fallback to none
find_package(nlohmann_json)

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

# GUI Application
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ragger-gui.cpp")
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
endif()

# Simple Console Interface (working version)
add_executable(ragger-console
    src/ui/simple_console.cpp
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

if(IMGUI_FOUND)
    target_link_libraries(ragger-working-gui PRIVATE ${IMGUI_LIBRARIES})
    target_include_directories(ragger-working-gui PRIVATE ${IMGUI_INCLUDE_DIRS})
    target_compile_definitions(ragger-working-gui PRIVATE RAGGER_HAS_IMGUI)
endif()

target_include_directories(ragger-working-gui PRIVATE
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
