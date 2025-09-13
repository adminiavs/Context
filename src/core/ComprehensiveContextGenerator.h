#pragma once

#include <string>
#include <vector>
#include <memory>

namespace Ragger {
namespace Core {

struct PluginInfo {
    std::string name;
    std::string className;
    void* handle;
};

class ComprehensiveContextGenerator {
public:
    ComprehensiveContextGenerator();
    ~ComprehensiveContextGenerator();

    // Initialize the context generator and load plugins
    bool initialize();
    
    // Cleanup resources
    void cleanup();
    
    // Main function to generate comprehensive context
    std::string generateComprehensiveContext(
        const std::string& filePath, 
        const std::string& query, 
        int startLine = 1, 
        int endLine = 100);

private:
    // Load a specific plugin
    bool loadPlugin(const std::string& pluginName, const std::string& pluginClass);
    
    // Generate context from each plugin
    std::string generateGitContext(const std::string& filePath, int startLine, int endLine);
    std::string generateStaticAnalysisContext(const std::string& filePath);
    std::string generateDocumentationContext(const std::string& filePath, const std::string& query);
    
    // Plugin management
    std::vector<PluginInfo> m_plugins;
    bool m_initialized;
};

} // namespace Core
} // namespace Ragger
