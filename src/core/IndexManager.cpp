#include "IndexManager.h"
// spdlog disabled
#include <fstream>
#include <sstream>
#include <iostream>
#include <openssl/evp.h>
#include <openssl/sha.h> // For SHA256 hashing

namespace Ragger {

IndexManager::IndexManager()
    : db_(nullptr), maxFileSize_(10 * 1024 * 1024), debounceDelay_(500) {
    dbPath_ = "data/index.db";
}

IndexManager::~IndexManager() {
    shutdown();
}

int IndexManager::initialize() {
    // Create data directory if it doesn't exist
    fs::create_directories(dbPath_.parent_path());

    // Open SQLite database
    int result = sqlite3_open(dbPath_.string().c_str(), &db_);
    if (result != SQLITE_OK) {
        std::cerr << "IndexManager: Failed to open database: " << sqlite3_errmsg(db_) << std::endl;
        return RAGGER_ERROR_OPERATION_NOT_SUPPORTED;
    }
    
    // Initialize connection pool
    initializeConnectionPool();

    // Create tables
    result = createTables();
    if (result != RAGGER_SUCCESS) {
        shutdown();
        return result;
    }

    std::cout << "IndexManager: Database initialized at " << dbPath_.string() << std::endl;
    return RAGGER_SUCCESS;
}

void IndexManager::shutdown() {
    // Cleanup connection pool first
    cleanupConnectionPool();
    
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
    fileHashes_.clear();
    std::cout << "IndexManager: Shutdown complete" << std::endl;
}

int IndexManager::indexFile(const fs::path& filePath) {
    if (!fs::exists(filePath) || !fs::is_regular_file(filePath)) {
        return RAGGER_ERROR_FILE_NOT_FOUND;
    }

    if (!shouldIndexFile(filePath)) {
        return RAGGER_SUCCESS; // Skip silently
    }

    // Check if file needs reindexing
    if (!needsReindexing(filePath)) {
        return RAGGER_SUCCESS; // Already up to date
    }

    // Calculate hash
    std::string hash = calculateFileHash(filePath);
    if (hash.empty()) {
        return RAGGER_ERROR_OPERATION_NOT_SUPPORTED;
    }

    // Remove old records
    deleteFileRecords(filePath);

    // Insert new file record
    int result = insertFileRecord(filePath, hash);
    if (result != RAGGER_SUCCESS) {
        return result;
    }

    // TODO: Parse file and extract code blocks/symbols
    // This would use the parser plugins in a full implementation

    fileHashes_[filePath.string()] = hash;
    std::cout << "IndexManager: Indexed file " << filePath.string() << std::endl;

    return RAGGER_SUCCESS;
}

int IndexManager::indexDirectory(const fs::path& directory) {
    if (!fs::exists(directory) || !fs::is_directory(directory)) {
        return RAGGER_ERROR_FILE_NOT_FOUND;
    }

    auto files = discoverFiles(directory);
    int indexedCount = 0;

    for (const auto& file : files) {
        if (indexFile(file) == RAGGER_SUCCESS) {
            indexedCount++;
        }
    }

    std::cout << "IndexManager: Indexed " << indexedCount << " files in directory " << directory.string() << std::endl;
    return indexedCount;
}

int IndexManager::removeFromIndex(const fs::path& filePath) {
    int result = deleteFileRecords(filePath);
    if (result == RAGGER_SUCCESS) {
        fileHashes_.erase(filePath.string());
        std::cout << "IndexManager: Removed file from index: " << filePath.string() << std::endl;
    }
    return result;
}

int IndexManager::findFilesByContent(const std::string& query, std::vector<fs::path>& results) {
    // TODO: Implement full-text search
    // This is a placeholder implementation
    results.clear();
    std::cout << "IndexManager: Searching for '" << query << "'" << std::endl;
    return RAGGER_SUCCESS;
}

int IndexManager::findCodeBlocks(const std::string& symbol, std::vector<CodeBlock>& results) {
    // TODO: Implement symbol search
    results.clear();
    std::cout << "IndexManager: Finding code blocks for symbol '" << symbol << "'" << std::endl;
    return RAGGER_SUCCESS;
}

int IndexManager::getFileSymbols(const fs::path& filePath, std::vector<CodeSymbol>& symbols) {
    // TODO: Implement symbol extraction for file
    symbols.clear();
    std::cout << "IndexManager: Getting symbols for file " << filePath.string() << std::endl;
    return RAGGER_SUCCESS;
}

int IndexManager::rebuildIndex() {
    // Clear existing index
    executeQuery("DELETE FROM files");
    executeQuery("DELETE FROM code_blocks");
    executeQuery("DELETE FROM inverted_index");

    fileHashes_.clear();

    // TODO: Reindex all known directories
    std::cout << "IndexManager: Index rebuild completed" << std::endl;
    return RAGGER_SUCCESS;
}

int IndexManager::optimizeIndex() {
    // SQLite optimization
    executeQuery("VACUUM");
    executeQuery("ANALYZE");

    std::cout << "IndexManager: Index optimization completed" << std::endl;
    return RAGGER_SUCCESS;
}

int IndexManager::getIndexStats(size_t& fileCount, size_t& blockCount, size_t& symbolCount) {
    // TODO: Query database for actual counts
    fileCount = fileHashes_.size();
    blockCount = 0; // Placeholder
    symbolCount = 0; // Placeholder
    return RAGGER_SUCCESS;
}

bool IndexManager::needsReindexing(const fs::path& filePath) {
    std::string currentHash = calculateFileHash(filePath);
    if (currentHash.empty()) {
        return true; // File error, reindex
    }

    auto it = fileHashes_.find(filePath.string());
    if (it == fileHashes_.end()) {
        return true; // Not indexed yet
    }

    return it->second != currentHash;
}

int IndexManager::updateFileIndex(const fs::path& filePath) {
    return indexFile(filePath); // Same as initial indexing
}

void IndexManager::setMaxFileSize(size_t maxSize) {
    maxFileSize_ = maxSize;
}

void IndexManager::setDebounceDelay(int milliseconds) {
    debounceDelay_ = milliseconds;
}

void IndexManager::setDatabasePath(const fs::path& dbPath) {
    dbPath_ = dbPath;
}

int IndexManager::createTables() {
    const char* createFilesTable = R"(
        CREATE TABLE IF NOT EXISTS files (
            path TEXT PRIMARY KEY,
            hash TEXT NOT NULL,
            last_indexed INTEGER NOT NULL,
            file_size INTEGER,
            language TEXT
        )
    )";

    const char* createCodeBlocksTable = R"(
        CREATE TABLE IF NOT EXISTS code_blocks (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            file_path TEXT NOT NULL,
            name TEXT,
            content TEXT,
            start_line INTEGER,
            end_line INTEGER,
            block_type TEXT,
            visibility INTEGER DEFAULT 2,
            FOREIGN KEY(file_path) REFERENCES files(path)
        )
    )";

    const char* createInvertedIndexTable = R"(
        CREATE TABLE IF NOT EXISTS inverted_index (
            token TEXT NOT NULL,
            block_id INTEGER NOT NULL,
            frequency INTEGER DEFAULT 1,
            PRIMARY KEY (token, block_id),
            FOREIGN KEY(block_id) REFERENCES code_blocks(id)
        )
    )";

    const char* createSymbolsTable = R"(
        CREATE TABLE IF NOT EXISTS symbols (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            file_path TEXT NOT NULL,
            name TEXT NOT NULL,
            type TEXT NOT NULL,
            signature TEXT,
            definition_block_id INTEGER,
            FOREIGN KEY(file_path) REFERENCES files(path),
            FOREIGN KEY(definition_block_id) REFERENCES code_blocks(id)
        )
    )";

    int result;
    result = executeQuery(createFilesTable);
    if (result != RAGGER_SUCCESS) return result;

    result = executeQuery(createCodeBlocksTable);
    if (result != RAGGER_SUCCESS) return result;

    result = executeQuery(createInvertedIndexTable);
    if (result != RAGGER_SUCCESS) return result;

    result = executeQuery(createSymbolsTable);
    if (result != RAGGER_SUCCESS) return result;

    return RAGGER_SUCCESS;
}

int IndexManager::insertFileRecord(const fs::path& filePath, const std::string& hash) {
    std::stringstream query;
    query << "INSERT OR REPLACE INTO files (path, hash, last_indexed, file_size) VALUES (?, ?, ?, ?)";

    sqlite3_stmt* stmt;
    int result = sqlite3_prepare_v2(db_, query.str().c_str(), -1, &stmt, nullptr);
    if (result != SQLITE_OK) {
        std::cerr << "IndexManager: Failed to prepare insert statement: " << sqlite3_errmsg(db_) << std::endl;
        return RAGGER_ERROR_OPERATION_NOT_SUPPORTED;
    }

    // Get file size
    size_t fileSize = fs::file_size(filePath);

    sqlite3_bind_text(stmt, 1, filePath.string().c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, hash.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 3, std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
    sqlite3_bind_int64(stmt, 4, fileSize);

    result = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (result != SQLITE_DONE) {
        std::cerr << "IndexManager: Failed to insert file record: " << sqlite3_errmsg(db_) << std::endl;
        return RAGGER_ERROR_OPERATION_NOT_SUPPORTED;
    }

    return RAGGER_SUCCESS;
}

int IndexManager::insertCodeBlock(const CodeBlock* block, const fs::path& filePath) {
    // TODO: Implement code block insertion
    return RAGGER_SUCCESS;
}

int IndexManager::updateFileHash(const fs::path& filePath, const std::string& hash) {
    std::stringstream query;
    query << "UPDATE files SET hash = ?, last_indexed = ? WHERE path = ?";

    sqlite3_stmt* stmt;
    int result = sqlite3_prepare_v2(db_, query.str().c_str(), -1, &stmt, nullptr);
    if (result != SQLITE_OK) {
        std::cerr << "IndexManager: Failed to prepare update statement: " << sqlite3_errmsg(db_) << std::endl;
        return RAGGER_ERROR_OPERATION_NOT_SUPPORTED;
    }

    sqlite3_bind_text(stmt, 1, hash.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
    sqlite3_bind_text(stmt, 3, filePath.string().c_str(), -1, SQLITE_STATIC);

    result = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (result == SQLITE_DONE) ? RAGGER_SUCCESS : RAGGER_ERROR_OPERATION_NOT_SUPPORTED;
}

int IndexManager::deleteFileRecords(const fs::path& filePath) {
    // Delete from all tables
    std::vector<std::string> queries = {
        "DELETE FROM inverted_index WHERE block_id IN (SELECT id FROM code_blocks WHERE file_path = ?)",
        "DELETE FROM symbols WHERE file_path = ?",
        "DELETE FROM code_blocks WHERE file_path = ?",
        "DELETE FROM files WHERE path = ?"
    };

    for (const auto& queryStr : queries) {
        sqlite3_stmt* stmt;
        int result = sqlite3_prepare_v2(db_, queryStr.c_str(), -1, &stmt, nullptr);
        if (result != SQLITE_OK) {
            std::cerr << "IndexManager: Failed to prepare delete statement: " << sqlite3_errmsg(db_) << std::endl;
            return RAGGER_ERROR_OPERATION_NOT_SUPPORTED;
        }

        sqlite3_bind_text(stmt, 1, filePath.string().c_str(), -1, SQLITE_STATIC);

        result = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (result != SQLITE_DONE) {
            std::cerr << "IndexManager: Failed to delete records: " << sqlite3_errmsg(db_) << std::endl;
            return RAGGER_ERROR_OPERATION_NOT_SUPPORTED;
        }
    }

    return RAGGER_SUCCESS;
}

std::string IndexManager::calculateFileHash(const fs::path& filePath) {
    try {
        std::string pathStr = filePath.string();
        
        // Check if we have a cached hash and the file hasn't been modified
        auto hashIt = fileHashes_.find(pathStr);
        auto timeIt = hashCache_.find(pathStr);
        
        if (hashIt != fileHashes_.end() && timeIt != hashCache_.end()) {
            // Check if file modification time has changed (simplified check)
            auto fileTime = fs::last_write_time(filePath);
            auto currentTime = std::chrono::steady_clock::now();
            
            // If cache is less than 5 minutes old, use cached hash
            if ((currentTime - timeIt->second) < std::chrono::minutes(5)) {
                return hashIt->second; // Return cached hash
            }
        }
        
        std::ifstream file(filePath, std::ios::binary);
        if (!file) {
            return "";
        }

        // Use modern EVP interface for SHA256
        EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
        if (!mdctx) {
            return "";
        }

        const EVP_MD* md = EVP_sha256();
        if (!EVP_DigestInit_ex(mdctx, md, nullptr)) {
            EVP_MD_CTX_free(mdctx);
            return "";
        }

        char buffer[8192];
        while (file.read(buffer, sizeof(buffer))) {
            if (!EVP_DigestUpdate(mdctx, buffer, file.gcount())) {
                EVP_MD_CTX_free(mdctx);
                return "";
            }
        }
        if (!EVP_DigestUpdate(mdctx, buffer, file.gcount())) {
            EVP_MD_CTX_free(mdctx);
            return "";
        }

        unsigned char hash[SHA256_DIGEST_LENGTH];
        unsigned int hashLen;
        if (!EVP_DigestFinal_ex(mdctx, hash, &hashLen)) {
            EVP_MD_CTX_free(mdctx);
            return "";
        }
        EVP_MD_CTX_free(mdctx);

        std::stringstream ss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        }

        std::string hashStr = ss.str();
        
        // Cache the hash and timestamp
        fileHashes_[pathStr] = hashStr;
        hashCache_[pathStr] = std::chrono::steady_clock::now();

        return hashStr;

    } catch (const std::exception& e) {
        std::cerr << "IndexManager: Error calculating hash for " << filePath.string() << ": " << e.what() << std::endl;
        return "";
    }
}

bool IndexManager::shouldIndexFile(const fs::path& filePath) {
    // Check file size
    try {
        if (fs::file_size(filePath) > maxFileSize_) {
            return false;
        }
    } catch (...) {
        return false;
    }

    // Check file extension (basic filtering)
    std::string ext = filePath.extension().string();
    std::vector<std::string> supportedExts = {
        ".cpp", ".cxx", ".cc", ".c", ".hpp", ".hxx", ".h",
        ".py", ".java", ".js", ".ts", ".rs", ".go"
    };

    for (const auto& supportedExt : supportedExts) {
        if (ext == supportedExt) {
            return true;
        }
    }

    return false;
}

std::vector<fs::path> IndexManager::discoverFiles(const fs::path& directory) {
    std::vector<fs::path> files;

    try {
        for (const auto& entry : fs::recursive_directory_iterator(directory)) {
            if (fs::is_regular_file(entry) && shouldIndexFile(entry.path())) {
                files.push_back(entry.path());
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "IndexManager: Error discovering files in " << directory.string() << ": " << e.what() << std::endl;
    }

    return files;
}

int IndexManager::executeQuery(const std::string& query) {
    char* errorMsg = nullptr;
    int result = sqlite3_exec(db_, query.c_str(), nullptr, nullptr, &errorMsg);

    if (result != SQLITE_OK) {
        std::cerr << "IndexManager: SQL error: " << errorMsg << std::endl;
        sqlite3_free(errorMsg);
        return RAGGER_ERROR_OPERATION_NOT_SUPPORTED;
    }

    return RAGGER_SUCCESS;
}

int IndexManager::executeQueryWithCallback(const std::string& query,
                                          int (*callback)(void*, int, char**, char**),
                                          void* callbackData) {
    char* errorMsg = nullptr;
    int result = sqlite3_exec(db_, query.c_str(), callback, callbackData, &errorMsg);

    if (result != SQLITE_OK) {
        std::cerr << "IndexManager: SQL error: " << errorMsg << std::endl;
        sqlite3_free(errorMsg);
        return RAGGER_ERROR_OPERATION_NOT_SUPPORTED;
    }

    return RAGGER_SUCCESS;
}

// Connection pool implementation
void IndexManager::initializeConnectionPool() {
    std::lock_guard<std::mutex> lock(poolMutex_);
    
    for (size_t i = 0; i < MAX_CONNECTIONS; ++i) {
        DatabaseConnection conn;
        if (sqlite3_open(dbPath_.string().c_str(), &conn.db) == SQLITE_OK) {
            connectionPool_.push_back(conn);
        }
    }
}

void IndexManager::cleanupConnectionPool() {
    std::lock_guard<std::mutex> lock(poolMutex_);
    
    for (auto& conn : connectionPool_) {
        if (conn.db) {
            sqlite3_close(conn.db);
        }
    }
    connectionPool_.clear();
}

sqlite3* IndexManager::getConnection() {
    std::lock_guard<std::mutex> lock(poolMutex_);
    
    // Find available connection
    for (auto& conn : connectionPool_) {
        if (!conn.inUse) {
            conn.inUse = true;
            conn.lastUsed = std::chrono::steady_clock::now();
            return conn.db;
        }
    }
    
    // If no connection available, return the main db (fallback)
    return db_;
}

void IndexManager::returnConnection(sqlite3* db) {
    std::lock_guard<std::mutex> lock(poolMutex_);
    
    for (auto& conn : connectionPool_) {
        if (conn.db == db) {
            conn.inUse = false;
            conn.lastUsed = std::chrono::steady_clock::now();
            break;
        }
    }
}

} // namespace Ragger
