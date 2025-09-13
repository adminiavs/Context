# RAGger Docker Guide

This guide explains how to use Docker with RAGger for development, testing, and production deployment.

## 🐳 Overview

RAGger provides a comprehensive Docker setup with multiple stages for different use cases:

- **Development**: Full development environment with all tools
- **Production**: Optimized runtime environment
- **Testing**: Automated testing and CI/CD
- **Documentation**: Documentation generation
- **CI/CD**: Continuous integration and deployment

## 📋 Prerequisites

- Docker 20.10+ 
- Docker Compose 2.0+
- Git
- At least 4GB RAM available for Docker
- At least 10GB free disk space

## 🚀 Quick Start

### Development Environment

```bash
# Start development environment
make -f Makefile.docker docker-dev

# Access the development container
make -f Makefile.docker docker-shell

# Build the project inside the container
cd /workspace/build
cmake ..
make -j$(nproc)
```

### Production Environment

```bash
# Start production environment
make -f Makefile.docker docker-prod

# Check if it's running
curl http://localhost:8080/health
```

### Run Tests

```bash
# Run all tests
make -f Makefile.docker docker-test

# Run specific test types
make -f Makefile.docker docker-test-unit
make -f Makefile.docker docker-test-benchmark
make -f Makefile.docker docker-test-memory
```

## 🏗️ Docker Images

### Base Image (`ragger:base`)
- Ubuntu 22.04 LTS
- Build essentials (gcc, g++, cmake, make)
- System libraries (SQLite, GLFW, OpenGL)
- Development tools (valgrind, gdb, clang-format)

### Development Image (`ragger:development`)
- Includes base image
- vcpkg package manager
- All RAGger dependencies
- Development user setup
- Source code mounted

### Build Image (`ragger:build`)
- Includes development image
- Configured and built RAGger
- All tests passed
- Ready for deployment

### Production Image (`ragger:production`)
- Minimal runtime environment
- Only required libraries
- Optimized for size and security
- Non-root user
- Health checks

### Testing Image (`ragger:testing`)
- Includes build image
- Testing tools (lcov, cppcheck, clang-tidy)
- Test data and scripts
- Coverage reporting

### Documentation Image (`ragger:documentation`)
- Includes development image
- Documentation tools (doxygen, pandoc)
- Generated documentation

### CI/CD Image (`ragger:ci`)
- Includes development image
- CI/CD tools and scripts
- Automated testing pipeline

## 📁 Directory Structure

```
/workspace/                 # Mounted source code
├── src/                   # Source code
├── build/                 # Build directory
├── docs/                  # Documentation
├── scripts/               # Scripts
│   └── ci/               # CI/CD scripts
├── test_results/          # Test results
├── coverage/              # Coverage reports
└── benchmark_results/     # Benchmark results
```

## 🔧 Configuration

### Environment Variables

| Variable | Description | Default |
|----------|-------------|---------|
| `CMAKE_BUILD_TYPE` | Build type | `Release` |
| `VCPKG_ROOT` | vcpkg installation path | `/opt/vcpkg` |
| `RAGGER_DATA_DIR` | Data directory | `/app/data` |
| `RAGGER_CONFIG_DIR` | Config directory | `/app/config` |
| `CI` | CI environment flag | `false` |

### Docker Compose Services

| Service | Description | Ports |
|---------|-------------|-------|
| `ragger-dev` | Development environment | - |
| `ragger-prod` | Production environment | 8080 |
| `ragger-test` | Testing environment | - |
| `ragger-docs` | Documentation generation | - |
| `ragger-ci` | CI/CD pipeline | - |
| `ragger-db` | Database service | - |

## 🛠️ Development Workflow

### 1. Start Development Environment

```bash
# Start development container
make -f Makefile.docker docker-dev

# Access container shell
make -f Makefile.docker docker-shell
```

### 2. Build and Test

```bash
# Inside the container
cd /workspace/build
cmake ..
make -j$(nproc)

# Run tests
./bin/ragger-tests
./bin/ragger-benchmarks
```

### 3. Code Changes

- Edit code in your host system
- Changes are automatically reflected in the container
- Rebuild as needed

### 4. Run Tests

```bash
# From host system
make -f Makefile.docker docker-test
```

## 🧪 Testing

### Unit Tests

```bash
# Run unit tests
make -f Makefile.docker docker-test-unit

# Run with coverage
docker-compose run --rm ragger-test ./bin/ragger-tests
```

### Benchmarks

```bash
# Run benchmarks
make -f Makefile.docker docker-test-benchmark

# Results saved to benchmark_results/
```

### Memory Tests

```bash
# Run memory tests with Valgrind
make -f Makefile.docker docker-test-memory
```

### Static Analysis

```bash
# Run static analysis
docker-compose run --rm ragger-test cppcheck --enable=all /workspace/src/
docker-compose run --rm ragger-test clang-tidy /workspace/src/*.cpp
```

## 📊 CI/CD Pipeline

### Automated Testing

```bash
# Run full CI/CD pipeline
make -f Makefile.docker docker-ci

# Check results
ls -la ci_results/
```

### Pipeline Steps

1. **Build**: Configure and build RAGger
2. **Unit Tests**: Run all unit tests
3. **Static Analysis**: Cppcheck and clang-tidy
4. **Memory Tests**: Valgrind leak detection
5. **Performance Tests**: Benchmarks
6. **Coverage**: Code coverage analysis
7. **Integration Tests**: End-to-end testing
8. **Security Tests**: Security scanning
9. **Documentation**: Generate docs
10. **Package Tests**: Verify packaging

## 🚀 Production Deployment

### Build Production Image

```bash
# Build production image
make -f Makefile.docker docker-prod-build

# Or build all images
make -f Makefile.docker docker-build-all
```

### Deploy Production

```bash
# Start production environment
make -f Makefile.docker docker-prod

# Check health
make -f Makefile.docker docker-health

# View logs
make -f Makefile.docker docker-logs
```

### Production Configuration

```yaml
# docker-compose.override.yml
version: '3.8'
services:
  ragger-prod:
    environment:
      - RAGGER_DATA_DIR=/app/data
      - RAGGER_CONFIG_DIR=/app/config
    volumes:
      - ./data:/app/data
      - ./config:/app/config
    restart: unless-stopped
```

## 📚 Documentation Generation

### Generate Documentation

```bash
# Generate all documentation
make -f Makefile.docker docker-docs

# Check generated docs
ls -la docs/generated/
```

### Documentation Types

- **API Documentation**: Doxygen-generated API docs
- **User Guide**: Markdown documentation
- **Developer Guide**: Plugin development guide
- **Examples**: Code examples and tutorials

## 🔍 Monitoring and Debugging

### Container Status

```bash
# Check container status
make -f Makefile.docker docker-status

# View resource usage
make -f Makefile.docker docker-stats
```

### Logs

```bash
# View all logs
make -f Makefile.docker docker-logs

# View specific service logs
docker-compose logs -f ragger-prod
```

### Debugging

```bash
# Access production container
docker-compose exec ragger-prod /bin/bash

# Debug with gdb
docker-compose exec ragger-dev gdb ./bin/ragger-console
```

## 🧹 Maintenance

### Cleanup

```bash
# Clean up containers and volumes
make -f Makefile.docker docker-clean

# Full cleanup (including images)
make -f Makefile.docker docker-clean-all
```

### Backup and Restore

```bash
# Create backup
make -f Makefile.docker docker-backup

# Restore from backup
make -f Makefile.docker docker-restore BACKUP_FILE=backups/ragger-backup-20240101-120000.tar.gz
```

### Updates

```bash
# Pull latest changes
git pull

# Rebuild images
make -f Makefile.docker docker-build

# Restart services
docker-compose restart
```

## 🔒 Security

### Security Best Practices

1. **Non-root User**: All containers run as non-root user
2. **Minimal Images**: Production images contain only necessary components
3. **Regular Updates**: Base images are regularly updated
4. **Security Scanning**: Automated security tests in CI/CD
5. **Secrets Management**: Use Docker secrets for sensitive data

### Security Scanning

```bash
# Run security scan
docker-compose run --rm ragger-ci /workspace/scripts/ci/security_scan.sh
```

## 🐛 Troubleshooting

### Common Issues

#### Container Won't Start

```bash
# Check logs
docker-compose logs ragger-dev

# Check resource usage
docker stats

# Restart services
docker-compose restart
```

#### Build Failures

```bash
# Clean build cache
docker-compose build --no-cache

# Check dependencies
docker-compose exec ragger-dev vcpkg list
```

#### Permission Issues

```bash
# Fix file permissions
sudo chown -R $USER:$USER .

# Rebuild with proper permissions
make -f Makefile.docker docker-clean
make -f Makefile.docker docker-build
```

#### Memory Issues

```bash
# Increase Docker memory limit
# In Docker Desktop: Settings > Resources > Memory

# Check memory usage
docker stats
```

### Getting Help

1. Check the logs: `make -f Makefile.docker docker-logs`
2. Verify container status: `make -f Makefile.docker docker-status`
3. Check resource usage: `make -f Makefile.docker docker-stats`
4. Review documentation: `docs/`
5. Check GitHub issues: [RAGger Issues](https://github.com/adminiavs/Context/issues)

## 📈 Performance

### Optimization Tips

1. **Use Build Cache**: Leverage Docker layer caching
2. **Parallel Builds**: Use `make -j$(nproc)` for parallel compilation
3. **Volume Mounts**: Use volumes for persistent data
4. **Resource Limits**: Set appropriate memory and CPU limits
5. **Image Size**: Use multi-stage builds to minimize image size

### Performance Monitoring

```bash
# Monitor resource usage
make -f Makefile.docker docker-stats

# Run benchmarks
make -f Makefile.docker docker-test-benchmark

# Check performance logs
docker-compose logs ragger-prod | grep performance
```

## 🔄 Continuous Integration

### GitHub Actions Integration

```yaml
# .github/workflows/docker-ci.yml
name: Docker CI
on: [push, pull_request]
jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Run Docker CI
        run: make -f Makefile.docker docker-ci
```

### GitLab CI Integration

```yaml
# .gitlab-ci.yml
docker-ci:
  stage: test
  script:
    - make -f Makefile.docker docker-ci
  artifacts:
    paths:
      - test_results/
      - coverage/
      - benchmark_results/
```

## 📝 Examples

### Custom Development Setup

```bash
# Create custom development environment
docker-compose -f docker-compose.yml -f docker-compose.dev.yml up -d
```

### Production with Custom Config

```bash
# Deploy with custom configuration
RAGGER_CONFIG_DIR=./custom-config make -f Makefile.docker docker-prod
```

### Testing Specific Components

```bash
# Test only specific components
docker-compose run --rm ragger-test ./bin/ragger-tests --gtest_filter="IndexManager*"
```

This Docker setup provides a complete development and deployment environment for RAGger, making it easy to develop, test, and deploy the application in any environment.
