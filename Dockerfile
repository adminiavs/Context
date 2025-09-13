# RAGger - Multi-stage Dockerfile for Development and Production
# This Dockerfile provides both development and production environments

# =============================================================================
# Stage 1: Base Development Environment
# =============================================================================
FROM ubuntu:22.04 AS base

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC
ENV CMAKE_BUILD_TYPE=Release
ENV CC=gcc
ENV CXX=g++

# Install system dependencies
RUN apt-get update && apt-get install -y \
    # Build essentials
    build-essential \
    cmake \
    make \
    git \
    pkg-config \
    # C++ development tools
    gcc \
    g++ \
    clang \
    clang-format \
    clang-tidy \
    # System libraries
    libsqlite3-dev \
    libglfw3-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    # Development tools
    valgrind \
    gdb \
    strace \
    # Documentation tools
    doxygen \
    graphviz \
    # Package managers
    curl \
    wget \
    unzip \
    # Cleanup
    && rm -rf /var/lib/apt/lists/*

# =============================================================================
# Stage 2: Development Environment with vcpkg
# =============================================================================
FROM base AS development

# Install vcpkg
WORKDIR /opt
RUN git clone https://github.com/Microsoft/vcpkg.git && \
    cd vcpkg && \
    ./bootstrap-vcpkg.sh && \
    ./vcpkg integrate install

# Set vcpkg environment
ENV VCPKG_ROOT=/opt/vcpkg
ENV CMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake

# Install RAGger dependencies via vcpkg
RUN /opt/vcpkg/vcpkg install \
    sqlite3 \
    spdlog \
    nlohmann-json \
    glfw3 \
    opengl \
    imgui[glfw-binding,opengl3-binding] \
    stb \
    gtest \
    benchmark

# Create development user
RUN useradd -m -s /bin/bash ragger && \
    usermod -aG sudo ragger

# Set working directory
WORKDIR /workspace

# Copy source code
COPY --chown=ragger:ragger . /workspace/

# Switch to development user
USER ragger

# =============================================================================
# Stage 3: Build Environment
# =============================================================================
FROM development AS build

# Create build directory
RUN mkdir -p build

# Configure and build RAGger
WORKDIR /workspace/build
RUN cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DRAGGER_ENABLE_ASAN=OFF \
    -DRAGGER_ENABLE_MSAN=OFF \
    -DRAGGER_ENABLE_TSAN=OFF \
    -DRAGGER_ENABLE_UBSAN=OFF

# Build the project
RUN make -j$(nproc)

# Run tests
RUN ./bin/ragger-tests

# =============================================================================
# Stage 4: Production Environment
# =============================================================================
FROM ubuntu:22.04 AS production

# Install runtime dependencies only
RUN apt-get update && apt-get install -y \
    # Runtime libraries
    libsqlite3-0 \
    libglfw3 \
    libgl1-mesa-glx \
    libglu1-mesa \
    # System tools
    ca-certificates \
    # Cleanup
    && rm -rf /var/lib/apt/lists/*

# Create ragger user
RUN useradd -m -s /bin/bash ragger

# Set working directory
WORKDIR /app

# Copy built binaries and libraries
COPY --from=build /workspace/build/bin/ /app/bin/
COPY --from=build /workspace/build/lib/ /app/lib/
COPY --from=build /workspace/build/data/ /app/data/

# Copy configuration files
COPY --from=build /workspace/config/ /app/config/
COPY --from=build /workspace/README.md /app/
COPY --from=build /workspace/ragger.png /app/

# Set permissions
RUN chown -R ragger:ragger /app && \
    chmod +x /app/bin/*

# Switch to ragger user
USER ragger

# Set environment variables
ENV PATH="/app/bin:${PATH}"
ENV LD_LIBRARY_PATH="/app/lib:${LD_LIBRARY_PATH}"

# Expose ports (if needed for future web interface)
EXPOSE 8080

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD /app/bin/ragger --version || exit 1

# Default command
CMD ["/app/bin/ragger-console"]

# =============================================================================
# Stage 5: Testing Environment
# =============================================================================
FROM build AS testing

# Install additional testing tools
USER root
RUN apt-get update && apt-get install -y \
    # Code coverage
    lcov \
    gcovr \
    # Static analysis
    cppcheck \
    # Performance profiling
    perf-tools-unstable \
    # Cleanup
    && rm -rf /var/lib/apt/lists/*

# Switch back to development user
USER ragger

# Set up test environment
ENV RAGGER_TEST_DATA_DIR=/workspace/test_data
RUN mkdir -p $RAGGER_TEST_DATA_DIR

# Copy test data
COPY --chown=ragger:ragger tests/ /workspace/tests/
COPY --chown=ragger:ragger benchmarks/ /workspace/benchmarks/

# Run comprehensive tests
RUN cd /workspace/build && \
    # Unit tests
    ./bin/ragger-tests && \
    # Benchmarks
    ./bin/ragger-benchmarks --benchmark_format=json > benchmark_results.json && \
    # Memory tests with valgrind
    valgrind --leak-check=full --show-leak-kinds=all ./bin/ragger-tests

# =============================================================================
# Stage 6: Documentation Environment
# =============================================================================
FROM development AS documentation

# Install documentation tools
USER root
RUN apt-get update && apt-get install -y \
    # Documentation generation
    doxygen \
    graphviz \
    # Markdown tools
    pandoc \
    # Cleanup
    && rm -rf /var/lib/apt/lists/*

# Switch back to development user
USER ragger

# Generate documentation
WORKDIR /workspace
RUN mkdir -p docs/generated && \
    # Generate API documentation
    doxygen Doxyfile 2>/dev/null || echo "Doxyfile not found, skipping Doxygen" && \
    # Generate markdown documentation
    pandoc README.md -o docs/generated/README.pdf 2>/dev/null || echo "Pandoc PDF generation failed"

# =============================================================================
# Stage 7: CI/CD Environment
# =============================================================================
FROM development AS ci

# Install CI/CD tools
USER root
RUN apt-get update && apt-get install -y \
    # Code quality tools
    cppcheck \
    clang-tidy \
    # Coverage tools
    lcov \
    gcovr \
    # Security scanning
    bandit \
    # Cleanup
    && rm -rf /var/lib/apt/lists/*

# Switch back to development user
USER ragger

# Set up CI environment
ENV CI=true
ENV RAGGER_CI_BUILD=1

# Create CI scripts
RUN mkdir -p /workspace/scripts/ci

# Copy CI scripts
COPY --chown=ragger:ragger scripts/ci/ /workspace/scripts/ci/

# Make CI scripts executable
RUN chmod +x /workspace/scripts/ci/*.sh

# Default CI command
CMD ["/workspace/scripts/ci/run_all_tests.sh"]

# =============================================================================
# Labels and Metadata
# =============================================================================
LABEL maintainer="RAGger Development Team"
LABEL version="1.0.0"
LABEL description="RAGger - RAG Pre-processor for AI Development"
LABEL org.opencontainers.image.source="https://github.com/adminiavs/Context"
LABEL org.opencontainers.image.licenses="MIT"
LABEL org.opencontainers.image.created="2024-01-01T00:00:00Z"
LABEL org.opencontainers.image.revision="1bc0345"

# Add build arguments for customization
ARG BUILD_DATE
ARG VCS_REF
ARG VERSION

LABEL org.label-schema.build-date=$BUILD_DATE
LABEL org.label-schema.vcs-ref=$VCS_REF
LABEL org.label-schema.version=$VERSION
LABEL org.label-schema.schema-version="1.0"
