# RAGger Core API Documentation

## Overview

The RAGger Core API provides the fundamental functionality for code indexing, retrieval, and context generation. This document describes the main classes, methods, and error handling mechanisms.

## Core Classes

### EventBus

The EventBus manages communication between different components of RAGger.

```cpp
#include "EventBus.h"

// Create an event bus
Ragger::EventBus* eventBus = new Ragger::EventBus();

// Subscribe to events
eventBus->subscribe("file_indexed", [](const Event& event) {
    std::cout << "File indexed: " << event.getData<std::string>() << std::endl;
});

// Emit events
eventBus->emit("file_indexed", std::string("example.cpp"));
```

**Error Handling:**
```cpp
try {
    eventBus->emit("invalid_event", data);
} catch (const Ragger::EventBusException& e) {
    std::cerr << "Event bus error: " << e.what() << std::endl;
}
```

### IndexManager

Manages code indexing and retrieval operations.

```cpp
#include "IndexManager.h"

Ragger::IndexManager indexManager;

// Initialize with error handling
try {
    int result = indexManager.initialize();
    if (result != RAGGER_SUCCESS) {
        throw std::runtime_error("Failed to initialize IndexManager");
    }
} catch (const std::exception& e) {
    std::cerr << "IndexManager initialization failed: " << e.what() << std::endl;
    return -1;
}

// Index a directory
try {
    std::filesystem::path projectPath = "/path/to/project";
    int result = indexManager.indexDirectory(projectPath);
    if (result != RAGGER_SUCCESS) {
        throw std::runtime_error("Failed to index directory");
    }
} catch (const std::filesystem::filesystem_error& e) {
    std::cerr << "Filesystem error: " << e.what() << std::endl;
} catch (const std::exception& e) {
    std::cerr << "Indexing error: " << e.what() << std::endl;
}
```

### ContextEngine

Generates context for AI queries.

```cpp
#include "ContextEngine.h"

Ragger::ContextEngine contextEngine(eventBus);

// Create a context request
ContextRequest request;
request.query = "How does the authentication system work?";
request.maxResults = 10;
request.fileTypes = {"cpp", "h", "hpp"};

try {
    ContextResponse response = contextEngine.generateContext(&request);
    if (response.status != RAGGER_SUCCESS) {
        throw std::runtime_error("Context generation failed");
    }
    
    // Process results
    for (const auto& result : response.results) {
        std::cout << "File: " << result.filePath << std::endl;
        std::cout << "Score: " << result.relevanceScore << std::endl;
        std::cout << "Content: " << result.content << std::endl;
    }
} catch (const Ragger::ContextEngineException& e) {
    std::cerr << "Context engine error: " << e.what() << std::endl;
} catch (const std::exception& e) {
    std::cerr << "Unexpected error: " << e.what() << std::endl;
}
```

### ComprehensiveContextGenerator

Generates development-lifecycle-aware context.

```cpp
#include "ComprehensiveContextGenerator.h"

Ragger::Core::ComprehensiveContextGenerator contextGenerator;

try {
    if (!contextGenerator.initialize()) {
        throw std::runtime_error("Failed to initialize context generator");
    }
    
    std::string context = contextGenerator.generateComprehensiveContext(
        "src/main.cpp", 
        "How does the main function work?", 
        1, 
        50
    );
    
    std::cout << "Generated context:\n" << context << std::endl;
} catch (const std::exception& e) {
    std::cerr << "Context generation error: " << e.what() << std::endl;
}
```

## Error Handling

### Error Codes

RAGger uses a comprehensive error code system:

```cpp
enum RaggerErrorCode {
    RAGGER_SUCCESS = 0,
    RAGGER_ERROR_INVALID_ARGUMENT = -1,
    RAGGER_ERROR_FILE_NOT_FOUND = -2,
    RAGGER_ERROR_PERMISSION_DENIED = -3,
    RAGGER_ERROR_DATABASE_ERROR = -4,
    RAGGER_ERROR_PLUGIN_LOAD_FAILED = -5,
    RAGGER_ERROR_MEMORY_ALLOCATION = -6,
    RAGGER_ERROR_THREAD_CREATION = -7,
    RAGGER_ERROR_OPERATION_NOT_SUPPORTED = -8,
    RAGGER_ERROR_TIMEOUT = -9,
    RAGGER_ERROR_NETWORK_ERROR = -10
};
```

### Exception Classes

```cpp
// Base exception class
class RaggerException : public std::exception {
public:
    RaggerException(const std::string& message, RaggerErrorCode code = RAGGER_ERROR_UNKNOWN);
    const char* what() const noexcept override;
    RaggerErrorCode getErrorCode() const;
};

// Specific exception types
class EventBusException : public RaggerException;
class IndexManagerException : public RaggerException;
class ContextEngineException : public RaggerException;
class PluginManagerException : public RaggerException;
```

### Error Handling Best Practices

1. **Always check return codes:**
```cpp
int result = indexManager.indexFile(filePath);
if (result != RAGGER_SUCCESS) {
    // Handle error appropriately
    return result;
}
```

2. **Use exceptions for exceptional cases:**
```cpp
try {
    auto result = contextEngine.generateContext(&request);
    // Process result
} catch (const Ragger::ContextEngineException& e) {
    // Log error and handle gracefully
    logger.error("Context generation failed: {}", e.what());
    return RAGGER_ERROR_CONTEXT_GENERATION_FAILED;
}
```

3. **Provide meaningful error messages:**
```cpp
if (filePath.empty()) {
    throw Ragger::IndexManagerException(
        "File path cannot be empty", 
        RAGGER_ERROR_INVALID_ARGUMENT
    );
}
```

## Threading and Concurrency

### Thread-Safe Operations

Most RAGger operations are thread-safe. However, some operations require exclusive access:

```cpp
// Thread-safe operations
std::string context = contextGenerator.generateComprehensiveContext(...);

// Operations requiring exclusive access
std::lock_guard<std::mutex> lock(indexManagerMutex);
int result = indexManager.rebuildIndex();
```

### Parallel Processing

RAGger supports parallel processing for indexing operations:

```cpp
#include <execution>

// Parallel file indexing
std::vector<std::filesystem::path> files = getFilesToIndex();
std::for_each(std::execution::par_unseq, files.begin(), files.end(), 
    [&](const std::filesystem::path& file) {
        try {
            indexManager.indexFile(file);
        } catch (const std::exception& e) {
            std::cerr << "Failed to index " << file << ": " << e.what() << std::endl;
        }
    });
```

## Memory Management

### Smart Pointers

RAGger uses smart pointers for automatic memory management:

```cpp
// Use unique_ptr for exclusive ownership
std::unique_ptr<Ragger::IndexManager> indexManager = 
    std::make_unique<Ragger::IndexManager>();

// Use shared_ptr for shared ownership
std::shared_ptr<Ragger::EventBus> eventBus = 
    std::make_shared<Ragger::EventBus>();
```

### Resource Cleanup

Always ensure proper cleanup:

```cpp
class RaggerApplication {
private:
    std::unique_ptr<Ragger::IndexManager> indexManager;
    std::unique_ptr<Ragger::ContextEngine> contextEngine;
    
public:
    ~RaggerApplication() {
        // Smart pointers automatically clean up
        // Additional cleanup if needed
        if (indexManager) {
            indexManager->shutdown();
        }
    }
};
```

## Performance Considerations

### Database Optimization

- Use prepared statements for repeated queries
- Ensure proper indexing on database tables
- Use transactions for bulk operations

### Memory Usage

- Monitor memory usage with large codebases
- Use streaming for large file processing
- Implement proper caching strategies

### Threading

- Use thread pools for I/O operations
- Avoid blocking operations in main thread
- Use lock-free data structures where possible
