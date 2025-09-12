#include "EventBus.h"
#include "ConfigManager.h"
#include "FileUtils.h"
#include "ragger_plugin_api.h"
#include <gtest/gtest.h>
#include <string>

namespace Ragger {

// Simple tests that don't require complex callbacks
TEST(SimpleTest, ConfigManagerBasic) {
    ConfigManager config;
    
    // Test setting and getting a string value
    config.setString("test.key", "test_value");
    
    const char* value = nullptr;
    int result = config.getString("test.key", &value);
    EXPECT_EQ(result, RAGGER_SUCCESS);
    EXPECT_STREQ(value, "test_value");
}

TEST(SimpleTest, ConfigManagerInt) {
    ConfigManager config;
    
    // Test setting and getting an integer value
    config.setInt("test.number", 42);
    
    int value = 0;
    int result = config.getInt("test.number", &value);
    EXPECT_EQ(result, RAGGER_SUCCESS);
    EXPECT_EQ(value, 42);
}

TEST(SimpleTest, ConfigManagerFloat) {
    ConfigManager config;
    
    // Test setting and getting a float value
    config.setFloat("test.float", 3.14f);
    
    float value = 0.0f;
    int result = config.getFloat("test.float", &value);
    EXPECT_EQ(result, RAGGER_SUCCESS);
    EXPECT_FLOAT_EQ(value, 3.14f);
}

TEST(SimpleTest, FileUtilsLanguageDetection) {
    // Test language detection from file paths
    EXPECT_EQ(FileUtils::detectLanguageFromPath("test.cpp"), "cpp");
    EXPECT_EQ(FileUtils::detectLanguageFromPath("test.c"), "c");
    EXPECT_EQ(FileUtils::detectLanguageFromPath("test.py"), "python");
    EXPECT_EQ(FileUtils::detectLanguageFromPath("test.js"), "javascript");
    EXPECT_EQ(FileUtils::detectLanguageFromPath("test.java"), "java");
    EXPECT_EQ(FileUtils::detectLanguageFromPath("test.rs"), "rust");
    EXPECT_EQ(FileUtils::detectLanguageFromPath("test.unknown"), "unknown");
}

TEST(SimpleTest, FileUtilsExtension) {
    // Test file extension extraction
    EXPECT_EQ(FileUtils::getFileExtension("test.cpp"), ".cpp");
    EXPECT_EQ(FileUtils::getFileExtension("test.h"), ".h");
    EXPECT_EQ(FileUtils::getFileExtension("test"), "");
    EXPECT_EQ(FileUtils::getFileExtension("test.tar.gz"), ".gz");
}

TEST(SimpleTest, EventBusCreation) {
    // Test that EventBus can be created and initialized
    EventBus eventBus;
    
    // Basic test - just ensure it doesn't crash
    EXPECT_TRUE(true);
}

TEST(SimpleTest, RaggerConstants) {
    // Test that the API constants are defined
    EXPECT_GT(RAGGER_PLUGIN_ABI_VERSION, 0);
    EXPECT_EQ(RAGGER_SUCCESS, 0);
    EXPECT_NE(RAGGER_ERROR_INVALID_ARGUMENT, RAGGER_SUCCESS);
}

} // namespace Ragger
