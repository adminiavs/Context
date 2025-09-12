#include "core/EventBus.h"
#include "core/PluginManager.h"
#include "core/IndexManager.h"
#include "core/ContextEngine.h"
#include "core/ConfigManager.h"
#include "core/Logger.h"
#include "core/FileUtils.h"
#include "ragger_plugin_api.h"
// spdlog disabled
#include <iostream>
#include <csignal>
#include <atomic>
#include <filesystem>

namespace Ragger {

// Global application state
std::atomic<bool> running(true);

// Signal handler for graceful shutdown
void signalHandler(int signal) {
    std::cout << "Received signal " << signal << ", shutting down..." << std::endl;
    running = false;
}

// Core API implementation
class RaggerCore {
public:
    RaggerCore() {
        // Initialize core components
        eventBus_ = std::make_unique<EventBus>();
        pluginManager_ = std::make_unique<PluginManager>(nullptr);
        indexManager_ = std::make_unique<IndexManager>();
        contextEngine_ = std::make_unique<ContextEngine>(nullptr);
        configManager_ = std::make_unique<ConfigManager>();
        logger_ = std::make_unique<Logger>();
    }

    ~RaggerCore() = default;

    // Event system
    int emit_event(const EventData* event) {
        return eventBus_->emitEvent(event);
    }

    int subscribe_event(EventType type, EventCallback callback, void* userData)  {
        return eventBus_->subscribe(type, callback, userData);
    }

    int unsubscribe_event(EventType type, EventCallback callback)  {
        return eventBus_->unsubscribe(type, callback);
    }

    // Logging
    void log_info(const char* message)  {
        std::cout << message << std::endl;
    }

    void log_warning(const char* message)  {
        std::cout << "WARNING: " << message << std::endl;
    }

    void log_error(const char* message)  {
        std::cerr << "ERROR: " << message << std::endl;
    }

    // Configuration
    int get_config_string(const char* key, const char** value)  {
        return configManager_->getString(key, value);
    }

    int get_config_int(const char* key, int* value)  {
        return configManager_->getInt(key, value);
    }

    int get_config_float(const char* key, float* value)  {
        return configManager_->getFloat(key, value);
    }

    // Resource management
    void* allocate_memory(size_t size)  {
        return new char[size];
    }

    void free_memory(void* ptr)  {
        delete[] static_cast<char*>(ptr);
    }

    // File system utilities
    int read_file(const char* path, char** content, size_t* size)  {
        return FileUtils::readFile(fs::path(path), content, size);
    }

    int write_file(const char* path, const char* content, size_t size)  {
        return FileUtils::writeFile(fs::path(path), content, size);
    }

    int get_file_hash(const char* path, char* hash, size_t hashSize)  {
        return FileUtils::getFileHash(fs::path(path), hash, hashSize);
    }

    // Component access
    EventBus* getEventBus() { return eventBus_.get(); }
    PluginManager* getPluginManager() { return pluginManager_.get(); }
    IndexManager* getIndexManager() { return indexManager_.get(); }
    ContextEngine* getContextEngine() { return contextEngine_.get(); }
    ConfigManager* getConfigManager() { return configManager_.get(); }

private:
    std::unique_ptr<EventBus> eventBus_;
    std::unique_ptr<PluginManager> pluginManager_;
    std::unique_ptr<IndexManager> indexManager_;
    std::unique_ptr<ContextEngine> contextEngine_;
    std::unique_ptr<ConfigManager> configManager_;
    std::unique_ptr<Logger> logger_;
};

} // namespace Ragger

int main(int argc, char* argv[]) {
    // Set up signal handlers
    std::signal(SIGINT, Ragger::signalHandler);
    std::signal(SIGTERM, Ragger::signalHandler);

    // Initialize logging
    std::cout << "RAGger starting up..." << std::endl;

    try {
        // Create core application
        Ragger::RaggerCore core;

        // Load configuration
        if (argc > 1) {
            core.getConfigManager()->loadFromFile(argv[1]);
        } else {
            core.getConfigManager()->loadDefaults();
        }

        // Load plugins
        std::cout << "Loading plugins..." << std::endl;
        core.getPluginManager()->loadPluginsFromDirectory("lib/ragger/plugins");

        // Initialize indexing
        std::cout << "Initializing index..." << std::endl;
        core.getIndexManager()->initialize();

        // Start main application loop
        std::cout << "RAGger ready. Press Ctrl+C to exit." << std::endl;

        while (Ragger::running) {
            // Main application logic would go here
            // For now, just sleep and check for shutdown
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            // Process any pending events
            // UI updates, plugin communication, etc.
        }

        std::cout << "RAGger shutting down..." << std::endl;

        // Cleanup happens automatically in destructors

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "RAGger shutdown complete." << std::endl;
    return 0;
}
