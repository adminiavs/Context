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
    struct DocsState {
        bool initialized;
        std::string projectRoot;
        
        DocsState() : initialized(false) {}
    };
    
    DocsState* g_state = nullptr;
}

// Helper function to find project root
std::string findProjectRoot(const std::string& startPath = ".") {
    std::string currentPath = startPath;
    while (!currentPath.empty() && currentPath != "/") {
        // Check for common project indicators
        if (std::filesystem::exists(currentPath + "/README.md") ||
            std::filesystem::exists(currentPath + "/CMakeLists.txt") ||
            std::filesystem::exists(currentPath + "/Makefile") ||
            std::filesystem::exists(currentPath + "/.git")) {
            return std::filesystem::absolute(currentPath).string();
        }
        currentPath = std::filesystem::path(currentPath).parent_path().string();
    }
    return std::filesystem::absolute(".").string();
}

// Extract relevant content from README.md based on query keywords
std::string extractRelevantReadmeContent(const std::string& readmePath, const std::string& query) {
    if (!std::filesystem::exists(readmePath)) {
        return "No relevant sections found.";
    }
    
    std::ifstream file(readmePath);
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    
    // Convert query to lowercase for case-insensitive matching
    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
    
    // Split content into lines
    std::vector<std::string> lines;
    std::istringstream stream(content);
    std::string line;
    while (std::getline(stream, line)) {
        lines.push_back(line);
    }
    
    // Look for lines that contain keywords from the query
    std::vector<std::string> keywords;
    std::istringstream queryStream(lowerQuery);
    std::string word;
    while (queryStream >> word) {
        // Remove common words
        if (word != "the" && word != "a" && word != "an" && word != "and" && 
            word != "or" && word != "but" && word != "in" && word != "on" && 
            word != "at" && word != "to" && word != "for" && word != "of" && 
            word != "with" && word != "by" && word != "is" && word != "are" &&
            word != "was" && word != "were" && word != "be" && word != "been" &&
            word != "have" && word != "has" && word != "had" && word != "do" &&
            word != "does" && word != "did" && word != "will" && word != "would" &&
            word != "could" && word != "should" && word != "may" && word != "might") {
            keywords.push_back(word);
        }
    }
    
    // Find relevant lines
    for (const auto& line : lines) {
        std::string lowerLine = line;
        std::transform(lowerLine.begin(), lowerLine.end(), lowerLine.begin(), ::tolower);
        
        for (const auto& keyword : keywords) {
            if (lowerLine.find(keyword) != std::string::npos && line.length() > 20) {
                // Found a relevant line, return it (truncated if too long)
                std::string result = line;
                if (result.length() > 200) {
                    result = result.substr(0, 197) + "...";
                }
                return result;
            }
        }
    }
    
    return "No relevant sections found.";
}

// Extract Doxygen-style comments from file
std::string extractDoxygenComments(const std::string& filePath, const std::string& query) {
    if (!std::filesystem::exists(filePath)) {
        return "No API documentation found.";
    }
    
    std::ifstream file(filePath);
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    
    // Look for Doxygen-style comments (/** ... */)
    std::regex doxygenRegex(R"(\/\*\*([^*]|\*(?!\/))*\*\/)");
    std::sregex_iterator begin(content.begin(), content.end(), doxygenRegex);
    std::sregex_iterator end;
    
    for (auto it = begin; it != end; ++it) {
        std::string comment = it->str();
        
        // Clean up the comment (remove /** and */, clean up whitespace)
        comment = std::regex_replace(comment, std::regex(R"(\/\*\*|\*\/)"), "");
        comment = std::regex_replace(comment, std::regex(R"(\s*\*\s*)"), " ");
        comment = std::regex_replace(comment, std::regex(R"(\s+)"), " ");
        
        // Remove leading/trailing whitespace
        comment.erase(0, comment.find_first_not_of(" \t\n\r"));
        comment.erase(comment.find_last_not_of(" \t\n\r") + 1);
        
        if (!comment.empty() && comment.length() > 10) {
            // Truncate if too long
            if (comment.length() > 200) {
                comment = comment.substr(0, 197) + "...";
            }
            return comment;
        }
    }
    
    return "No API documentation found.";
}

extern "C" {

int plugin_get_abi_version() {
    return RAGGER_PLUGIN_ABI_VERSION;
}

const char* plugin_get_name() {
    return "DocsPlugin";
}

const char* plugin_get_version() {
    return "1.0.0";
}

const char* plugin_get_description() {
    return "Documentation plugin for RAGger - provides architectural and functional context from project documentation";
}

int plugin_initialize(RaggerCoreHandle core) {
    if (g_state) {
        return RAGGER_ERROR_OPERATION_NOT_SUPPORTED;
    }

    g_state = new DocsState();
    g_state->initialized = true;
    g_state->projectRoot = findProjectRoot();
    
    std::cout << "DocsPlugin: Initialized" << std::endl;
    std::cout << "DocsPlugin: Project root found at " << g_state->projectRoot << std::endl;
    
    return RAGGER_SUCCESS;
}

void plugin_shutdown() {
    if (g_state) {
        delete g_state;
        g_state = nullptr;
    }
    std::cout << "DocsPlugin: Shutdown" << std::endl;
}

void plugin_register_events(EventCallback callback, void* userData) {
    // Register for context generation events
}

void plugin_unregister_events() {
    // Unregister events
}

const char* plugin_get_capabilities() {
    return R"({
        "documentation": {
            "features": ["readme_parsing", "doxygen_extraction", "keyword_matching"]
        }
    })";
}

// Main function to generate documentation context
const char* generateDocumentationContext(const char* filePath, const char* query) {
    static std::string result;
    result.clear();
    
    if (!g_state || !filePath || !query) {
        result = "### RELEVANT DOCUMENTATION\n* **From README.md:** No relevant sections found.\n* **From API Docs:** No API documentation found.\n";
        return result.c_str();
    }
    
    result = "### RELEVANT DOCUMENTATION\n";
    
    // Extract relevant content from README.md
    std::string readmePath = g_state->projectRoot + "/README.md";
    std::string readmeContent = extractRelevantReadmeContent(readmePath, query);
    result += "* **From README.md:** " + readmeContent + "\n";
    
    // Extract Doxygen comments from the file
    std::string doxygenContent = extractDoxygenComments(filePath, query);
    result += "* **From API Docs:** " + doxygenContent + "\n";
    
    return result.c_str();
}

} // extern "C"
