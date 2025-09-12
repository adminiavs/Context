#pragma once

#include <string>
#include <filesystem>

namespace fs = std::filesystem;

namespace Ragger {

class FileUtils {
public:
    // File operations
    static int readFile(const fs::path& path, char** content, size_t* size);
    static int writeFile(const fs::path& path, const char* content, size_t size);
    static int getFileHash(const fs::path& path, char* hash, size_t hashSize);

    // Path utilities
    static fs::path expandUserPath(const std::string& path);
    static bool ensureDirectoryExists(const fs::path& path);
    static std::string getFileExtension(const fs::path& path);
    static std::string getFileNameWithoutExtension(const fs::path& path);

    // Content analysis
    static std::string detectLanguageFromContent(const std::string& content);
    static std::string detectLanguageFromPath(const fs::path& path);
    static bool isTextFile(const fs::path& path);

private:
    FileUtils() = delete; // Static utility class
};

} // namespace Ragger
