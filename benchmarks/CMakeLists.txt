# Benchmark configuration
cmake_minimum_required(VERSION 3.16)

# Find Google Benchmark
find_package(benchmark REQUIRED)

# Benchmark executables
add_executable(ragger-benchmarks
    benchmark_simple.cpp
)

# Link with RAGger core and Google Benchmark (this should inherit include directories)
target_link_libraries(ragger-benchmarks
    PRIVATE
        ragger-core
        benchmark::benchmark
        benchmark::benchmark_main
)

# Set output directory
set_target_properties(ragger-benchmarks PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Set benchmark properties
target_compile_definitions(ragger-benchmarks PRIVATE
    BENCHMARK_STATIC_DEFINE
)
