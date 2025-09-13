# Basic Usage Examples

## Overview

This document provides basic examples of using RAGger for common tasks. All examples include proper error handling and follow best practices.

## Prerequisites

```cpp
#include "ragger_plugin_api.h"
#include "EventBus.h"
#include "IndexManager.h"
#include "ContextEngine.h"
#include "ComprehensiveContextGenerator.h"
#include <iostream>
#include <memory>
#include <exception>
```

## Example 1: Basic Indexing and Querying

```cpp
#include <iostream>
#include <memory>
#include <filesystem>

int main() {
    try {
        // Initialize core components
        auto eventBus = std::make_unique<Ragger::EventBus>();
        auto indexManager = std::make_unique<Ragger::IndexManager>();
        auto contextEngine = std::make_unique<Ragger::ContextEngine>(eventBus.get());
        
        // Initialize IndexManager
        int result = indexManager->initialize();
        if (result != RAGGER_SUCCESS) {
            throw std::runtime_error("Failed to initialize IndexManager: " + 
                                   std::to_string(result));
        }
        
        // Index a project directory
        std::filesystem::path projectPath = "/path/to/your/project";
        if (!std::filesystem::exists(projectPath)) {
            throw std::filesystem::filesystem_error(
                "Project path does not exist", 
                projectPath, 
                std::make_error_code(std::errc::no_such_file_or_directory)
            );
        }
        
        std::cout << "Indexing project: " << projectPath << std::endl;
        result = indexManager->indexDirectory(projectPath);
        if (result != RAGGER_SUCCESS) {
            throw std::runtime_error("Failed to index directory: " + 
                                   std::to_string(result));
        }
        
        // Create a context request
        ContextRequest request;
        request.query = "How does the authentication system work?";
        request.maxResults = 10;
        request.fileTypes = {"cpp", "h", "hpp"};
        
        // Generate context
        std::cout << "Generating context for query: " << request.query << std::endl;
        ContextResponse response = contextEngine->generateContext(&request);
        if (response.status != RAGGER_SUCCESS) {
            throw std::runtime_error("Context generation failed: " + 
                                   std::to_string(response.status));
        }
        
        // Display results
        std::cout << "\nFound " << response.results.size() << " relevant results:\n" << std::endl;
        for (size_t i = 0; i < response.results.size(); ++i) {
            const auto& result = response.results[i];
            std::cout << "Result " << (i + 1) << ":" << std::endl;
            std::cout << "  File: " << result.filePath << std::endl;
            std::cout << "  Score: " << result.relevanceScore << std::endl;
            std::cout << "  Content: " << result.content.substr(0, 200) << "..." << std::endl;
            std::cout << std::endl;
        }
        
        return 0;
        
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
        std::cerr << "Path: " << e.path1() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown error occurred" << std::endl;
        return 1;
    }
}
```

## Example 2: Using Comprehensive Context Generation

```cpp
#include <iostream>
#include <memory>

int main() {
    try {
        // Initialize comprehensive context generator
        auto contextGenerator = std::make_unique<Ragger::Core::ComprehensiveContextGenerator>();
        
        if (!contextGenerator->initialize()) {
            throw std::runtime_error("Failed to initialize context generator");
        }
        
        // Generate comprehensive context
        std::string filePath = "src/main.cpp";
        std::string query = "How does the main function initialize the application?";
        int startLine = 1;
        int endLine = 100;
        
        std::cout << "Generating comprehensive context..." << std::endl;
        std::cout << "File: " << filePath << std::endl;
        std::cout << "Query: " << query << std::endl;
        std::cout << "Lines: " << startLine << "-" << endLine << std::endl;
        std::cout << std::string(80, '=') << std::endl;
        
        std::string context = contextGenerator->generateComprehensiveContext(
            filePath, query, startLine, endLine
        );
        
        std::cout << context << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
```

## Example 3: Event-Driven Processing

```cpp
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

class RaggerApplication {
private:
    std::unique_ptr<Ragger::EventBus> eventBus;
    std::unique_ptr<Ragger::IndexManager> indexManager;
    bool running;
    
public:
    RaggerApplication() : running(false) {
        eventBus = std::make_unique<Ragger::EventBus>();
        indexManager = std::make_unique<Ragger::IndexManager>();
    }
    
    bool initialize() {
        try {
            // Initialize IndexManager
            int result = indexManager->initialize();
            if (result != RAGGER_SUCCESS) {
                std::cerr << "Failed to initialize IndexManager: " << result << std::endl;
                return false;
            }
            
            // Subscribe to events
            eventBus->subscribe("file_indexed", [this](const Ragger::Event& event) {
                std::string filePath = event.getData<std::string>();
                std::cout << "Indexed file: " << filePath << std::endl;
            });
            
            eventBus->subscribe("indexing_complete", [this](const Ragger::Event& event) {
                std::cout << "Indexing completed!" << std::endl;
                running = false;
            });
            
            eventBus->subscribe("error_occurred", [this](const Ragger::Event& event) {
                std::string error = event.getData<std::string>();
                std::cerr << "Error: " << error << std::endl;
                running = false;
            });
            
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Initialization failed: " << e.what() << std::endl;
            return false;
        }
    }
    
    void run() {
        if (!initialize()) {
            return;
        }
        
        running = true;
        
        // Start indexing in a separate thread
        std::thread indexingThread([this]() {
            try {
                std::filesystem::path projectPath = "/path/to/project";
                int result = indexManager->indexDirectory(projectPath);
                
                if (result == RAGGER_SUCCESS) {
                    eventBus->emit("indexing_complete", std::string(""));
                } else {
                    eventBus->emit("error_occurred", 
                                 std::string("Indexing failed: " + std::to_string(result)));
                }
            } catch (const std::exception& e) {
                eventBus->emit("error_occurred", std::string(e.what()));
            }
        });
        
        // Main event loop
        while (running) {
            // Process events (simplified)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Wait for indexing thread to complete
        if (indexingThread.joinable()) {
            indexingThread.join();
        }
    }
    
    ~RaggerApplication() {
        if (indexManager) {
            indexManager->shutdown();
        }
    }
};

int main() {
    try {
        RaggerApplication app;
        app.run();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Application error: " << e.what() << std::endl;
        return 1;
    }
}
```

## Example 4: Parallel File Processing

```cpp
#include <iostream>
#include <vector>
#include <filesystem>
#include <execution>
#include <future>
#include <mutex>

class ParallelIndexer {
private:
    std::unique_ptr<Ragger::IndexManager> indexManager;
    std::mutex outputMutex;
    std::atomic<int> processedFiles{0};
    std::atomic<int> failedFiles{0};
    
public:
    ParallelIndexer() {
        indexManager = std::make_unique<Ragger::IndexManager>();
    }
    
    bool initialize() {
        int result = indexManager->initialize();
        if (result != RAGGER_SUCCESS) {
            std::cerr << "Failed to initialize IndexManager: " << result << std::endl;
            return false;
        }
        return true;
    }
    
    void processFile(const std::filesystem::path& filePath) {
        try {
            int result = indexManager->indexFile(filePath);
            if (result == RAGGER_SUCCESS) {
                processedFiles++;
                std::lock_guard<std::mutex> lock(outputMutex);
                std::cout << "Processed: " << filePath << std::endl;
            } else {
                failedFiles++;
                std::lock_guard<std::mutex> lock(outputMutex);
                std::cerr << "Failed to process: " << filePath << " (error: " << result << ")" << std::endl;
            }
        } catch (const std::exception& e) {
            failedFiles++;
            std::lock_guard<std::mutex> lock(outputMutex);
            std::cerr << "Exception processing " << filePath << ": " << e.what() << std::endl;
        }
    }
    
    void processDirectory(const std::filesystem::path& directory) {
        if (!std::filesystem::exists(directory)) {
            throw std::filesystem::filesystem_error(
                "Directory does not exist", 
                directory, 
                std::make_error_code(std::errc::no_such_file_or_directory)
            );
        }
        
        // Collect all files to process
        std::vector<std::filesystem::path> files;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                const auto& path = entry.path();
                if (path.extension() == ".cpp" || path.extension() == ".h" || 
                    path.extension() == ".hpp" || path.extension() == ".c") {
                    files.push_back(path);
                }
            }
        }
        
        std::cout << "Found " << files.size() << " files to process" << std::endl;
        
        // Process files in parallel
        auto startTime = std::chrono::high_resolution_clock::now();
        
        std::for_each(std::execution::par_unseq, files.begin(), files.end(),
            [this](const std::filesystem::path& file) {
                processFile(file);
            });
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        std::cout << "\nProcessing completed:" << std::endl;
        std::cout << "  Files processed: " << processedFiles.load() << std::endl;
        std::cout << "  Files failed: " << failedFiles.load() << std::endl;
        std::cout << "  Total time: " << duration.count() << " ms" << std::endl;
        std::cout << "  Average time per file: " << 
                     (duration.count() / static_cast<double>(files.size())) << " ms" << std::endl;
    }
    
    ~ParallelIndexer() {
        if (indexManager) {
            indexManager->shutdown();
        }
    }
};

int main() {
    try {
        ParallelIndexer indexer;
        
        if (!indexer.initialize()) {
            return 1;
        }
        
        std::filesystem::path projectPath = "/path/to/your/project";
        indexer.processDirectory(projectPath);
        
        return 0;
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
```

## Example 5: Configuration Management

```cpp
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

struct RaggerConfig {
    std::string projectPath;
    int maxResults;
    std::vector<std::string> fileTypes;
    bool enableParallelProcessing;
    int threadCount;
    std::string databasePath;
    bool enableLogging;
    std::string logLevel;
};

class ConfigManager {
public:
    static RaggerConfig loadConfig(const std::string& configPath) {
        try {
            std::ifstream configFile(configPath);
            if (!configFile.is_open()) {
                throw std::runtime_error("Cannot open config file: " + configPath);
            }
            
            nlohmann::json config;
            configFile >> config;
            
            RaggerConfig raggerConfig;
            
            // Load required fields
            if (!config.contains("project_path")) {
                throw std::runtime_error("Missing required field: project_path");
            }
            raggerConfig.projectPath = config["project_path"];
            
            // Load optional fields with defaults
            raggerConfig.maxResults = config.value("max_results", 10);
            raggerConfig.enableParallelProcessing = config.value("enable_parallel_processing", true);
            raggerConfig.threadCount = config.value("thread_count", std::thread::hardware_concurrency());
            raggerConfig.databasePath = config.value("database_path", "ragger.db");
            raggerConfig.enableLogging = config.value("enable_logging", true);
            raggerConfig.logLevel = config.value("log_level", "info");
            
            // Load file types
            if (config.contains("file_types") && config["file_types"].is_array()) {
                for (const auto& type : config["file_types"]) {
                    raggerConfig.fileTypes.push_back(type.get<std::string>());
                }
            } else {
                // Default file types
                raggerConfig.fileTypes = {"cpp", "h", "hpp", "c", "cc", "cxx"};
            }
            
            return raggerConfig;
        } catch (const nlohmann::json::exception& e) {
            throw std::runtime_error("JSON parsing error: " + std::string(e.what()));
        } catch (const std::exception& e) {
            throw std::runtime_error("Config loading error: " + std::string(e.what()));
        }
    }
    
    static void saveConfig(const RaggerConfig& config, const std::string& configPath) {
        try {
            nlohmann::json configJson;
            configJson["project_path"] = config.projectPath;
            configJson["max_results"] = config.maxResults;
            configJson["file_types"] = config.fileTypes;
            configJson["enable_parallel_processing"] = config.enableParallelProcessing;
            configJson["thread_count"] = config.threadCount;
            configJson["database_path"] = config.databasePath;
            configJson["enable_logging"] = config.enableLogging;
            configJson["log_level"] = config.logLevel;
            
            std::ofstream configFile(configPath);
            if (!configFile.is_open()) {
                throw std::runtime_error("Cannot open config file for writing: " + configPath);
            }
            
            configFile << configJson.dump(4) << std::endl;
        } catch (const std::exception& e) {
            throw std::runtime_error("Config saving error: " + std::string(e.what()));
        }
    }
};

int main() {
    try {
        // Load configuration
        std::string configPath = "ragger_config.json";
        RaggerConfig config = ConfigManager::loadConfig(configPath);
        
        std::cout << "Configuration loaded:" << std::endl;
        std::cout << "  Project path: " << config.projectPath << std::endl;
        std::cout << "  Max results: " << config.maxResults << std::endl;
        std::cout << "  File types: ";
        for (const auto& type : config.fileTypes) {
            std::cout << type << " ";
        }
        std::cout << std::endl;
        std::cout << "  Parallel processing: " << (config.enableParallelProcessing ? "enabled" : "disabled") << std::endl;
        std::cout << "  Thread count: " << config.threadCount << std::endl;
        std::cout << "  Database path: " << config.databasePath << std::endl;
        std::cout << "  Logging: " << (config.enableLogging ? "enabled" : "disabled") << std::endl;
        std::cout << "  Log level: " << config.logLevel << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
```

## Error Handling Best Practices

### 1. Always Check Return Codes
```cpp
int result = indexManager->indexFile(filePath);
if (result != RAGGER_SUCCESS) {
    std::cerr << "Indexing failed with error code: " << result << std::endl;
    return result;
}
```

### 2. Use Exceptions for Exceptional Cases
```cpp
try {
    auto response = contextEngine->generateContext(&request);
    // Process response
} catch (const Ragger::ContextEngineException& e) {
    std::cerr << "Context generation failed: " << e.what() << std::endl;
    return RAGGER_ERROR_CONTEXT_GENERATION_FAILED;
}
```

### 3. Validate Input Parameters
```cpp
if (filePath.empty()) {
    throw std::invalid_argument("File path cannot be empty");
}

if (!std::filesystem::exists(filePath)) {
    throw std::filesystem::filesystem_error(
        "File does not exist", 
        filePath, 
        std::make_error_code(std::errc::no_such_file_or_directory)
    );
}
```

### 4. Handle Resource Cleanup
```cpp
class ResourceManager {
private:
    std::unique_ptr<Ragger::IndexManager> indexManager;
    
public:
    ~ResourceManager() {
        if (indexManager) {
            indexManager->shutdown();
        }
    }
};
```

## Performance Tips

1. **Use parallel processing** for file indexing
2. **Cache results** when possible
3. **Limit result sets** to avoid memory issues
4. **Use appropriate data structures** for your use case
5. **Profile your code** to identify bottlenecks
