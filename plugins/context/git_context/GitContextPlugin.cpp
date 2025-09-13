#include "ragger_plugin_api.h"
#include <iostream>
#include <string>
#include <cstring>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <regex>
#include <algorithm>
#include <vector>

namespace {
    // Plugin state
    struct GitContextState {
        bool initialized;
        std::string repositoryRoot;
        
        GitContextState() : initialized(false) {}
    };
    
    GitContextState* g_state = nullptr;
}

// Helper functions for Git operations
std::string executeGitCommand(const std::string& command, const std::string& workingDir = "") {
    std::string fullCommand = "cd " + (workingDir.empty() ? g_state->repositoryRoot : workingDir) + " && " + command + " 2>/dev/null";
    FILE* pipe = popen(fullCommand.c_str(), "r");
    if (!pipe) return "";
    
    std::string result;
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);
    
    // Remove trailing newline
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }
    return result;
}

std::vector<std::string> splitLines(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        lines.push_back(line);
    }
    return lines;
}

// Git context structures
struct GitCommitInfo {
    std::string hash;
    std::string message;
    std::string author;
    std::string date;
};

struct GitBlameInfo {
    std::string commitHash;
    std::string author;
    std::string lineContent;
    int lineNumber;
};

extern "C" {

int plugin_get_abi_version() {
    return RAGGER_PLUGIN_ABI_VERSION;
}

const char* plugin_get_name() {
    return "GitContextPlugin";
}

const char* plugin_get_version() {
    return "1.0.0";
}

const char* plugin_get_description() {
    return "Git context plugin for RAGger - provides historical context about code evolution";
}

int plugin_initialize(RaggerCoreHandle core) {
    if (g_state) {
        return RAGGER_ERROR_OPERATION_NOT_SUPPORTED;
    }

    g_state = new GitContextState();
    g_state->initialized = true;
    
    // Try to find repository root
    std::string currentPath = ".";
    while (!currentPath.empty() && currentPath != "/") {
        if (std::filesystem::exists(currentPath + "/.git")) {
            g_state->repositoryRoot = std::filesystem::absolute(currentPath).string();
            break;
        }
        currentPath = std::filesystem::path(currentPath).parent_path().string();
    }
    
    std::cout << "GitContextPlugin: Initialized" << std::endl;
    if (!g_state->repositoryRoot.empty()) {
        std::cout << "GitContextPlugin: Repository root found at " << g_state->repositoryRoot << std::endl;
    }
    
    return RAGGER_SUCCESS;
}

void plugin_shutdown() {
    if (g_state) {
        delete g_state;
        g_state = nullptr;
    }
    std::cout << "GitContextPlugin: Shutdown" << std::endl;
}

void plugin_register_events(EventCallback callback, void* userData) {
    // Register for context generation events
}

void plugin_unregister_events() {
    // Unregister events
}

const char* plugin_get_capabilities() {
    return R"({
        "git_context": {
            "features": ["commit_history", "blame_analysis", "file_evolution"]
        }
    })";
}

// Main function to generate Git context
const char* generateGitContext(const char* filePath, int startLine, int endLine) {
    static std::string result;
    result.clear();
    
    if (!g_state || !filePath || g_state->repositoryRoot.empty()) {
        result = "### GIT CONTEXT\n* **File Last Changed In:** Not available (not in a Git repository)\n* **Function Blame:** Not available (not in a Git repository)\n";
        return result.c_str();
    }
    
    result = "### GIT CONTEXT\n";
    
    // Get the most recent commit for the file
    std::string logOutput = executeGitCommand("git log -1 --pretty=format:\"%H|%s|%an|%ad\" --date=short " + std::string(filePath));
    
    std::string commitHash = "Unknown";
    std::string commitMessage = "No commit message";
    std::string author = "Unknown";
    
    if (!logOutput.empty()) {
        std::vector<std::string> parts;
        std::istringstream stream(logOutput);
        std::string part;
        while (std::getline(stream, part, '|')) {
            parts.push_back(part);
        }
        
        if (parts.size() >= 4) {
            commitHash = parts[0].substr(0, 8); // Short hash
            commitMessage = parts[1];
            author = parts[2];
        }
    }
    
    result += "* **File Last Changed In:** " + commitHash + " - \"" + commitMessage + "\" by " + author + "\n";
    
    // Get blame information for the specific lines
    std::string blameOutput = executeGitCommand("git blame -L " + std::to_string(startLine) + "," + std::to_string(endLine) + " " + std::string(filePath));
    
    std::string blameAuthor = "Unknown";
    if (!blameOutput.empty()) {
        std::vector<std::string> lines = splitLines(blameOutput);
        if (!lines.empty()) {
            // Parse the first line of blame output
            std::string firstLine = lines[0];
            // Extract author from blame output (format: hash (author date) line)
            std::regex authorRegex(R"(\(([^)]+)\s+\d{4}-\d{2}-\d{2})");
            std::smatch match;
            if (std::regex_search(firstLine, match, authorRegex)) {
                blameAuthor = match[1].str();
            }
        }
    }
    
    result += "* **Function Blame:** The selected code block was primarily authored by " + blameAuthor + ".\n";
    
    return result.c_str();
}

// Plugin interface functions
bool ragger_git_is_repository(const char* path) {
    if (!g_state || !path) {
        return false;
    }
    
    std::string gitDir = std::string(path) + "/.git";
    return std::filesystem::exists(gitDir) || std::filesystem::exists(gitDir + "/HEAD");
}

int ragger_git_get_repository_root(const char* path, char** rootPath) {
    if (!g_state || !path || !rootPath) {
        return RAGGER_ERROR_INVALID_ARGUMENT;
    }

    std::string result = executeGitCommand("git rev-parse --show-toplevel", std::string(path));
    if (result.empty()) {
        *rootPath = nullptr;
        return RAGGER_ERROR_FILE_NOT_FOUND;
    }
    
    *rootPath = new char[result.length() + 1];
    strcpy(*rootPath, result.c_str());
    g_state->repositoryRoot = result;
    return RAGGER_SUCCESS;
}

void ragger_git_free_string(char* str) {
    delete[] str;
}

} // extern "C"
