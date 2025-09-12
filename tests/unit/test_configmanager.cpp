#include "ConfigManager.h"
#include "ragger_plugin_api.h"
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>

namespace Ragger {

class ConfigManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        configManager = std::make_unique<ConfigManager>();
        testConfigFile = "test_config.conf";
    }

    void TearDown() override {
        configManager.reset();
        // Clean up test file
        if (std::filesystem::exists(testConfigFile)) {
            std::filesystem::remove(testConfigFile);
        }
    }

    std::unique_ptr<ConfigManager> configManager;
    std::string testConfigFile;
};

TEST_F(ConfigManagerTest, BasicConfiguration) {
    // Test setting and getting string values
    configManager->setString("test.key", "test_value");
    
    const char* value = nullptr;
    int result = configManager->getString("test.key", &value);
    EXPECT_EQ(result, RAGGER_SUCCESS);
    EXPECT_STREQ(value, "test_value");
}

TEST_F(ConfigManagerTest, IntegerConfiguration) {
    // Test setting and getting integer values
    configManager->setInt("test.number", 42);
    
    int value = 0;
    int result = configManager->getInt("test.number", &value);
    EXPECT_EQ(result, RAGGER_SUCCESS);
    EXPECT_EQ(value, 42);
}

TEST_F(ConfigManagerTest, FloatConfiguration) {
    // Test setting and getting float values
    configManager->setFloat("test.float", 3.14f);
    
    float value = 0.0f;
    int result = configManager->getFloat("test.float", &value);
    EXPECT_EQ(result, RAGGER_SUCCESS);
    EXPECT_FLOAT_EQ(value, 3.14f);
}

TEST_F(ConfigManagerTest, ConfigurationFileOperations) {
    // Create a test configuration file
    std::ofstream configFile(testConfigFile);
    configFile << "test.key=test_value\n";
    configFile << "test.number=42\n";
    configFile << "test.float=3.14\n";
    configFile.close();

    // Load configuration from file
    int result = configManager->loadFromFile(testConfigFile);
    EXPECT_EQ(result, RAGGER_SUCCESS);

    // Verify loaded values
    const char* stringValue = nullptr;
    result = configManager->getString("test.key", &stringValue);
    EXPECT_EQ(result, RAGGER_SUCCESS);
    EXPECT_STREQ(stringValue, "test_value");

    int intValue = 0;
    result = configManager->getInt("test.number", &intValue);
    EXPECT_EQ(result, RAGGER_SUCCESS);
    EXPECT_EQ(intValue, 42);

    float floatValue = 0.0f;
    result = configManager->getFloat("test.float", &floatValue);
    EXPECT_EQ(result, RAGGER_SUCCESS);
    EXPECT_FLOAT_EQ(floatValue, 3.14f);
}

TEST_F(ConfigManagerTest, SaveConfiguration) {
    // Set some values
    configManager->setString("test.key", "test_value");
    configManager->setInt("test.number", 42);
    configManager->setFloat("test.float", 3.14f);

    // Save to file
    int result = configManager->saveToFile(testConfigFile);
    EXPECT_EQ(result, RAGGER_SUCCESS);

    // Verify file exists
    EXPECT_TRUE(std::filesystem::exists(testConfigFile));
}

TEST_F(ConfigManagerTest, DefaultValues) {
    // Load default configuration
    int result = configManager->loadDefaults();
    EXPECT_EQ(result, RAGGER_SUCCESS);

    // Verify some default values exist
    const char* value = nullptr;
    result = configManager->getString("indexing.database_path", &value);
    EXPECT_EQ(result, RAGGER_SUCCESS);
    EXPECT_NE(value, nullptr);
}

TEST_F(ConfigManagerTest, EnvironmentVariableOverride) {
    // Enable environment variable override
    configManager->enableEnvOverride(true);

    // Set environment variable
    setenv("RAGGER_TEST_KEY", "env_value", 1);

    // Set a value that can be overridden
    configManager->setString("test.key", "file_value");

    // The environment variable should override the file value
    const char* value = nullptr;
    int result = configManager->getString("test.key", &value);
    EXPECT_EQ(result, RAGGER_SUCCESS);
    EXPECT_STREQ(value, "env_value");

    // Clean up
    unsetenv("RAGGER_TEST_KEY");
}

TEST_F(ConfigManagerTest, ConfigurationValidation) {
    // Load default configuration
    configManager->loadDefaults();

    // Validate configuration
    bool isValid = configManager->validateConfig();
    EXPECT_TRUE(isValid);

    // Get validation errors (should be empty for valid config)
    auto errors = configManager->getValidationErrors();
    EXPECT_TRUE(errors.empty());
}

TEST_F(ConfigManagerTest, InvalidOperations) {
    // Test getting non-existent key
    const char* value = nullptr;
    int result = configManager->getString("non.existent.key", &value);
    EXPECT_EQ(result, RAGGER_ERROR_INVALID_ARGUMENT);

    // Test loading non-existent file
    result = configManager->loadFromFile("non_existent_file.conf");
    EXPECT_EQ(result, RAGGER_ERROR_FILE_NOT_FOUND);
}

TEST_F(ConfigManagerTest, KeyPrefixOperations) {
    // Set multiple keys with same prefix
    configManager->setString("database.host", "localhost");
    configManager->setString("database.port", "5432");
    configManager->setString("database.name", "ragger");
    configManager->setString("other.key", "value");

    // Get keys with prefix
    auto keys = configManager->getKeysWithPrefix("database.");
    EXPECT_EQ(keys.size(), 3);
    
    // Verify all database keys are present
    EXPECT_TRUE(std::find(keys.begin(), keys.end(), "database.host") != keys.end());
    EXPECT_TRUE(std::find(keys.begin(), keys.end(), "database.port") != keys.end());
    EXPECT_TRUE(std::find(keys.begin(), keys.end(), "database.name") != keys.end());
    EXPECT_TRUE(std::find(keys.begin(), keys.end(), "other.key") == keys.end());
}

} // namespace Ragger
