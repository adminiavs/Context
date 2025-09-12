#pragma once

#include <string>
#include <unordered_map>
#include <variant>
#include <filesystem>
#include <vector>
// JSON support disabled for now
// #include <nlohmann/json.hpp>

namespace fs = std::filesystem;

namespace Ragger {

class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager() = default;

    // Configuration loading
    int loadFromFile(const fs::path& configFile);
    int loadDefaults();
    int saveToFile(const fs::path& configFile) const;

    // Value retrieval (RaggerCoreAPI interface)
    int getString(const std::string& key, const char** value) const;
    int getInt(const std::string& key, int* value) const;
    int getFloat(const std::string& key, float* value) const;

    // Value setting
    void setString(const std::string& key, const std::string& value);
    void setInt(const std::string& key, int value);
    void setFloat(const std::string& key, float value);
    void setBool(const std::string& key, bool value);

    // Advanced operations
    bool hasKey(const std::string& key) const;
    void removeKey(const std::string& key);
    std::vector<std::string> getKeys() const;
    std::vector<std::string> getKeysWithPrefix(const std::string& prefix) const;

    // Configuration sections
    void setSection(const std::string& section);
    std::string getSection() const;

    // Validation
    bool validateConfig() const;
    std::vector<std::string> getValidationErrors() const;

    // Environment variable integration
    void enableEnvOverride(bool enabled);
    bool isEnvOverrideEnabled() const;

private:
    using ConfigValue = std::variant<std::string, int, float, bool>;

    std::unordered_map<std::string, ConfigValue> config_;
    std::string currentSection_;
    bool envOverrideEnabled_;

    // Key formatting
    std::string formatKey(const std::string& key) const;
    std::string makeKey(const std::string& section, const std::string& key) const;

    // Default configuration
    void setDefaultValues();

    // JSON serialization helpers (disabled for now)
    // nlohmann::json toJson() const;
    // void fromJson(const nlohmann::json& j);

    // Environment variable handling
    std::string getEnvOverride(const std::string& key) const;

    // Validation helpers
    bool validateRequiredKeys() const;
    bool validateValueRanges() const;
    bool validatePaths() const;
};

} // namespace Ragger
