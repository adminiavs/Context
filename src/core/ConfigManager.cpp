#include "ConfigManager.h"
#include "ragger_plugin_api.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
// spdlog disabled

namespace Ragger {

ConfigManager::ConfigManager() : envOverrideEnabled_(true) {
    setDefaultValues();
}

int ConfigManager::loadFromFile(const fs::path& configFile) {
    try {
        if (!fs::exists(configFile)) {
            std::cout << "ConfigManager: Configuration file does not exist: " << configFile.string() << std::endl;
            return RAGGER_ERROR_FILE_NOT_FOUND;
        }

        // Simple key-value parsing for now (no JSON)
        std::ifstream file(configFile);
        if (!file.is_open()) {
            std::cerr << "ConfigManager: Failed to open configuration file: " << configFile.string() << std::endl;
            return RAGGER_ERROR_FILE_NOT_FOUND;
        }

        std::string line;
        while (std::getline(file, line)) {
            // Skip comments and empty lines
            if (line.empty() || line[0] == '#') continue;

            // Simple key=value parsing
            size_t equalsPos = line.find('=');
            if (equalsPos != std::string::npos) {
                std::string key = line.substr(0, equalsPos);
                std::string value = line.substr(equalsPos + 1);

                // Trim whitespace
                key.erase(key.begin(), std::find_if(key.begin(), key.end(), [](int ch) { return !std::isspace(ch); }));
                key.erase(std::find_if(key.rbegin(), key.rend(), [](int ch) { return !std::isspace(ch); }).base(), key.end());
                value.erase(value.begin(), std::find_if(value.begin(), value.end(), [](int ch) { return !std::isspace(ch); }));
                value.erase(std::find_if(value.rbegin(), value.rend(), [](int ch) { return !std::isspace(ch); }).base(), value.end());

                // Store as string for now
                config_[key] = value;
            }
        }

        std::cout << "ConfigManager: Loaded configuration from " << configFile.string() << std::endl;
        return RAGGER_SUCCESS;

    } catch (const std::exception& e) {
        std::cerr << "ConfigManager: Failed to parse configuration file: " << e.what() << std::endl;
        return RAGGER_ERROR_PARSING_FAILED;
    }
}

int ConfigManager::loadDefaults() {
    setDefaultValues();
    std::cout << "ConfigManager: Loaded default configuration" << std::endl;
    return RAGGER_SUCCESS;
}

int ConfigManager::saveToFile(const fs::path& configFile) const {
    try {
        // Create directory if it doesn't exist
        fs::create_directories(configFile.parent_path());

        std::ofstream file(configFile);
        if (!file.is_open()) {
            std::cerr << "ConfigManager: Failed to open file for writing: " << configFile.string() << std::endl;
            return RAGGER_ERROR_FILE_NOT_FOUND;
        }

        // Simple key=value format
        for (const auto& pair : config_) {
            if (std::holds_alternative<std::string>(pair.second)) {
                file << pair.first << "=" << std::get<std::string>(pair.second) << std::endl;
            } else if (std::holds_alternative<int>(pair.second)) {
                file << pair.first << "=" << std::get<int>(pair.second) << std::endl;
            } else if (std::holds_alternative<float>(pair.second)) {
                file << pair.first << "=" << std::get<float>(pair.second) << std::endl;
            } else if (std::holds_alternative<bool>(pair.second)) {
                file << pair.first << "=" << (std::get<bool>(pair.second) ? "true" : "false") << std::endl;
            }
        }

        std::cout << "ConfigManager: Saved configuration to " << configFile.string() << std::endl;
        return RAGGER_SUCCESS;

    } catch (const std::exception& e) {
        std::cerr << "ConfigManager: Failed to save configuration: " << e.what() << std::endl;
        return RAGGER_ERROR_OPERATION_NOT_SUPPORTED;
    }
}

int ConfigManager::getString(const std::string& key, const char** value) const {
    if (envOverrideEnabled_) {
        std::string envValue = getEnvOverride(key);
        if (!envValue.empty()) {
            *value = envValue.c_str();
            return RAGGER_SUCCESS;
        }
    }

    std::string fullKey = formatKey(key);
    auto it = config_.find(fullKey);
    if (it != config_.end() && std::holds_alternative<std::string>(it->second)) {
        *value = std::get<std::string>(it->second).c_str();
        return RAGGER_SUCCESS;
    }

    return RAGGER_ERROR_INVALID_ARGUMENT;
}

int ConfigManager::getInt(const std::string& key, int* value) const {
    if (envOverrideEnabled_) {
        std::string envValue = getEnvOverride(key);
        if (!envValue.empty()) {
            try {
                *value = std::stoi(envValue);
                return RAGGER_SUCCESS;
            } catch (...) {
                // Invalid env value, fall through to config
            }
        }
    }

    std::string fullKey = formatKey(key);
    auto it = config_.find(fullKey);
    if (it != config_.end()) {
        if (std::holds_alternative<int>(it->second)) {
            *value = std::get<int>(it->second);
            return RAGGER_SUCCESS;
        } else if (std::holds_alternative<std::string>(it->second)) {
            // Try to convert string to int
            try {
                *value = std::stoi(std::get<std::string>(it->second));
                return RAGGER_SUCCESS;
            } catch (...) {
                // Conversion failed
            }
        }
    }

    return RAGGER_ERROR_INVALID_ARGUMENT;
}

int ConfigManager::getFloat(const std::string& key, float* value) const {
    if (envOverrideEnabled_) {
        std::string envValue = getEnvOverride(key);
        if (!envValue.empty()) {
            try {
                *value = std::stof(envValue);
                return RAGGER_SUCCESS;
            } catch (...) {
                // Invalid env value, fall through to config
            }
        }
    }

    std::string fullKey = formatKey(key);
    auto it = config_.find(fullKey);
    if (it != config_.end()) {
        if (std::holds_alternative<float>(it->second)) {
            *value = std::get<float>(it->second);
            return RAGGER_SUCCESS;
        } else if (std::holds_alternative<std::string>(it->second)) {
            // Try to convert string to float
            try {
                *value = std::stof(std::get<std::string>(it->second));
                return RAGGER_SUCCESS;
            } catch (...) {
                // Conversion failed
            }
        }
    }

    return RAGGER_ERROR_INVALID_ARGUMENT;
}

void ConfigManager::setString(const std::string& key, const std::string& value) {
    config_[formatKey(key)] = value;
}

void ConfigManager::setInt(const std::string& key, int value) {
    config_[formatKey(key)] = value;
}

void ConfigManager::setFloat(const std::string& key, float value) {
    config_[formatKey(key)] = value;
}

void ConfigManager::setBool(const std::string& key, bool value) {
    config_[formatKey(key)] = value;
}

bool ConfigManager::hasKey(const std::string& key) const {
    return config_.find(formatKey(key)) != config_.end();
}

void ConfigManager::removeKey(const std::string& key) {
    config_.erase(formatKey(key));
}

std::vector<std::string> ConfigManager::getKeys() const {
    std::vector<std::string> keys;
    for (const auto& pair : config_) {
        keys.push_back(pair.first);
    }
    return keys;
}

std::vector<std::string> ConfigManager::getKeysWithPrefix(const std::string& prefix) const {
    std::vector<std::string> keys;
    for (const auto& pair : config_) {
        if (pair.first.find(prefix) == 0) {
            keys.push_back(pair.first);
        }
    }
    return keys;
}

void ConfigManager::setSection(const std::string& section) {
    currentSection_ = section;
}

std::string ConfigManager::getSection() const {
    return currentSection_;
}

bool ConfigManager::validateConfig() const {
    return validateRequiredKeys() && validateValueRanges() && validatePaths();
}

std::vector<std::string> ConfigManager::getValidationErrors() const {
    std::vector<std::string> errors;

    if (!validateRequiredKeys()) {
        errors.push_back("Missing required configuration keys");
    }

    if (!validateValueRanges()) {
        errors.push_back("Configuration values out of valid range");
    }

    if (!validatePaths()) {
        errors.push_back("Invalid paths in configuration");
    }

    return errors;
}

void ConfigManager::enableEnvOverride(bool enabled) {
    envOverrideEnabled_ = enabled;
}

bool ConfigManager::isEnvOverrideEnabled() const {
    return envOverrideEnabled_;
}

std::string ConfigManager::formatKey(const std::string& key) const {
    return makeKey(currentSection_, key);
}

std::string ConfigManager::makeKey(const std::string& section, const std::string& key) const {
    if (section.empty()) {
        return key;
    }
    return section + "." + key;
}

void ConfigManager::setDefaultValues() {
    // Core settings
    setString("app.name", "RAGger");
    setString("app.version", "0.1.0");
    setString("app.log_level", "info");

    // Indexing settings
    setInt("indexing.max_file_size", 10485760); // 10MB
    setBool("indexing.enable_incremental", true);
    setInt("indexing.debounce_ms", 500);
    setString("indexing.database_path", "data/index.db");

    // Plugin settings
    setBool("plugins.enable_sandbox", false);
    setString("plugins.user_dir", "~/.ragger/plugins");
    setString("plugins.project_dir", ".ragger/plugins");

    // UI settings
    setString("ui.backend", "imgui"); // or "qt"
    setInt("ui.max_context_tokens", 8000);
    setBool("ui.show_token_count", true);

    // LLM settings
    setFloat("llm.temperature", 0.7f);
    setInt("llm.max_tokens", 2048);
    setString("llm.default_model", "gpt-3.5-turbo");

    // Performance settings
    setInt("performance.thread_pool_size", 4);
    setInt("performance.memory_limit_mb", 512);
    setBool("performance.enable_caching", true);
}

// JSON methods disabled - using simple key=value format instead
// nlohmann::json ConfigManager::toJson() const { return nlohmann::json{}; }
// void ConfigManager::fromJson(const nlohmann::json& j) { }

std::string ConfigManager::getEnvOverride(const std::string& key) const {
    std::string envKey = "RAGGER_" + key;
    std::transform(envKey.begin(), envKey.end(), envKey.begin(), ::toupper);
    std::replace(envKey.begin(), envKey.end(), '.', '_');

    const char* envValue = std::getenv(envKey.c_str());
    return envValue ? std::string(envValue) : "";
}

bool ConfigManager::validateRequiredKeys() const {
    std::vector<std::string> requiredKeys = {
        "app.name",
        "indexing.database_path",
        "ui.max_context_tokens"
    };

    for (const auto& key : requiredKeys) {
        if (!hasKey(key)) {
            return false;
        }
    }

    return true;
}

bool ConfigManager::validateValueRanges() const {
    // Check thread pool size
    int threadPoolSize = 4;
    getInt("performance.thread_pool_size", &threadPoolSize);
    if (threadPoolSize < 1 || threadPoolSize > 64) {
        return false;
    }

    // Check memory limit
    int memoryLimit = 512;
    getInt("performance.memory_limit_mb", &memoryLimit);
    if (memoryLimit < 64 || memoryLimit > 8192) {
        return false;
    }

    // Check max tokens
    int maxTokens = 8000;
    getInt("ui.max_context_tokens", &maxTokens);
    if (maxTokens < 1000 || maxTokens > 32000) {
        return false;
    }

    return true;
}

bool ConfigManager::validatePaths() const {
    // Check if database path is valid (basic check)
    const char* dbPath = nullptr;
    if (getString("indexing.database_path", &dbPath) == RAGGER_SUCCESS) {
        fs::path path(dbPath);
        if (!path.parent_path().empty() && !fs::exists(path.parent_path())) {
            // Parent directory doesn't exist - this might be OK if we create it
            // but we'll warn about it
            std::cout << "ConfigManager: Database parent directory does not exist: " << path.parent_path().string() << std::endl;
        }
    }

    return true;
}

} // namespace Ragger
