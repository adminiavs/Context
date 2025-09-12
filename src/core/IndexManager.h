#pragma once

#include "ragger_plugin_api.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
// SQLite support enabled
#include <sqlite3.h>

namespace fs = std::filesystem;

namespace Ragger {

class IndexManager {
public:
    IndexManager();
    ~IndexManager();

    // Initialization
    int initialize();
    void shutdown();

    // File indexing
    int indexFile(const fs::path& filePath);
    int indexDirectory(const fs::path& directory);
    int removeFromIndex(const fs::path& filePath);

    // Index queries
    int findFilesByContent(const std::string& query, std::vector<fs::path>& results);
    int findCodeBlocks(const std::string& symbol, std::vector<CodeBlock>& results);
    int getFileSymbols(const fs::path& filePath, std::vector<CodeSymbol>& symbols);

    // Index maintenance
    int rebuildIndex();
    int optimizeIndex();
    int getIndexStats(size_t& fileCount, size_t& blockCount, size_t& symbolCount);

    // Incremental indexing
    bool needsReindexing(const fs::path& filePath);
    int updateFileIndex(const fs::path& filePath);

    // Configuration
    void setMaxFileSize(size_t maxSize);
    void setDebounceDelay(int milliseconds);
    void setDatabasePath(const fs::path& dbPath);

private:
    sqlite3* db_;
    fs::path dbPath_;
    size_t maxFileSize_;
    int debounceDelay_;
    std::unordered_map<std::string, std::string> fileHashes_; // file path -> hash

    // Database operations
    int createTables();
    int insertFileRecord(const fs::path& filePath, const std::string& hash);
    int insertCodeBlock(const CodeBlock* block, const fs::path& filePath);
    int updateFileHash(const fs::path& filePath, const std::string& hash);
    int deleteFileRecords(const fs::path& filePath);

    // File processing
    std::string calculateFileHash(const fs::path& filePath);
    bool shouldIndexFile(const fs::path& filePath);
    std::vector<fs::path> discoverFiles(const fs::path& directory);

    // SQLite helper functions
    int executeQuery(const std::string& query);
    int executeQueryWithCallback(const std::string& query,
                                int (*callback)(void*, int, char**, char**),
                                void* callbackData);

    // Prevent copying
    IndexManager(const IndexManager&) = delete;
    IndexManager& operator=(const IndexManager&) = delete;
};

} // namespace Ragger
