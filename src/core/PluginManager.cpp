#include "PluginManager.h"
#include <dlfcn.h>
#include <iostream>
#include <algorithm>
// spdlog and nlohmann/json disabled

namespace Ragger {

PluginManager::PluginManager(RaggerCoreAPI* coreAPI)
    : coreAPI_(coreAPI), sandboxingEnabled_(false) {
    // Set default plugin directories
    addPluginDirectory(fs::current_path() / "lib" / "ragger" / "plugins");

    // Initialize user plugin directory
    const char* home = std::getenv("HOME");
    if (home) {
        userPluginDirectory_ = fs::path(home) / ".ragger" / "plugins";
        addPluginDirectory(userPluginDirectory_);
    }

    // Initialize project plugin directory
    projectPluginDirectory_ = fs::current_path() / ".ragger" / "plugins";
    addPluginDirectory(projectPluginDirectory_);
}

PluginManager::~PluginManager() {
    unloadAllPlugins();
}

int PluginManager::loadPlugin(const fs::path& pluginPath) {
    if (!fs::exists(pluginPath)) {
        reportPluginError("", "Plugin file does not exist: " + pluginPath.string(), RAGGER_ERROR_FILE_NOT_FOUND);
        return RAGGER_ERROR_FILE_NOT_FOUND;
    }

    std::string pluginName = extractPluginNameFromPath(pluginPath);

    // Check if already loaded
    if (loadedPlugins_.find(pluginName) != loadedPlugins_.end()) {
        std::cout << "PluginManager: Plugin '" << pluginName << "' already loaded" << std::endl;
        return RAGGER_ERROR_INVALID_ARGUMENT;
    }

    auto plugin = std::make_unique<LoadedPlugin>();
    plugin->name = pluginName;
    plugin->info.path = pluginPath;
    plugin->info.loaded = false;

    // Load the plugin library
    int result = loadPluginLibrary(pluginPath, *plugin);
    if (result != RAGGER_SUCCESS) {
        reportPluginError(pluginName, "Failed to load plugin library", result);
        return result;
    }

    // Check ABI compatibility
    result = checkPluginABI(*plugin);
    if (result != RAGGER_SUCCESS) {
        unloadPluginLibrary(*plugin);
        reportPluginError(pluginName, "ABI version mismatch", result);
        return result;
    }

    // Initialize the plugin
    result = initializePlugin(*plugin);
    if (result != RAGGER_SUCCESS) {
        unloadPluginLibrary(*plugin);
        reportPluginError(pluginName, "Failed to initialize plugin", result);
        return result;
    }

    plugin->info.loaded = true;
    loadedPlugins_[pluginName] = std::move(plugin);

    std::cout << "PluginManager: Successfully loaded plugin '" << pluginName << "'" << std::endl;
    return RAGGER_SUCCESS;
}

int PluginManager::unloadPlugin(const std::string& pluginName) {
    auto it = loadedPlugins_.find(pluginName);
    if (it == loadedPlugins_.end()) {
        return RAGGER_ERROR_INVALID_ARGUMENT;
    }

    auto& plugin = *it->second;

    // Call plugin shutdown
    try {
        if (plugin.interface.api) {
            // Plugin-specific cleanup if needed
        }
        plugin.interface.api = nullptr; // Clear interface
    } catch (const std::exception& e) {
        std::cerr << "PluginManager: Exception during plugin shutdown: " << e.what() << std::endl;
    }

    // Unload library
    unloadPluginLibrary(plugin);

    loadedPlugins_.erase(it);

    std::cout << "PluginManager: Unloaded plugin '" << pluginName << "'" << std::endl;
    return RAGGER_SUCCESS;
}

int PluginManager::reloadPlugin(const std::string& pluginName) {
    auto it = loadedPlugins_.find(pluginName);
    if (it == loadedPlugins_.end()) {
        return RAGGER_ERROR_INVALID_ARGUMENT;
    }

    fs::path pluginPath = it->second->info.path;

    // Unload first
    int result = unloadPlugin(pluginName);
    if (result != RAGGER_SUCCESS) {
        return result;
    }

    // Load again
    return loadPlugin(pluginPath);
}

int PluginManager::loadPluginsFromDirectory(const fs::path& directory) {
    if (!fs::exists(directory) || !fs::is_directory(directory)) {
        std::cout << "PluginManager: Plugin directory does not exist: " << directory.string() << std::endl;
        return RAGGER_ERROR_FILE_NOT_FOUND;
    }

    auto pluginPaths = discoverPlugins(directory);
    int loadedCount = 0;

    for (const auto& path : pluginPaths) {
        if (loadPlugin(path) == RAGGER_SUCCESS) {
            loadedCount++;
        }
    }

    std::cout << "PluginManager: Loaded " << loadedCount << " plugins from directory " << directory.string() << std::endl;
    return loadedCount;
}

int PluginManager::unloadAllPlugins() {
    int unloadedCount = 0;
    std::vector<std::string> pluginNames;

    // Collect names first to avoid iterator invalidation
    for (const auto& pair : loadedPlugins_) {
        pluginNames.push_back(pair.first);
    }

    for (const auto& name : pluginNames) {
        if (unloadPlugin(name) == RAGGER_SUCCESS) {
            unloadedCount++;
        }
    }

    std::cout << "PluginManager: Unloaded " << unloadedCount << " plugins" << std::endl;
    return unloadedCount;
}

int PluginManager::reloadAllPlugins() {
    std::vector<fs::path> pluginPaths;

    // Collect paths first
    for (const auto& pair : loadedPlugins_) {
        pluginPaths.push_back(pair.second->info.path);
    }

    // Unload all
    unloadAllPlugins();

    // Load all again
    int loadedCount = 0;
    for (const auto& path : pluginPaths) {
        if (loadPlugin(path) == RAGGER_SUCCESS) {
            loadedCount++;
        }
    }

    return loadedCount;
}

std::vector<std::string> PluginManager::getLoadedPlugins() const {
    std::vector<std::string> names;
    for (const auto& pair : loadedPlugins_) {
        names.push_back(pair.first);
    }
    return names;
}

bool PluginManager::isPluginLoaded(const std::string& pluginName) const {
    return loadedPlugins_.find(pluginName) != loadedPlugins_.end();
}

PluginType PluginManager::getPluginType(const std::string& pluginName) const {
    auto it = loadedPlugins_.find(pluginName);
    if (it != loadedPlugins_.end()) {
        return it->second->interface.type;
    }
    return static_cast<PluginType>(0); // Invalid
}

const PluginInterface* PluginManager::getPluginInterface(const std::string& pluginName) const {
    auto it = loadedPlugins_.find(pluginName);
    if (it != loadedPlugins_.end()) {
        return &it->second->interface;
    }
    return nullptr;
}

PluginHandle PluginManager::getPluginHandle(const std::string& pluginName) const {
    auto it = loadedPlugins_.find(pluginName);
    if (it != loadedPlugins_.end()) {
        return it->second->handle;
    }
    return nullptr;
}

void PluginManager::addPluginDirectory(const fs::path& directory) {
    if (fs::exists(directory) && fs::is_directory(directory)) {
        pluginDirectories_.push_back(directory);
        std::cout << "PluginManager: Added plugin directory: " << directory.string() << std::endl;
    }
}

void PluginManager::removePluginDirectory(const fs::path& directory) {
    auto it = std::remove_if(pluginDirectories_.begin(), pluginDirectories_.end(),
        [&directory](const fs::path& path) { return path == directory; });
    if (it != pluginDirectories_.end()) {
        pluginDirectories_.erase(it, pluginDirectories_.end());
        std::cout << "PluginManager: Removed plugin directory: " << directory.string() << std::endl;
    }
}

std::vector<fs::path> PluginManager::getPluginDirectories() const {
    return pluginDirectories_;
}

void PluginManager::setUserPluginDirectory(const fs::path& directory) {
    userPluginDirectory_ = directory;
    if (fs::exists(directory) && fs::is_directory(directory)) {
        addPluginDirectory(directory);
    }
}

void PluginManager::setProjectPluginDirectory(const fs::path& directory) {
    projectPluginDirectory_ = directory;
    if (fs::exists(directory) && fs::is_directory(directory)) {
        addPluginDirectory(directory);
    }
}

const PluginManager::PluginInfo* PluginManager::getPluginInfo(const std::string& pluginName) const {
    auto it = loadedPlugins_.find(pluginName);
    if (it != loadedPlugins_.end()) {
        return &it->second->info;
    }
    return nullptr;
}

std::vector<PluginManager::PluginInfo> PluginManager::getAllPluginInfo() const {
    std::vector<PluginInfo> infos;
    for (const auto& pair : loadedPlugins_) {
        infos.push_back(pair.second->info);
    }
    return infos;
}

const std::vector<PluginManager::PluginError>& PluginManager::getPluginErrors() const {
    return pluginErrors_;
}

void PluginManager::clearPluginErrors() {
    pluginErrors_.clear();
}

void PluginManager::enableSandboxing(bool enabled) {
    sandboxingEnabled_ = enabled;
    std::cout << "PluginManager: Sandboxing " << (enabled ? "enabled" : "disabled") << std::endl;
}

bool PluginManager::isSandboxingEnabled() const {
    return sandboxingEnabled_;
}

int PluginManager::loadPluginLibrary(const fs::path& pluginPath, LoadedPlugin& plugin) {
    plugin.libraryHandle = dlopen(pluginPath.c_str(), RTLD_LAZY);
    if (!plugin.libraryHandle) {
        std::cerr << "PluginManager: Failed to load library " << pluginPath.string() << ": " << dlerror() << std::endl;
        return RAGGER_ERROR_PLUGIN_LOAD_FAILED;
    }

    // Load required functions
    auto get_abi_version = reinterpret_cast<decltype(plugin_get_abi_version)*>(
        dlsym(plugin.libraryHandle, "plugin_get_abi_version"));
    auto get_name = reinterpret_cast<decltype(plugin_get_name)*>(
        dlsym(plugin.libraryHandle, "plugin_get_name"));
    auto get_version = reinterpret_cast<decltype(plugin_get_version)*>(
        dlsym(plugin.libraryHandle, "plugin_get_version"));
    auto get_description = reinterpret_cast<decltype(plugin_get_description)*>(
        dlsym(plugin.libraryHandle, "plugin_get_description"));

    if (!get_abi_version || !get_name || !get_version || !get_description) {
        std::cerr << "PluginManager: Missing required functions in plugin " << pluginPath.string() << std::endl;
        dlclose(plugin.libraryHandle);
        plugin.libraryHandle = nullptr;
        return RAGGER_ERROR_PLUGIN_LOAD_FAILED;
    }

    // Get plugin metadata
    plugin.info.abiVersion = get_abi_version();
    plugin.info.name = get_name();
    plugin.info.version = get_version();
    plugin.info.description = get_description();

    return RAGGER_SUCCESS;
}

int PluginManager::initializePlugin(LoadedPlugin& plugin) {
    // Load plugin-specific functions
    auto initialize = reinterpret_cast<decltype(plugin_initialize)*>(
        dlsym(plugin.libraryHandle, "plugin_initialize"));
    auto register_events = reinterpret_cast<decltype(plugin_register_events)*>(
        dlsym(plugin.libraryHandle, "plugin_register_events"));
    auto get_capabilities = reinterpret_cast<decltype(plugin_get_capabilities)*>(
        dlsym(plugin.libraryHandle, "plugin_get_capabilities"));

    if (!initialize) {
        std::cerr << "PluginManager: Missing plugin_initialize function in plugin " << plugin.info.name << std::endl;
        return RAGGER_ERROR_PLUGIN_LOAD_FAILED;
    }

    // Initialize plugin
    int result = initialize(coreAPI_);
    if (result != RAGGER_SUCCESS) {
        std::cerr << "PluginManager: Plugin initialization failed for " << plugin.info.name << std::endl;
        return result;
    }

    // Register for events if supported
    if (register_events) {
        register_events(nullptr, nullptr); // Plugin handles event registration internally
    }

    // Get capabilities
    if (get_capabilities) {
        plugin.info.capabilities = get_capabilities();
    }

    // Load plugin interface (this would be implemented by specific plugin types)
    // For now, we'll assume it's set during initialization

    return RAGGER_SUCCESS;
}

void PluginManager::unloadPluginLibrary(LoadedPlugin& plugin) {
    if (plugin.libraryHandle) {
        // Call shutdown if available
        auto shutdown = reinterpret_cast<decltype(plugin_shutdown)*>(
            dlsym(plugin.libraryHandle, "plugin_shutdown"));
        if (shutdown) {
            try {
                shutdown();
            } catch (const std::exception& e) {
                std::cerr << "PluginManager: Exception during plugin shutdown: " << e.what() << std::endl;
            }
        }

        dlclose(plugin.libraryHandle);
        plugin.libraryHandle = nullptr;
    }
}

std::vector<fs::path> PluginManager::discoverPlugins(const fs::path& directory) const {
    std::vector<fs::path> plugins;

    try {
        for (const auto& entry : fs::directory_iterator(directory)) {
            if (isValidPluginFile(entry.path())) {
                plugins.push_back(entry.path());
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "PluginManager: Error discovering plugins in " << directory.string() << ": " << e.what() << std::endl;
    }

    return plugins;
}

bool PluginManager::isValidPluginFile(const fs::path& path) const {
    if (!fs::is_regular_file(path)) return false;

    std::string extension = path.extension().string();
#ifdef _WIN32
    return extension == ".dll";
#else
    return extension == ".so";
#endif
}

std::string PluginManager::extractPluginNameFromPath(const fs::path& path) const {
    return path.stem().string(); // Remove extension
}

int PluginManager::checkPluginABI(const LoadedPlugin& plugin) const {
    if (plugin.info.abiVersion != RAGGER_PLUGIN_ABI_VERSION) {
        std::cerr << "PluginManager: ABI version mismatch for plugin " << plugin.info.name 
                  << ": expected " << RAGGER_PLUGIN_ABI_VERSION << ", got " << plugin.info.abiVersion << std::endl;
        return RAGGER_ERROR_ABI_VERSION_MISMATCH;
    }
    return RAGGER_SUCCESS;
}

void PluginManager::reportPluginError(const std::string& pluginName, const std::string& message, int errorCode) {
    PluginError error;
    error.pluginName = pluginName;
    error.errorMessage = message;
    error.errorCode = errorCode;
    error.timestamp = std::chrono::system_clock::now();

    pluginErrors_.push_back(error);
    std::cerr << "PluginManager: Plugin error - " << pluginName << ": " << message << std::endl;
}

} // namespace Ragger
