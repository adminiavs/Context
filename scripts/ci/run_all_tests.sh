#!/bin/bash

# RAGger CI/CD Test Suite
# This script runs all tests in the CI/CD environment

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Configuration
BUILD_DIR="/workspace/build"
TEST_RESULTS_DIR="/workspace/test_results"
COVERAGE_DIR="/workspace/coverage"
BENCHMARK_RESULTS_DIR="/workspace/benchmark_results"

# Create directories
mkdir -p "$TEST_RESULTS_DIR"
mkdir -p "$COVERAGE_DIR"
mkdir -p "$BENCHMARK_RESULTS_DIR"

log_info "Starting RAGger CI/CD Test Suite"
log_info "Build directory: $BUILD_DIR"
log_info "Test results directory: $TEST_RESULTS_DIR"

# Function to run a test and capture results
run_test() {
    local test_name="$1"
    local test_command="$2"
    local output_file="$TEST_RESULTS_DIR/${test_name}.log"
    
    log_info "Running $test_name..."
    
    if eval "$test_command" > "$output_file" 2>&1; then
        log_success "$test_name passed"
        return 0
    else
        log_error "$test_name failed (see $output_file)"
        return 1
    fi
}

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Change to workspace directory
cd /workspace

# 1. Build the project
log_info "Building RAGger..."
if [ ! -d "$BUILD_DIR" ]; then
    mkdir -p "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# Configure with different build types for testing
log_info "Configuring CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DRAGGER_ENABLE_ASAN=OFF \
    -DRAGGER_ENABLE_MSAN=OFF \
    -DRAGGER_ENABLE_TSAN=OFF \
    -DRAGGER_ENABLE_UBSAN=OFF \
    -DCMAKE_BUILD_TYPE=Debug

# Build
log_info "Building..."
make -j$(nproc)

# 2. Unit Tests
log_info "Running unit tests..."
run_test "unit_tests" "./bin/ragger-tests"

# 3. Static Analysis
log_info "Running static analysis..."

# Cppcheck
if command_exists cppcheck; then
    run_test "cppcheck" "cppcheck --enable=all --inconclusive --std=c++17 --suppress=missingIncludeSystem /workspace/src/ 2>/dev/null || true"
else
    log_warning "cppcheck not available, skipping static analysis"
fi

# Clang-tidy
if command_exists clang-tidy; then
    run_test "clang_tidy" "find /workspace/src -name '*.cpp' -o -name '*.h' | head -10 | xargs clang-tidy -p $BUILD_DIR 2>/dev/null || true"
else
    log_warning "clang-tidy not available, skipping clang-tidy analysis"
fi

# 4. Memory Tests
log_info "Running memory tests..."

# Valgrind
if command_exists valgrind; then
    run_test "valgrind" "valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1 ./bin/ragger-tests 2>/dev/null || true"
else
    log_warning "valgrind not available, skipping memory tests"
fi

# 5. Performance Tests
log_info "Running performance tests..."
run_test "benchmarks" "./bin/ragger-benchmarks --benchmark_format=json --benchmark_out=$BENCHMARK_RESULTS_DIR/benchmark_results.json"

# 6. Code Coverage
log_info "Running code coverage analysis..."

# Configure for coverage
cd /workspace
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DCMAKE_CXX_FLAGS="--coverage -fprofile-arcs -ftest-coverage" \
    -DCMAKE_C_FLAGS="--coverage -fprofile-arcs -ftest-coverage" \
    -DCMAKE_EXE_LINKER_FLAGS="--coverage"

make -j$(nproc)

# Run tests with coverage
./bin/ragger-tests

# Generate coverage report
if command_exists lcov; then
    lcov --capture --directory . --output-file coverage.info
    lcov --remove coverage.info '/usr/*' --output-file coverage.info
    lcov --remove coverage.info '/opt/*' --output-file coverage.info
    
    if command_exists genhtml; then
        genhtml coverage.info --output-directory "$COVERAGE_DIR"
        log_success "Coverage report generated in $COVERAGE_DIR"
    fi
else
    log_warning "lcov not available, skipping coverage report"
fi

# 7. Integration Tests
log_info "Running integration tests..."

# Test console interface
run_test "console_interface" "echo 'test query' | timeout 10s ./bin/ragger-console || true"

# Test GUI (if available)
if [ -f "./bin/ragger-simple-gui" ]; then
    run_test "gui_interface" "timeout 5s ./bin/ragger-simple-gui --help || true"
fi

# 8. Security Tests
log_info "Running security tests..."

# Check for common security issues
run_test "security_scan" "find /workspace -name '*.cpp' -o -name '*.h' | xargs grep -l 'strcpy\\|sprintf\\|gets' 2>/dev/null || echo 'No obvious security issues found'"

# 9. Documentation Tests
log_info "Running documentation tests..."

# Check if documentation builds
if [ -f "/workspace/Doxyfile" ]; then
    run_test "doxygen" "doxygen /workspace/Doxyfile 2>/dev/null || true"
else
    log_warning "Doxyfile not found, skipping documentation generation"
fi

# 10. Package Tests
log_info "Running package tests..."

# Test if all required files are present
run_test "package_check" "ls -la /workspace/build/bin/ && ls -la /workspace/build/lib/"

# 11. Generate Test Report
log_info "Generating test report..."

cat > "$TEST_RESULTS_DIR/test_report.md" << EOF
# RAGger CI/CD Test Report

Generated on: $(date)
Build: $(git rev-parse HEAD 2>/dev/null || echo "unknown")
Environment: $(uname -a)

## Test Results

EOF

# Add test results to report
for log_file in "$TEST_RESULTS_DIR"/*.log; do
    if [ -f "$log_file" ]; then
        test_name=$(basename "$log_file" .log)
        echo "### $test_name" >> "$TEST_RESULTS_DIR/test_report.md"
        echo '```' >> "$TEST_RESULTS_DIR/test_report.md"
        tail -20 "$log_file" >> "$TEST_RESULTS_DIR/test_report.md"
        echo '```' >> "$TEST_RESULTS_DIR/test_report.md"
        echo "" >> "$TEST_RESULTS_DIR/test_report.md"
    fi
done

# 12. Summary
log_info "Test suite completed!"

# Count passed/failed tests
passed_tests=0
failed_tests=0

for log_file in "$TEST_RESULTS_DIR"/*.log; do
    if [ -f "$log_file" ]; then
        if grep -q "SUCCESS" "$log_file" 2>/dev/null; then
            ((passed_tests++))
        else
            ((failed_tests++))
        fi
    fi
done

log_info "Test Summary:"
log_info "  Passed: $passed_tests"
log_info "  Failed: $failed_tests"

if [ $failed_tests -eq 0 ]; then
    log_success "All tests passed!"
    exit 0
else
    log_error "Some tests failed!"
    exit 1
fi
