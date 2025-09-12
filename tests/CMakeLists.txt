# Test configuration
cmake_minimum_required(VERSION 3.16)

# Find Google Test
find_package(GTest REQUIRED)

# Test executables
add_executable(ragger-tests
    unit/test_simple.cpp
)

# Link with RAGger core and Google Test (this should inherit include directories)
target_link_libraries(ragger-tests
    PRIVATE
        ragger-core
        GTest::gtest
        GTest::gtest_main
)

# Set output directory
set_target_properties(ragger-tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Enable testing
enable_testing()

# Add test to CTest
add_test(NAME RaggerUnitTests COMMAND ragger-tests)

# Set test properties
set_tests_properties(RaggerUnitTests PROPERTIES
    TIMEOUT 30
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
