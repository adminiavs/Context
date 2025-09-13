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
    struct StaticAnalysisState {
        bool initialized;
        std::string projectRoot;
        
        StaticAnalysisState() : initialized(false) {}
    };
    
    StaticAnalysisState* g_state = nullptr;
}

// Helper function to find project root
std::string findProjectRoot(const std::string& startPath = ".") {
    std::string currentPath = startPath;
    while (!currentPath.empty() && currentPath != "/") {
        // Check for common project indicators
        if (std::filesystem::exists(currentPath + "/CMakeLists.txt") ||
            std::filesystem::exists(currentPath + "/Makefile") ||
            std::filesystem::exists(currentPath + "/package.json") ||
            std::filesystem::exists(currentPath + "/.git")) {
            return std::filesystem::absolute(currentPath).string();
        }
        currentPath = std::filesystem::path(currentPath).parent_path().string();
    }
    return std::filesystem::absolute(".").string();
}

// Parse .clang-format file
struct ClangFormatInfo {
    std::string basedOnStyle;
    std::string indentWidth;
    bool found;
    
    ClangFormatInfo() : found(false) {}
};

ClangFormatInfo parseClangFormat(const std::string& filePath) {
    ClangFormatInfo info;
    
    if (!std::filesystem::exists(filePath)) {
        return info;
    }
    
    std::ifstream file(filePath);
    std::string line;
    
    while (std::getline(file, line)) {
        // Remove leading/trailing whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        
        if (line.find("BasedOnStyle:") != std::string::npos) {
            info.basedOnStyle = line.substr(line.find(":") + 1);
            // Remove leading whitespace
            info.basedOnStyle.erase(0, info.basedOnStyle.find_first_not_of(" \t"));
            info.found = true;
        } else if (line.find("IndentWidth:") != std::string::npos) {
            info.indentWidth = line.substr(line.find(":") + 1);
            // Remove leading whitespace
            info.indentWidth.erase(0, info.indentWidth.find_first_not_of(" \t"));
            info.found = true;
        }
    }
    
    return info;
}

// Find TODO/FIXME comments in file
struct TodoComment {
    int lineNumber;
    std::string message;
    std::string type; // "TODO" or "FIXME"
};

std::vector<TodoComment> findTodoComments(const std::string& filePath) {
    std::vector<TodoComment> comments;
    
    if (!std::filesystem::exists(filePath)) {
        return comments;
    }
    
    std::ifstream file(filePath);
    std::string line;
    int lineNumber = 1;
    
    std::regex todoRegex(R"(//\s*(TODO|FIXME)[:\s]*(.*))", std::regex_constants::icase);
    
    while (std::getline(file, line)) {
        std::smatch match;
        if (std::regex_search(line, match, todoRegex)) {
            TodoComment comment;
            comment.lineNumber = lineNumber;
            comment.type = match[1].str();
            comment.message = match[2].str();
            // Remove leading/trailing whitespace from message
            comment.message.erase(0, comment.message.find_first_not_of(" \t"));
            comment.message.erase(comment.message.find_last_not_of(" \t") + 1);
            comments.push_back(comment);
        }
        lineNumber++;
    }
    
    return comments;
}

extern "C" {

int plugin_get_abi_version() {
    return RAGGER_PLUGIN_ABI_VERSION;
}

const char* plugin_get_name() {
    return "StaticAnalysisPlugin";
}

const char* plugin_get_version() {
    return "1.0.0";
}

const char* plugin_get_description() {
    return "Static analysis plugin for RAGger - provides project standards and code quality analysis";
}

int plugin_initialize(RaggerCoreHandle core) {
    if (g_state) {
        return RAGGER_ERROR_OPERATION_NOT_SUPPORTED;
    }

    g_state = new StaticAnalysisState();
    g_state->initialized = true;
    g_state->projectRoot = findProjectRoot();
    
    std::cout << "StaticAnalysisPlugin: Initialized" << std::endl;
    std::cout << "StaticAnalysisPlugin: Project root found at " << g_state->projectRoot << std::endl;
    
    return RAGGER_SUCCESS;
}

void plugin_shutdown() {
    if (g_state) {
        delete g_state;
        g_state = nullptr;
    }
    std::cout << "StaticAnalysisPlugin: Shutdown" << std::endl;
}

void plugin_register_events(EventCallback callback, void* userData) {
    // Register for context generation events
}

void plugin_unregister_events() {
    // Unregister events
}

const char* plugin_get_capabilities() {
    return R"({
        "static_analysis": {
            "features": ["clang_format_parsing", "todo_comment_detection", "code_quality_analysis"]
        }
    })";
}

// Main function to generate static analysis context
const char* generateStaticAnalysisContext(const char* filePath) {
    static std::string result;
    result.clear();
    
    if (!g_state || !filePath) {
        result = "### PROJECT STANDARDS & ANALYSIS\n* **Formatting Rules (.clang-format):** Not found.\n* **Actionable Comments:** No TODO/FIXME comments found.\n";
        return result.c_str();
    }
    
    result = "### PROJECT STANDARDS & ANALYSIS\n";
    
    // Check for .clang-format file
    std::string clangFormatPath = g_state->projectRoot + "/.clang-format";
    ClangFormatInfo clangInfo = parseClangFormat(clangFormatPath);
    
    if (clangInfo.found) {
        result += "* **Formatting Rules (.clang-format):** Found. Style: " + clangInfo.basedOnStyle;
        if (!clangInfo.indentWidth.empty()) {
            result += ", Indent Width: " + clangInfo.indentWidth;
        }
        result += ".\n";
    } else {
        result += "* **Formatting Rules (.clang-format):** Not found.\n";
    }
    
    // Find TODO/FIXME comments
    std::vector<TodoComment> comments = findTodoComments(filePath);
    
    if (!comments.empty()) {
        // Use the first TODO comment found
        TodoComment firstComment = comments[0];
        result += "* **Actionable Comments:** Found a `// " + firstComment.type + ":` comment on line " + 
                 std::to_string(firstComment.lineNumber) + " with the message: \"" + firstComment.message + "\".\n";
    } else {
        result += "* **Actionable Comments:** No TODO/FIXME comments found.\n";
    }
    
    return result.c_str();
}

} // extern "C"
