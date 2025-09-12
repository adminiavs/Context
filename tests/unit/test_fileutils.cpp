#include "FileUtils.h"
#include "ragger_plugin_api.h"
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <string>

namespace Ragger {

class FileUtilsTest : public ::testing::Test {
protected:
    void SetUp() override {
        testDir = "test_files";
        testFile = testDir + "/test.txt";
        testContent = "Hello, RAGger!";
        
        // Create test directory
        std::filesystem::create_directories(testDir);
        
        // Create test file
        std::ofstream file(testFile);
        file << testContent;
        file.close();
    }

    void TearDown() override {
        // Clean up test files
        if (std::filesystem::exists(testDir)) {
            std::filesystem::remove_all(testDir);
        }
    }

    std::string testDir;
    std::string testFile;
    std::string testContent;
};

TEST_F(FileUtilsTest, ReadFile) {
    char* content = nullptr;
    size_t size = 0;
    
    int result = FileUtils::readFile(testFile, &content, &size);
    EXPECT_EQ(result, RAGGER_SUCCESS);
    EXPECT_NE(content, nullptr);
    EXPECT_EQ(size, testContent.length());
    
    std::string readContent(content, size);
    EXPECT_EQ(readContent, testContent);
    
    // Clean up
    delete[] content;
}

TEST_F(FileUtilsTest, WriteFile) {
    std::string newContent = "New content for testing";
    std::string newFile = testDir + "/new_file.txt";
    
    int result = FileUtils::writeFile(newFile, newContent.c_str(), newContent.length());
    EXPECT_EQ(result, RAGGER_SUCCESS);
    
    // Verify file was created and contains correct content
    EXPECT_TRUE(std::filesystem::exists(newFile));
    
    std::ifstream file(newFile);
    std::string fileContent((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
    EXPECT_EQ(fileContent, newContent);
}

TEST_F(FileUtilsTest, FileHash) {
    char hash[65]; // SHA256 hex string length
    
    int result = FileUtils::getFileHash(testFile, hash, sizeof(hash));
    EXPECT_EQ(result, RAGGER_SUCCESS);
    
    // Verify hash is not empty and has correct length
    EXPECT_NE(hash[0], '\0');
    EXPECT_EQ(strlen(hash), 64); // SHA256 hex string length
    
    // Test that same file produces same hash
    char hash2[65];
    result = FileUtils::getFileHash(testFile, hash2, sizeof(hash2));
    EXPECT_EQ(result, RAGGER_SUCCESS);
    EXPECT_STREQ(hash, hash2);
}

TEST_F(FileUtilsTest, FileExtension) {
    std::string extension = FileUtils::getFileExtension(testFile);
    EXPECT_EQ(extension, ".txt");
    
    // Test with different extensions
    EXPECT_EQ(FileUtils::getFileExtension("test.cpp"), ".cpp");
    EXPECT_EQ(FileUtils::getFileExtension("test.h"), ".h");
    EXPECT_EQ(FileUtils::getFileExtension("test"), "");
    EXPECT_EQ(FileUtils::getFileExtension("test.tar.gz"), ".gz");
}

TEST_F(FileUtilsTest, LanguageDetection) {
    // Test language detection from file path
    EXPECT_EQ(FileUtils::detectLanguageFromPath("test.cpp"), "cpp");
    EXPECT_EQ(FileUtils::detectLanguageFromPath("test.c"), "c");
    EXPECT_EQ(FileUtils::detectLanguageFromPath("test.py"), "python");
    EXPECT_EQ(FileUtils::detectLanguageFromPath("test.js"), "javascript");
    EXPECT_EQ(FileUtils::detectLanguageFromPath("test.java"), "java");
    EXPECT_EQ(FileUtils::getFileExtension("test.go"), ".go");
    EXPECT_EQ(FileUtils::detectLanguageFromPath("test.rs"), "rust");
    EXPECT_EQ(FileUtils::detectLanguageFromPath("test.json"), "json");
    EXPECT_EQ(FileUtils::detectLanguageFromPath("test.yaml"), "yaml");
    EXPECT_EQ(FileUtils::detectLanguageFromPath("test.unknown"), "");
}

TEST_F(FileUtilsTest, DirectoryOperations) {
    std::string newDir = testDir + "/subdir";
    
    // Test directory creation
    bool result = FileUtils::ensureDirectoryExists(newDir);
    EXPECT_TRUE(result);
    EXPECT_TRUE(std::filesystem::exists(newDir));
    
    // Test creating existing directory (should succeed)
    result = FileUtils::ensureDirectoryExists(newDir);
    EXPECT_TRUE(result);
}

TEST_F(FileUtilsTest, PathExpansion) {
    // Test user path expansion
    std::string expandedPath = FileUtils::expandUserPath("~/test");
    EXPECT_FALSE(expandedPath.empty());
    EXPECT_NE(expandedPath, "~/test"); // Should be expanded
    
    // Test relative path (should remain unchanged)
    std::string relativePath = FileUtils::expandUserPath("./test");
    EXPECT_EQ(relativePath, "./test");
    
    // Test absolute path (should remain unchanged)
    std::string absolutePath = FileUtils::expandUserPath("/absolute/path");
    EXPECT_EQ(absolutePath, "/absolute/path");
}

TEST_F(FileUtilsTest, InvalidOperations) {
    // Test reading non-existent file
    char* content = nullptr;
    size_t size = 0;
    int result = FileUtils::readFile("non_existent_file.txt", &content, &size);
    EXPECT_EQ(result, RAGGER_ERROR_FILE_NOT_FOUND);
    
    // Test writing to invalid path
    result = FileUtils::writeFile("/invalid/path/file.txt", "content", 7);
    EXPECT_EQ(result, RAGGER_ERROR_OPERATION_NOT_SUPPORTED);
    
    // Test hashing non-existent file
    char hash[65];
    result = FileUtils::getFileHash("non_existent_file.txt", hash, sizeof(hash));
    EXPECT_EQ(result, RAGGER_ERROR_OPERATION_NOT_SUPPORTED);
}

TEST_F(FileUtilsTest, LargeFileHandling) {
    // Create a larger test file
    std::string largeFile = testDir + "/large_file.txt";
    std::string largeContent(10000, 'A'); // 10KB of 'A's
    
    std::ofstream file(largeFile);
    file << largeContent;
    file.close();
    
    // Test reading large file
    char* content = nullptr;
    size_t size = 0;
    int result = FileUtils::readFile(largeFile, &content, &size);
    EXPECT_EQ(result, RAGGER_SUCCESS);
    EXPECT_EQ(size, largeContent.length());
    
    // Test hashing large file
    char hash[65];
    result = FileUtils::getFileHash(largeFile, hash, sizeof(hash));
    EXPECT_EQ(result, RAGGER_SUCCESS);
    EXPECT_NE(hash[0], '\0');
    
    // Clean up
    delete[] content;
}

TEST_F(FileUtilsTest, FileSizeLimits) {
    // Test with very small file
    std::string smallFile = testDir + "/small.txt";
    std::ofstream file(smallFile);
    file << "X";
    file.close();
    
    char* content = nullptr;
    size_t size = 0;
    int result = FileUtils::readFile(smallFile, &content, &size);
    EXPECT_EQ(result, RAGGER_SUCCESS);
    EXPECT_EQ(size, 1);
    
    delete[] content;
}

} // namespace Ragger
