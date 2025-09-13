#include "ragger_plugin_api.h"
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <regex>
#include <algorithm>
#include <map>
#include <set>

// Git integration structures
struct GitBlameInfo {
    std::string commitHash;
    std::string author;
    std::string authorEmail;
    std::string commitMessage;
    std::string date;
    int lineNumber;
    std::string lineContent;
};

struct GitCommitInfo {
    std::string hash;
    std::string author;
    std::string authorEmail;
    std::string message;
    std::string date;
    std::vector<std::string> changedFiles;
};

struct ProjectStandards {
    std::string indentationStyle;
    std::string bracketStyle;
    std::vector<std::string> lintingRules;
    std::vector<std::string> todoComments;
    std::vector<std::string> fixmeComments;
    int cyclomaticComplexity;
    std::string buildSystem;
    std::vector<std::string> dependencies;
};

struct DocumentationInfo {
    std::string sourceFile;
    std::string content;
    std::string relevance;
    std::vector<std::string> keywords;
};

namespace {

// Plugin state
struct GitIntegrationState {
    bool initialized;
    std::string repositoryRoot;
    std::map<std::string, ProjectStandards> projectStandards;
    std::map<std::string, std::vector<DocumentationInfo>> documentationCache;
    
    GitIntegrationState() : initialized(false) {}
};

GitIntegrationState* g_state = nullptr;

} // anonymous namespace

extern "C" {

int plugin_get_abi_version() {
    return RAGGER_PLUGIN_ABI_VERSION;
}

const char* plugin_get_name() {
    return "Git Integration Plugin";
}

const char* plugin_get_version() {
    return "1.0.0";
}

const char* plugin_get_description() {
    return "Git integration plugin for RAGger";
}

int plugin_initialize(RaggerCoreHandle core) {
    if (g_state) {
        return RAGGER_ERROR_OPERATION_NOT_SUPPORTED;
    }

    g_state = new GitIntegrationState();
    g_state->initialized = true;
    
    std::cout << "GitIntegrationPlugin: Initialized" << std::endl;
    return RAGGER_SUCCESS;
}

void plugin_shutdown() {
    if (g_state) {
        delete g_state;
        g_state = nullptr;
    }
    std::cout << "GitIntegrationPlugin: Shutdown" << std::endl;
}

void plugin_register_events(EventCallback callback, void* userData) {
    // Register for Git events
}

void plugin_unregister_events() {
    // Unregister events
}

const char* plugin_get_capabilities() {
    return R"({
        "git_integration": {
            "features": ["blame", "commit_history", "branch_info"]
        }
    })";
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

// Git-specific functions
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

int ragger_git_get_blame_info(const char* filePath, void** blameInfo, size_t* count) {
    if (!g_state || !filePath || !blameInfo || !count) {
        return RAGGER_ERROR_INVALID_ARGUMENT;
    }

    // Get blame information using git blame
    std::string blameOutput = executeGitCommand("git blame -p " + std::string(filePath));
    if (blameOutput.empty()) {
        *count = 0;
        *blameInfo = nullptr;
        return RAGGER_ERROR_FILE_NOT_FOUND;
    }

    std::vector<GitBlameInfo> blameData;
    std::vector<std::string> lines = splitLines(blameOutput);
    
    std::string currentCommit;
    std::string currentAuthor;
    std::string currentEmail;
    std::string currentDate;
    std::string currentMessage;
    
    for (const auto& line : lines) {
        if (line.empty()) continue;
        
        if (line[0] == '\t') {
            // This is the actual line content
            std::string content = line.substr(1);
            if (!currentCommit.empty()) {
                GitBlameInfo info;
                info.commitHash = currentCommit;
                info.author = currentAuthor;
                info.authorEmail = currentEmail;
                info.date = currentDate;
                info.commitMessage = currentMessage;
                info.lineContent = content;
                blameData.push_back(info);
            }
        } else if (line.substr(0, 8) == "author ") {
            currentAuthor = line.substr(8);
        } else if (line.substr(0, 13) == "author-mail ") {
            currentEmail = line.substr(13);
        } else if (line.substr(0, 13) == "author-time ") {
            currentDate = line.substr(13);
        } else if (line.substr(0, 7) == "summary") {
            currentMessage = line.substr(8);
        } else if (line.length() >= 40 && line.find(' ') != std::string::npos) {
            // This is a commit hash and line info
            currentCommit = line.substr(0, 40);
        }
    }

    *count = blameData.size();
    if (*count > 0) {
        *blameInfo = new GitBlameInfo[*count];
        for (size_t i = 0; i < *count; ++i) {
            static_cast<GitBlameInfo*>(*blameInfo)[i] = blameData[i];
        }
    } else {
        *blameInfo = nullptr;
    }
    
    return RAGGER_SUCCESS;
}

void ragger_git_free_blame_info(void* blameInfo, size_t count) {
    if (blameInfo) {
        delete[] static_cast<GitBlameInfo*>(blameInfo);
    }
}

int ragger_git_get_commit_history(const char* filePath, int maxCommits, void** commits, size_t* count) {
    if (!g_state || !filePath || !commits || !count) {
        return RAGGER_ERROR_INVALID_ARGUMENT;
    }

    // TODO: Implement commit history retrieval
    *count = 0;
    *commits = nullptr;
    return RAGGER_SUCCESS;
}

void ragger_git_free_commit_info(void* commits, size_t count) {
    // TODO: Implement proper cleanup
}

int ragger_git_get_current_branch(char** branchName) {
    if (!g_state || !branchName) {
        return RAGGER_ERROR_INVALID_ARGUMENT;
    }

    // TODO: Implement current branch detection
    *branchName = nullptr;
    return RAGGER_SUCCESS;
}

int ragger_git_get_branches(char*** branches, size_t* count) {
    if (!g_state || !branches || !count) {
        return RAGGER_ERROR_INVALID_ARGUMENT;
    }

    // TODO: Implement branch listing
    *count = 0;
    *branches = nullptr;
    return RAGGER_SUCCESS;
}

void ragger_git_free_branches(char** branches, size_t count) {
    // TODO: Implement proper cleanup
}

bool ragger_git_has_uncommitted_changes() {
    return g_state ? false : false; // TODO: Implement change detection
}

int ragger_git_get_modified_files(char*** files, size_t* count) {
    if (!g_state || !files || !count) {
        return RAGGER_ERROR_INVALID_ARGUMENT;
    }

    // TODO: Implement modified files detection
    *count = 0;
    *files = nullptr;
    return RAGGER_SUCCESS;
}

void ragger_git_free_modified_files(char** files, size_t count) {
    if (files) {
        for (size_t i = 0; i < count; ++i) {
            delete[] files[i];
        }
        delete[] files;
    }
}

// Pillar 2: Static Analysis & Project Standards
ProjectStandards analyzeProjectStandards(const std::string& filePath) {
    ProjectStandards standards;
    
    // Check for .clang-format
    std::string clangFormatPath = g_state->repositoryRoot + "/.clang-format";
    if (std::filesystem::exists(clangFormatPath)) {
        std::ifstream file(clangFormatPath);
        std::string line;
        while (std::getline(file, line)) {
            if (line.find("IndentWidth:") != std::string::npos) {
                standards.indentationStyle = line.substr(line.find(":") + 1);
                // Remove whitespace
                standards.indentationStyle.erase(0, standards.indentationStyle.find_first_not_of(" \t"));
            }
            if (line.find("BreakBeforeBraces:") != std::string::npos) {
                standards.bracketStyle = line.substr(line.find(":") + 1);
                standards.bracketStyle.erase(0, standards.bracketStyle.find_first_not_of(" \t"));
            }
        }
    }
    
    // Check for TODO/FIXME comments in the file
    std::ifstream file(filePath);
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    
    std::regex todoRegex(R"(//\s*TODO[:\s]*(.*))", std::regex_constants::icase);
    std::regex fixmeRegex(R"(//\s*FIXME[:\s]*(.*))", std::regex_constants::icase);
    
    std::sregex_iterator todoBegin(content.begin(), content.end(), todoRegex);
    std::sregex_iterator todoEnd;
    for (auto it = todoBegin; it != todoEnd; ++it) {
        standards.todoComments.push_back((*it)[1].str());
    }
    
    std::sregex_iterator fixmeBegin(content.begin(), content.end(), fixmeRegex);
    std::sregex_iterator fixmeEnd;
    for (auto it = fixmeBegin; it != fixmeEnd; ++it) {
        standards.fixmeComments.push_back((*it)[1].str());
    }
    
    // Detect build system
    if (std::filesystem::exists(g_state->repositoryRoot + "/CMakeLists.txt")) {
        standards.buildSystem = "CMake";
    } else if (std::filesystem::exists(g_state->repositoryRoot + "/Makefile")) {
        standards.buildSystem = "Make";
    } else if (std::filesystem::exists(g_state->repositoryRoot + "/package.json")) {
        standards.buildSystem = "npm";
    }
    
    return standards;
}

// Pillar 3: Documentation Integration
std::vector<DocumentationInfo> findRelevantDocumentation(const std::string& query, const std::string& filePath) {
    std::vector<DocumentationInfo> docs;
    
    // Search in common documentation files
    std::vector<std::string> docFiles = {
        "README.md", "CONTRIBUTING.md", "docs/", "doc/"
    };
    
    for (const auto& docFile : docFiles) {
        std::string fullPath = g_state->repositoryRoot + "/" + docFile;
        if (std::filesystem::exists(fullPath)) {
            if (std::filesystem::is_directory(fullPath)) {
                // Search all files in directory
                for (const auto& entry : std::filesystem::recursive_directory_iterator(fullPath)) {
                    if (entry.is_regular_file() && 
                        (entry.path().extension() == ".md" || entry.path().extension() == ".txt")) {
                        std::ifstream file(entry.path());
                        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                        
                        // Simple keyword matching
                        std::string lowerContent = content;
                        std::string lowerQuery = query;
                        std::transform(lowerContent.begin(), lowerContent.end(), lowerContent.begin(), ::tolower);
                        std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
                        
                        if (lowerContent.find(lowerQuery) != std::string::npos) {
                            DocumentationInfo info;
                            info.sourceFile = entry.path().string();
                            info.content = content.substr(0, 500); // First 500 chars
                            info.relevance = "High";
                            docs.push_back(info);
                        }
                    }
                }
            } else {
                // Single file
                std::ifstream file(fullPath);
                std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                
                std::string lowerContent = content;
                std::string lowerQuery = query;
                std::transform(lowerContent.begin(), lowerContent.end(), lowerContent.begin(), ::tolower);
                std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
                
                if (lowerContent.find(lowerQuery) != std::string::npos) {
                    DocumentationInfo info;
                    info.sourceFile = fullPath;
                    info.content = content.substr(0, 500);
                    info.relevance = "High";
                    docs.push_back(info);
                }
            }
        }
    }
    
    return docs;
}

// Main context generation function that combines all three pillars
std::string generateComprehensiveContext(const char* filePath, const char* userQuery, int startLine, int endLine) {
    if (!g_state || !filePath || !userQuery) {
        return "";
    }
    
    std::string context = "=== COMPREHENSIVE RAG CONTEXT ===\n\n";
    
    // Pillar 1: Git Context
    context += "### GIT CONTEXT\n";
    void* blameInfo = nullptr;
    size_t blameCount = 0;
    
    if (ragger_git_get_blame_info(filePath, &blameInfo, &blameCount) == RAGGER_SUCCESS && blameInfo) {
        GitBlameInfo* blame = static_cast<GitBlameInfo*>(blameInfo);
        
        // Find the most recent change in the specified line range
        std::string lastAuthor = "Unknown";
        std::string lastCommit = "Unknown";
        std::string lastMessage = "No commit message";
        
        for (size_t i = 0; i < blameCount; ++i) {
            if (blame[i].lineNumber >= startLine && blame[i].lineNumber <= endLine) {
                lastAuthor = blame[i].author;
                lastCommit = blame[i].commitHash.substr(0, 8);
                lastMessage = blame[i].commitMessage;
                break;
            }
        }
        
        context += "* **Last Change:** The selected code was last modified by \"" + lastAuthor + "\" in commit `" + lastCommit + "`.\n";
        context += "* **Commit Message:** \"" + lastMessage + "\"\n";
        
        // Get recent commit history
        void* commits = nullptr;
        size_t commitCount = 0;
        if (ragger_git_get_commit_history(filePath, 5, &commits, &commitCount) == RAGGER_SUCCESS) {
            context += "* **Recent File History:** This file has been recently modified with " + std::to_string(commitCount) + " recent commits.\n";
        }
        
        ragger_git_free_blame_info(blameInfo, blameCount);
    } else {
        context += "* **Git Information:** Not available (file may not be in a Git repository)\n";
    }
    context += "\n";
    
    // Pillar 2: Project Standards & Analysis
    context += "### PROJECT STANDARDS & ANALYSIS\n";
    ProjectStandards standards = analyzeProjectStandards(filePath);
    
    if (!standards.indentationStyle.empty()) {
        context += "* **Formatting:** This project uses " + standards.indentationStyle + " indentation";
        if (!standards.bracketStyle.empty()) {
            context += " and " + standards.bracketStyle + " bracket style";
        }
        context += ".\n";
    }
    
    if (!standards.buildSystem.empty()) {
        context += "* **Build System:** " + standards.buildSystem + "\n";
    }
    
    if (!standards.fixmeComments.empty()) {
        context += "* **Analysis Note:** " + std::to_string(standards.fixmeComments.size()) + " FIXME comment(s) found near this code block.\n";
    }
    
    if (!standards.todoComments.empty()) {
        context += "* **Analysis Note:** " + std::to_string(standards.todoComments.size()) + " TODO comment(s) found near this code block.\n";
    }
    context += "\n";
    
    // Pillar 3: Relevant Documentation
    context += "### RELEVANT DOCUMENTATION\n";
    std::vector<DocumentationInfo> docs = findRelevantDocumentation(userQuery, filePath);
    
    if (!docs.empty()) {
        for (const auto& doc : docs) {
            context += "* **From `" + std::filesystem::path(doc.sourceFile).filename().string() + "`:** ";
            context += doc.content.substr(0, 200) + "...\n";
        }
    } else {
        context += "* **Documentation:** No relevant documentation found for this query.\n";
    }
    context += "\n";
    
    return context;
}

// Export the comprehensive context generation function
extern "C" {
    const char* ragger_generate_comprehensive_context(const char* filePath, const char* userQuery, int startLine, int endLine) {
        static std::string result;
        result = generateComprehensiveContext(filePath, userQuery, startLine, endLine);
        return result.c_str();
    }
}

} // extern "C"