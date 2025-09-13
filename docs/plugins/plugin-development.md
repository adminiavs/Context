# Plugin Development Guide

## Overview

RAGger's plugin architecture allows you to extend functionality through modular components. This guide covers creating, building, and integrating plugins.

## Plugin Types

### 1. Parser Plugins
Parse and extract information from source code files.

### 2. Ranker Plugins
Rank and score code blocks for relevance.

### 3. Context Plugins
Generate additional context for queries.

### 4. LSP Client Plugins
Integrate with Language Server Protocol.

### 5. Integration Plugins
Connect with external tools and services.

## Plugin Structure

### Basic Plugin Template

```cpp
#include "ragger_plugin_api.h"
#include <iostream>
#include <string>
#include <memory>

namespace {
    // Plugin state
    struct MyPluginState {
        bool initialized;
        std::string config;
        
        MyPluginState() : initialized(false) {}
    };
    
    MyPluginState* g_state = nullptr;
}

extern "C" {

// Required plugin interface functions
int plugin_get_abi_version() {
    return RAGGER_PLUGIN_ABI_VERSION;
}

const char* plugin_get_name() {
    return "MyPlugin";
}

const char* plugin_get_version() {
    return "1.0.0";
}

const char* plugin_get_description() {
    return "A sample RAGger plugin";
}

int plugin_initialize(RaggerCoreHandle core) {
    if (g_state) {
        return RAGGER_ERROR_OPERATION_NOT_SUPPORTED;
    }

    try {
        g_state = new MyPluginState();
        g_state->initialized = true;
        
        std::cout << "MyPlugin: Initialized successfully" << std::endl;
        return RAGGER_SUCCESS;
    } catch (const std::exception& e) {
        std::cerr << "MyPlugin: Initialization failed: " << e.what() << std::endl;
        return RAGGER_ERROR_PLUGIN_LOAD_FAILED;
    }
}

void plugin_shutdown() {
    if (g_state) {
        delete g_state;
        g_state = nullptr;
    }
    std::cout << "MyPlugin: Shutdown" << std::endl;
}

void plugin_register_events(EventCallback callback, void* userData) {
    // Register for events if needed
}

void plugin_unregister_events() {
    // Unregister events if needed
}

const char* plugin_get_capabilities() {
    return R"({
        "my_plugin": {
            "features": ["feature1", "feature2"],
            "version": "1.0.0"
        }
    })";
}

// Plugin-specific functions
int my_plugin_process_data(const char* input, char** output) {
    if (!g_state || !g_state->initialized) {
        return RAGGER_ERROR_OPERATION_NOT_SUPPORTED;
    }
    
    if (!input || !output) {
        return RAGGER_ERROR_INVALID_ARGUMENT;
    }
    
    try {
        // Process input data
        std::string result = processInput(input);
        
        // Allocate output buffer
        *output = new char[result.length() + 1];
        strcpy(*output, result.c_str());
        
        return RAGGER_SUCCESS;
    } catch (const std::exception& e) {
        std::cerr << "MyPlugin: Processing failed: " << e.what() << std::endl;
        return RAGGER_ERROR_PLUGIN_LOAD_FAILED;
    }
}

void my_plugin_free_output(char* output) {
    delete[] output;
}

} // extern "C"
```

## Error Handling in Plugins

### Comprehensive Error Handling

```cpp
// Plugin-specific error codes
enum MyPluginErrorCode {
    MY_PLUGIN_SUCCESS = 0,
    MY_PLUGIN_ERROR_INVALID_INPUT = -1001,
    MY_PLUGIN_ERROR_PROCESSING_FAILED = -1002,
    MY_PLUGIN_ERROR_OUTPUT_TOO_LARGE = -1003
};

int my_plugin_process_with_error_handling(const char* input, char** output) {
    // Input validation
    if (!input) {
        std::cerr << "MyPlugin: Input is null" << std::endl;
        return MY_PLUGIN_ERROR_INVALID_INPUT;
    }
    
    if (strlen(input) == 0) {
        std::cerr << "MyPlugin: Input is empty" << std::endl;
        return MY_PLUGIN_ERROR_INVALID_INPUT;
    }
    
    if (!output) {
        std::cerr << "MyPlugin: Output pointer is null" << std::endl;
        return RAGGER_ERROR_INVALID_ARGUMENT;
    }
    
    // State validation
    if (!g_state || !g_state->initialized) {
        std::cerr << "MyPlugin: Plugin not initialized" << std::endl;
        return RAGGER_ERROR_OPERATION_NOT_SUPPORTED;
    }
    
    try {
        // Process with size limits
        const size_t MAX_OUTPUT_SIZE = 1024 * 1024; // 1MB limit
        std::string result = processInput(input);
        
        if (result.length() > MAX_OUTPUT_SIZE) {
            std::cerr << "MyPlugin: Output too large: " << result.length() << " bytes" << std::endl;
            return MY_PLUGIN_ERROR_OUTPUT_TOO_LARGE;
        }
        
        // Allocate and copy result
        *output = new(std::nothrow) char[result.length() + 1];
        if (!*output) {
            std::cerr << "MyPlugin: Memory allocation failed" << std::endl;
            return RAGGER_ERROR_MEMORY_ALLOCATION;
        }
        
        strcpy(*output, result.c_str());
        return MY_PLUGIN_SUCCESS;
        
    } catch (const std::bad_alloc& e) {
        std::cerr << "MyPlugin: Memory allocation failed: " << e.what() << std::endl;
        return RAGGER_ERROR_MEMORY_ALLOCATION;
    } catch (const std::exception& e) {
        std::cerr << "MyPlugin: Processing failed: " << e.what() << std::endl;
        return MY_PLUGIN_ERROR_PROCESSING_FAILED;
    } catch (...) {
        std::cerr << "MyPlugin: Unknown error occurred" << std::endl;
        return RAGGER_ERROR_UNKNOWN;
    }
}
```

## Building Plugins

### CMakeLists.txt for Plugins

```cmake
# Plugin CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(MyPlugin VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find RAGger
find_package(Ragger REQUIRED)

# Create plugin library
add_library(my_plugin SHARED
    MyPlugin.cpp
)

# Link with RAGger core
target_link_libraries(my_plugin
    PRIVATE
        ragger-core
)

# Include directories
target_include_directories(my_plugin
    PRIVATE
        ${RAGGER_INCLUDE_DIRS}
)

# Compiler definitions
target_compile_definitions(my_plugin
    PRIVATE
        MY_PLUGIN_VERSION_MAJOR=1
        MY_PLUGIN_VERSION_MINOR=0
        MY_PLUGIN_VERSION_PATCH=0
)

# Install plugin
install(TARGETS my_plugin
    LIBRARY DESTINATION lib/ragger/plugins
)
```

### Build Script

```bash
#!/bin/bash
# build_plugin.sh

set -e

PLUGIN_NAME="my_plugin"
BUILD_DIR="build"
INSTALL_DIR="install"

echo "Building $PLUGIN_NAME plugin..."

# Create build directory
mkdir -p $BUILD_DIR
cd $BUILD_DIR

# Configure with CMake
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=../$INSTALL_DIR \
    -DRAGGER_ROOT=/path/to/ragger

# Build
make -j$(nproc)

# Install
make install

echo "Plugin built and installed successfully!"
```

## Plugin Configuration

### Configuration File Format

```json
{
    "plugin_name": "my_plugin",
    "version": "1.0.0",
    "enabled": true,
    "config": {
        "max_processing_time": 5000,
        "memory_limit": "100MB",
        "debug_mode": false,
        "custom_settings": {
            "setting1": "value1",
            "setting2": 42
        }
    },
    "dependencies": [
        "core_plugin",
        "parser_plugin"
    ]
}
```

### Loading Configuration

```cpp
#include <fstream>
#include <nlohmann/json.hpp>

struct PluginConfig {
    std::string name;
    std::string version;
    bool enabled;
    int maxProcessingTime;
    std::string memoryLimit;
    bool debugMode;
    std::map<std::string, std::string> customSettings;
};

PluginConfig loadPluginConfig(const std::string& configPath) {
    try {
        std::ifstream configFile(configPath);
        if (!configFile.is_open()) {
            throw std::runtime_error("Cannot open config file: " + configPath);
        }
        
        nlohmann::json config;
        configFile >> config;
        
        PluginConfig pluginConfig;
        pluginConfig.name = config["plugin_name"];
        pluginConfig.version = config["version"];
        pluginConfig.enabled = config["enabled"];
        pluginConfig.maxProcessingTime = config["config"]["max_processing_time"];
        pluginConfig.memoryLimit = config["config"]["memory_limit"];
        pluginConfig.debugMode = config["config"]["debug_mode"];
        
        // Load custom settings
        if (config["config"].contains("custom_settings")) {
            auto customSettings = config["config"]["custom_settings"];
            for (auto& [key, value] : customSettings.items()) {
                pluginConfig.customSettings[key] = value.get<std::string>();
            }
        }
        
        return pluginConfig;
    } catch (const std::exception& e) {
        std::cerr << "Failed to load plugin config: " << e.what() << std::endl;
        throw;
    }
}
```

## Testing Plugins

### Unit Tests

```cpp
#include <gtest/gtest.h>
#include "MyPlugin.h"

class MyPluginTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize plugin for testing
        ASSERT_EQ(plugin_initialize(nullptr), RAGGER_SUCCESS);
    }
    
    void TearDown() override {
        // Cleanup
        plugin_shutdown();
    }
};

TEST_F(MyPluginTest, Initialization) {
    EXPECT_TRUE(g_state != nullptr);
    EXPECT_TRUE(g_state->initialized);
}

TEST_F(MyPluginTest, ProcessValidInput) {
    const char* input = "test input";
    char* output = nullptr;
    
    int result = my_plugin_process_data(input, &output);
    
    EXPECT_EQ(result, RAGGER_SUCCESS);
    EXPECT_NE(output, nullptr);
    EXPECT_STREQ(output, "processed: test input");
    
    my_plugin_free_output(output);
}

TEST_F(MyPluginTest, ProcessInvalidInput) {
    char* output = nullptr;
    
    // Test null input
    int result = my_plugin_process_data(nullptr, &output);
    EXPECT_EQ(result, RAGGER_ERROR_INVALID_ARGUMENT);
    
    // Test null output pointer
    result = my_plugin_process_data("test", nullptr);
    EXPECT_EQ(result, RAGGER_ERROR_INVALID_ARGUMENT);
}
```

### Integration Tests

```cpp
#include <gtest/gtest.h>
#include "PluginManager.h"

class PluginIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        pluginManager = std::make_unique<Ragger::PluginManager>(nullptr);
    }
    
    std::unique_ptr<Ragger::PluginManager> pluginManager;
};

TEST_F(PluginIntegrationTest, LoadAndExecutePlugin) {
    // Load plugin
    std::filesystem::path pluginPath = "lib/my_plugin.so";
    int result = pluginManager->loadPlugin(pluginPath);
    EXPECT_EQ(result, RAGGER_SUCCESS);
    
    // Execute plugin function
    const char* input = "integration test";
    char* output = nullptr;
    
    // Get plugin function pointer
    auto processFunc = pluginManager->getFunction<int(const char*, char**)>(
        "my_plugin_process_data"
    );
    EXPECT_NE(processFunc, nullptr);
    
    // Execute
    result = processFunc(input, &output);
    EXPECT_EQ(result, RAGGER_SUCCESS);
    EXPECT_NE(output, nullptr);
    
    // Cleanup
    auto freeFunc = pluginManager->getFunction<void(char*)>("my_plugin_free_output");
    if (freeFunc) {
        freeFunc(output);
    }
}
```

## Best Practices

### 1. Error Handling
- Always validate input parameters
- Use appropriate error codes
- Provide meaningful error messages
- Handle exceptions gracefully

### 2. Memory Management
- Use RAII principles
- Avoid memory leaks
- Check allocation failures
- Provide cleanup functions

### 3. Thread Safety
- Document thread safety requirements
- Use appropriate synchronization
- Avoid global state when possible
- Test concurrent access

### 4. Performance
- Optimize for common use cases
- Use efficient data structures
- Implement proper caching
- Profile and benchmark

### 5. Documentation
- Document all public functions
- Provide usage examples
- Include error handling examples
- Keep documentation up-to-date

## Debugging Plugins

### Debug Output

```cpp
#ifdef DEBUG
#define PLUGIN_DEBUG(msg) std::cout << "[DEBUG] " << plugin_get_name() << ": " << msg << std::endl
#else
#define PLUGIN_DEBUG(msg)
#endif

int my_plugin_process_data(const char* input, char** output) {
    PLUGIN_DEBUG("Processing input: " << input);
    
    // ... processing logic ...
    
    PLUGIN_DEBUG("Processing completed successfully");
    return RAGGER_SUCCESS;
}
```

### Logging

```cpp
#include <spdlog/spdlog.h>

int plugin_initialize(RaggerCoreHandle core) {
    try {
        // Setup logging
        auto logger = spdlog::get("ragger_plugins");
        if (!logger) {
            logger = spdlog::stdout_color_mt("ragger_plugins");
        }
        
        logger->info("Initializing {} plugin", plugin_get_name());
        
        // ... initialization ...
        
        logger->info("{} plugin initialized successfully", plugin_get_name());
        return RAGGER_SUCCESS;
    } catch (const std::exception& e) {
        spdlog::error("Failed to initialize {} plugin: {}", plugin_get_name(), e.what());
        return RAGGER_ERROR_PLUGIN_LOAD_FAILED;
    }
}
```

## Plugin Distribution

### Package Structure

```
my_plugin/
├── CMakeLists.txt
├── README.md
├── LICENSE
├── src/
│   └── MyPlugin.cpp
├── include/
│   └── MyPlugin.h
├── config/
│   └── my_plugin.json
├── tests/
│   └── test_my_plugin.cpp
└── docs/
    └── plugin_documentation.md
```

### Installation Script

```bash
#!/bin/bash
# install_plugin.sh

PLUGIN_NAME="my_plugin"
INSTALL_DIR="/usr/local/lib/ragger/plugins"
CONFIG_DIR="/etc/ragger/plugins"

echo "Installing $PLUGIN_NAME plugin..."

# Create directories
sudo mkdir -p $INSTALL_DIR
sudo mkdir -p $CONFIG_DIR

# Install plugin library
sudo cp lib/$PLUGIN_NAME.so $INSTALL_DIR/

# Install configuration
sudo cp config/$PLUGIN_NAME.json $CONFIG_DIR/

# Set permissions
sudo chmod 755 $INSTALL_DIR/$PLUGIN_NAME.so
sudo chmod 644 $CONFIG_DIR/$PLUGIN_NAME.json

echo "Plugin installed successfully!"
echo "Plugin library: $INSTALL_DIR/$PLUGIN_NAME.so"
echo "Configuration: $CONFIG_DIR/$PLUGIN_NAME.json"
```
