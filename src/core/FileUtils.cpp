#include "FileUtils.h"
#include "ragger_plugin_api.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>
// spdlog disabled
#include <openssl/sha.h>

namespace Ragger {

int FileUtils::readFile(const fs::path& path, char** content, size_t* size) {
    try {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) {
            return RAGGER_ERROR_FILE_NOT_FOUND;
        }

        std::streamsize fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        *content = new char[fileSize + 1];
        if (!file.read(*content, fileSize)) {
            delete[] *content;
            return RAGGER_ERROR_OPERATION_NOT_SUPPORTED;
        }

        (*content)[fileSize] = '\0';
        *size = fileSize;

        return RAGGER_SUCCESS;

    } catch (const std::exception& e) {
        std::cerr << "FileUtils: Error reading file " << path.string() << ": " << e.what() << std::endl;
        return RAGGER_ERROR_OPERATION_NOT_SUPPORTED;
    }
}

int FileUtils::writeFile(const fs::path& path, const char* content, size_t size) {
    try {
        // Ensure directory exists
        ensureDirectoryExists(path.parent_path());

        std::ofstream file(path, std::ios::binary);
        if (!file) {
            return RAGGER_ERROR_FILE_NOT_FOUND;
        }

        file.write(content, size);
        return file.good() ? RAGGER_SUCCESS : RAGGER_ERROR_OPERATION_NOT_SUPPORTED;

    } catch (const std::exception& e) {
        std::cerr << "FileUtils: Error writing file " << path.string() << ": " << e.what() << std::endl;
        return RAGGER_ERROR_OPERATION_NOT_SUPPORTED;
    }
}

int FileUtils::getFileHash(const fs::path& path, char* hash, size_t hashSize) {
    try {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            return RAGGER_ERROR_FILE_NOT_FOUND;
        }

        SHA256_CTX sha256;
        SHA256_Init(&sha256);

        char buffer[8192];
        while (file.read(buffer, sizeof(buffer))) {
            SHA256_Update(&sha256, buffer, file.gcount());
        }
        SHA256_Update(&sha256, buffer, file.gcount());

        unsigned char hashBytes[SHA256_DIGEST_LENGTH];
        SHA256_Final(hashBytes, &sha256);

        if (hashSize < (SHA256_DIGEST_LENGTH * 2 + 1)) {
            return RAGGER_ERROR_INVALID_ARGUMENT;
        }

        // Convert to hex string
        for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
            sprintf(hash + (i * 2), "%02x", hashBytes[i]);
        }
        hash[SHA256_DIGEST_LENGTH * 2] = '\0';

        return RAGGER_SUCCESS;

    } catch (const std::exception& e) {
        std::cerr << "FileUtils: Error hashing file " << path.string() << ": " << e.what() << std::endl;
        return RAGGER_ERROR_OPERATION_NOT_SUPPORTED;
    }
}

fs::path FileUtils::expandUserPath(const std::string& path) {
    if (path.empty() || path[0] != '~') {
        return fs::path(path);
    }

    const char* home = std::getenv("HOME");
    if (!home) {
        return fs::path(path); // Return as-is if HOME not found
    }

    if (path.length() == 1) {
        return fs::path(home);
    }

    return fs::path(home) / path.substr(2);
}

bool FileUtils::ensureDirectoryExists(const fs::path& path) {
    try {
        return fs::create_directories(path);
    } catch (const std::exception& e) {
        std::cerr << "FileUtils: Error creating directory " << path.string() << ": " << e.what() << std::endl;
        return false;
    }
}

std::string FileUtils::getFileExtension(const fs::path& path) {
    return path.extension().string();
}

std::string FileUtils::getFileNameWithoutExtension(const fs::path& path) {
    return path.stem().string();
}

std::string FileUtils::detectLanguageFromContent(const std::string& content) {
    // Simple heuristic-based language detection
    if (content.find("#include") != std::string::npos ||
        content.find("std::") != std::string::npos) {
        return "cpp";
    }

    if (content.find("import ") != std::string::npos &&
        content.find("def ") != std::string::npos) {
        return "python";
    }

    if (content.find("function") != std::string::npos &&
        content.find("var ") != std::string::npos) {
        return "javascript";
    }

    if (content.find("public class") != std::string::npos) {
        return "java";
    }

    return "unknown";
}

std::string FileUtils::detectLanguageFromPath(const fs::path& path) {
    std::string ext = getFileExtension(path);

    static const std::unordered_map<std::string, std::string> extToLang = {
        {".cpp", "cpp"}, {".cxx", "cpp"}, {".cc", "cpp"}, {".c", "c"},
        {".hpp", "cpp"}, {".hxx", "cpp"}, {".h", "c"},
        {".py", "python"}, {".java", "java"},
        {".js", "javascript"}, {".ts", "typescript"},
        {".rs", "rust"}, {".go", "go"}
    };

    auto it = extToLang.find(ext);
    return it != extToLang.end() ? it->second : "unknown";
}

bool FileUtils::isTextFile(const fs::path& path) {
    try {
        // Check file size first
        if (fs::file_size(path) > 100 * 1024 * 1024) { // 100MB limit
            return false;
        }

        // Try to read first few bytes and check for binary content
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            return false;
        }

        char buffer[1024];
        file.read(buffer, sizeof(buffer));
        std::streamsize bytesRead = file.gcount();

        // Check for null bytes (common in binary files)
        for (std::streamsize i = 0; i < bytesRead; ++i) {
            if (buffer[i] == '\0') {
                return false;
            }
        }

        return true;

    } catch (const std::exception&) {
        return false;
    }
}

} // namespace Ragger
