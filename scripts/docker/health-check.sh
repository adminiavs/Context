#!/bin/bash

# RAGger Docker Health Check Script
# This script performs comprehensive health checks on RAGger containers

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
CONTAINER_NAME="ragger-production"
HEALTH_CHECK_TIMEOUT=30
MAX_RETRIES=3

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

# Check if container is running
check_container_running() {
    log_info "Checking if container is running..."
    
    if docker ps --format "table {{.Names}}" | grep -q "^${CONTAINER_NAME}$"; then
        log_success "Container $CONTAINER_NAME is running"
        return 0
    else
        log_error "Container $CONTAINER_NAME is not running"
        return 1
    fi
}

# Check container health status
check_container_health() {
    log_info "Checking container health status..."
    
    local health_status=$(docker inspect --format='{{.State.Health.Status}}' $CONTAINER_NAME 2>/dev/null || echo "unknown")
    
    case $health_status in
        "healthy")
            log_success "Container is healthy"
            return 0
            ;;
        "unhealthy")
            log_error "Container is unhealthy"
            return 1
            ;;
        "starting")
            log_warning "Container is starting up"
            return 2
            ;;
        *)
            log_warning "Container health status unknown: $health_status"
            return 2
            ;;
    esac
}

# Check if RAGger binary is accessible
check_ragger_binary() {
    log_info "Checking RAGger binary accessibility..."
    
    local retries=0
    while [ $retries -lt $MAX_RETRIES ]; do
        if docker exec $CONTAINER_NAME /app/bin/ragger-console --version >/dev/null 2>&1; then
            log_success "RAGger binary is accessible"
            return 0
        else
            log_warning "RAGger binary not accessible (attempt $((retries + 1))/$MAX_RETRIES)"
            sleep 5
            ((retries++))
        fi
    done
    
    log_error "RAGger binary is not accessible after $MAX_RETRIES attempts"
    return 1
}

# Check if required files exist
check_required_files() {
    log_info "Checking required files..."
    
    local required_files=(
        "/app/bin/ragger-console"
        "/app/bin/ragger"
        "/app/lib/libragger-core.a"
        "/app/config/ragger.conf"
    )
    
    local missing_files=()
    
    for file in "${required_files[@]}"; do
        if docker exec $CONTAINER_NAME test -f "$file"; then
            log_success "File exists: $file"
        else
            log_error "File missing: $file"
            missing_files+=("$file")
        fi
    done
    
    if [ ${#missing_files[@]} -eq 0 ]; then
        log_success "All required files are present"
        return 0
    else
        log_error "Missing files: ${missing_files[*]}"
        return 1
    fi
}

# Check if ports are accessible
check_ports() {
    log_info "Checking port accessibility..."
    
    # Check if port 8080 is accessible (if web interface is enabled)
    if docker exec $CONTAINER_NAME netstat -tlnp 2>/dev/null | grep -q ":8080"; then
        log_success "Port 8080 is listening"
    else
        log_warning "Port 8080 is not listening (this may be normal if web interface is disabled)"
    fi
    
    return 0
}

# Check container resource usage
check_resource_usage() {
    log_info "Checking resource usage..."
    
    local stats=$(docker stats --no-stream --format "table {{.CPUPerc}}\t{{.MemUsage}}\t{{.MemPerc}}" $CONTAINER_NAME 2>/dev/null | tail -n 1)
    
    if [ -n "$stats" ]; then
        log_info "Resource usage: $stats"
        
        # Extract memory percentage
        local mem_percent=$(echo "$stats" | awk '{print $3}' | sed 's/%//')
        
        if [ "$mem_percent" -gt 90 ]; then
            log_warning "High memory usage: ${mem_percent}%"
        else
            log_success "Memory usage is normal: ${mem_percent}%"
        fi
    else
        log_warning "Could not retrieve resource usage statistics"
    fi
    
    return 0
}

# Check container logs for errors
check_logs() {
    log_info "Checking container logs for errors..."
    
    local error_count=$(docker logs $CONTAINER_NAME 2>&1 | grep -i "error\|exception\|fatal\|panic" | wc -l)
    
    if [ "$error_count" -eq 0 ]; then
        log_success "No errors found in container logs"
    else
        log_warning "Found $error_count potential errors in container logs"
        log_info "Recent errors:"
        docker logs $CONTAINER_NAME 2>&1 | grep -i "error\|exception\|fatal\|panic" | tail -5
    fi
    
    return 0
}

# Check database connectivity
check_database() {
    log_info "Checking database connectivity..."
    
    if docker exec $CONTAINER_NAME test -f "/app/data/index.db"; then
        log_success "Database file exists"
        
        # Check if database is accessible
        if docker exec $CONTAINER_NAME sqlite3 /app/data/index.db "SELECT 1;" >/dev/null 2>&1; then
            log_success "Database is accessible"
        else
            log_error "Database is not accessible"
            return 1
        fi
    else
        log_warning "Database file does not exist (this may be normal for new installations)"
    fi
    
    return 0
}

# Check network connectivity
check_network() {
    log_info "Checking network connectivity..."
    
    # Check if container can resolve DNS
    if docker exec $CONTAINER_NAME nslookup google.com >/dev/null 2>&1; then
        log_success "DNS resolution is working"
    else
        log_warning "DNS resolution may have issues"
    fi
    
    # Check if container can reach external networks
    if docker exec $CONTAINER_NAME ping -c 1 8.8.8.8 >/dev/null 2>&1; then
        log_success "External network connectivity is working"
    else
        log_warning "External network connectivity may have issues"
    fi
    
    return 0
}

# Main health check function
main() {
    log_info "Starting RAGger Docker health check..."
    log_info "Container: $CONTAINER_NAME"
    log_info "Timeout: ${HEALTH_CHECK_TIMEOUT}s"
    log_info "Max retries: $MAX_RETRIES"
    echo ""
    
    local overall_status=0
    
    # Run all health checks
    check_container_running || overall_status=1
    check_container_health || overall_status=1
    check_ragger_binary || overall_status=1
    check_required_files || overall_status=1
    check_ports || overall_status=1
    check_resource_usage || overall_status=1
    check_logs || overall_status=1
    check_database || overall_status=1
    check_network || overall_status=1
    
    echo ""
    if [ $overall_status -eq 0 ]; then
        log_success "All health checks passed!"
        exit 0
    else
        log_error "Some health checks failed!"
        exit 1
    fi
}

# Handle command line arguments
case "${1:-}" in
    --container)
        CONTAINER_NAME="$2"
        shift 2
        ;;
    --timeout)
        HEALTH_CHECK_TIMEOUT="$2"
        shift 2
        ;;
    --retries)
        MAX_RETRIES="$2"
        shift 2
        ;;
    --help)
        echo "Usage: $0 [OPTIONS]"
        echo ""
        echo "Options:"
        echo "  --container NAME    Container name to check (default: ragger-production)"
        echo "  --timeout SECONDS   Health check timeout (default: 30)"
        echo "  --retries COUNT     Maximum retries (default: 3)"
        echo "  --help             Show this help message"
        echo ""
        echo "Examples:"
        echo "  $0                                    # Check default container"
        echo "  $0 --container ragger-dev            # Check development container"
        echo "  $0 --timeout 60 --retries 5          # Custom timeout and retries"
        exit 0
        ;;
esac

# Run main health check
main
