#pragma once

#include "ragger_plugin_api.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <filesystem>

namespace fs = std::filesystem;

namespace Ragger {

class PluginManager {
public:
    explicit PluginManager(RaggerCoreAPI* coreAPI);
    ~PluginManager();

    // Plugin loading and unloading
    int loadPlugin(const fs::path& pluginPath);
    int unloadPlugin(const std::string& pluginName);
    int reloadPlugin(const std::string& pluginName);

    // Bulk operations
    int loadPluginsFromDirectory(const fs::path& directory);
    int unloadAllPlugins();
    int reloadAllPlugins();

    // Plugin discovery
    std::vector<std::string> getLoadedPlugins() const;
    bool isPluginLoaded(const std::string& pluginName) const;
    PluginType getPluginType(const std::string& pluginName) const;

    // Plugin interface access
    const PluginInterface* getPluginInterface(const std::string& pluginName) const;
    PluginHandle getPluginHandle(const std::string& pluginName) const;

    // Plugin directories
    void addPluginDirectory(const fs::path& directory);
    void removePluginDirectory(const fs::path& directory);
    std::vector<fs::path> getPluginDirectories() const;

    // User and project plugin support
    void setUserPluginDirectory(const fs::path& directory);
    void setProjectPluginDirectory(const fs::path& directory);

    // Plugin metadata
    struct PluginInfo {
        std::string name;
        std::string version;
        std::string description;
        PluginType type;
        fs::path path;
        bool loaded;
        int abiVersion;
        std::string capabilities; // JSON string
    };

    const PluginInfo* getPluginInfo(const std::string& pluginName) const;
    std::vector<PluginInfo> getAllPluginInfo() const;

    // Error handling and recovery
    struct PluginError {
        std::string pluginName;
        std::string errorMessage;
        int errorCode;
        std::chrono::system_clock::time_point timestamp;
    };

    const std::vector<PluginError>& getPluginErrors() const;
    void clearPluginErrors();

    // Plugin sandboxing (future feature)
    void enableSandboxing(bool enabled);
    bool isSandboxingEnabled() const;

private:
    struct LoadedPlugin {
        std::string name;
        PluginHandle handle;
        PluginInterface interface;
        PluginInfo info;
        void* libraryHandle; // dlopen handle
        bool sandboxed;

        LoadedPlugin() : handle(nullptr), libraryHandle(nullptr), sandboxed(false) {}
    };

    RaggerCoreAPI* coreAPI_;
    std::unordered_map<std::string, std::unique_ptr<LoadedPlugin>> loadedPlugins_;
    std::vector<fs::path> pluginDirectories_;
    fs::path userPluginDirectory_;
    fs::path projectPluginDirectory_;
    std::vector<PluginError> pluginErrors_;
    bool sandboxingEnabled_;

    // Plugin loading helpers
    int loadPluginLibrary(const fs::path& pluginPath, LoadedPlugin& plugin);
    int initializePlugin(LoadedPlugin& plugin);
    void unloadPluginLibrary(LoadedPlugin& plugin);

    // Plugin discovery helpers
    std::vector<fs::path> discoverPlugins(const fs::path& directory) const;
    bool isValidPluginFile(const fs::path& path) const;
    std::string extractPluginNameFromPath(const fs::path& path) const;

    // ABI compatibility
    int checkPluginABI(const LoadedPlugin& plugin) const;

    // Error reporting
    void reportPluginError(const std::string& pluginName, const std::string& message, int errorCode);

    // Prevent copying
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;
};

} // namespace Ragger
