#pragma once

#include "ragger_plugin_api.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>

namespace Ragger {

struct RaggerCoreAPI;

class ContextEngine {
public:
    explicit ContextEngine(RaggerCoreAPI* coreAPI);
    ~ContextEngine() = default;

    // Context generation
    int generateContext(const ContextRequest* request, ContextResponse** response);
    void freeResponse(ContextResponse* response);

    // Template management
    int loadTemplate(const std::string& name, const std::string& content);
    int getAvailableTemplates(std::vector<std::string>& templates) const;
    std::string getTemplate(const std::string& name) const;

    // Ranking integration
    void setRankingWeights(const std::vector<float>& weights);
    const std::vector<float>& getRankingWeights() const;

    // Context optimization
    void setMaxContextTokens(int maxTokens);
    int getMaxContextTokens() const;

private:
    RaggerCoreAPI* coreAPI_;
    std::unordered_map<std::string, std::string> templates_;
    std::vector<float> rankingWeights_;
    int maxContextTokens_;

    // Context building helpers
    std::string buildPromptFromTemplate(const std::string& templateName,
                                      const ContextRequest* request,
                                      const ContextResponse* contextData);
    int estimateTokenCount(const std::string& text) const;
    std::string truncateToTokenLimit(const std::string& text, int maxTokens);

    // Default templates
    void loadDefaultTemplates();

    // Prevent copying
    ContextEngine(const ContextEngine&) = delete;
    ContextEngine& operator=(const ContextEngine&) = delete;
};

} // namespace Ragger
