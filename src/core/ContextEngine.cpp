#include "ContextEngine.h"
#include "ragger_plugin_api.h"
// spdlog disabled
#include <algorithm>
#include <cstring>
#include <iostream>

namespace Ragger {

ContextEngine::ContextEngine(RaggerCoreAPI* coreAPI)
    : coreAPI_(coreAPI), maxContextTokens_(8000) {
    // Default ranking weights (BM25, Graph, TF-IDF)
    rankingWeights_ = {0.6f, 0.3f, 0.1f};
    loadDefaultTemplates();
}

int ContextEngine::generateContext(const ContextRequest* request, ContextResponse** response) {
    if (!request || !response) {
        return RAGGER_ERROR_INVALID_ARGUMENT;
    }

    *response = new ContextResponse();
    (*response)->numResults = 0;
    (*response)->results = nullptr;
    (*response)->totalTokens = 0;
    (*response)->promptTemplate = nullptr;

    // TODO: In a full implementation, this would:
    // 1. Query ranking plugins with the request
    // 2. Aggregate and filter results
    // 3. Build context using templates

    // Debug: Generated context for request

    // Placeholder: set basic template
    static const char* defaultTemplate = "default";
    (*response)->promptTemplate = new char[strlen(defaultTemplate) + 1];
    strcpy(const_cast<char*>((*response)->promptTemplate), defaultTemplate);

    return RAGGER_SUCCESS;
}

void ContextEngine::freeResponse(ContextResponse* response) {
    if (!response) return;

    delete[] response->results;
    delete[] response->promptTemplate;
    delete response;
}

int ContextEngine::loadTemplate(const std::string& name, const std::string& content) {
    templates_[name] = content;
    std::cout << "ContextEngine: Loaded template '" << name << "'" << std::endl;
    return RAGGER_SUCCESS;
}

int ContextEngine::getAvailableTemplates(std::vector<std::string>& templates) const {
    templates.clear();
    for (const auto& pair : templates_) {
        templates.push_back(pair.first);
    }
    return RAGGER_SUCCESS;
}

std::string ContextEngine::getTemplate(const std::string& name) const {
    auto it = templates_.find(name);
    return it != templates_.end() ? it->second : "";
}

void ContextEngine::setRankingWeights(const std::vector<float>& weights) {
    rankingWeights_ = weights;
}

const std::vector<float>& ContextEngine::getRankingWeights() const {
    return rankingWeights_;
}

void ContextEngine::setMaxContextTokens(int maxTokens) {
    maxContextTokens_ = maxTokens;
}

int ContextEngine::getMaxContextTokens() const {
    return maxContextTokens_;
}

std::string ContextEngine::buildPromptFromTemplate(const std::string& templateName,
                                                 const ContextRequest* request,
                                                 const ContextResponse* contextData) {
    std::string tmpl = getTemplate(templateName);
    if (tmpl.empty()) {
        tmpl = getTemplate("default");
    }

    // TODO: Implement template variable substitution
    // For now, return a basic prompt
    return "Please analyze the following code and provide insights.";
}

int ContextEngine::estimateTokenCount(const std::string& text) const {
    // Rough estimation: 1 token ≈ 4 characters for English text
    // This is a simplification; actual tokenization is more complex
    return text.length() / 4;
}

std::string ContextEngine::truncateToTokenLimit(const std::string& text, int maxTokens) {
    int estimatedTokens = estimateTokenCount(text);
    if (estimatedTokens <= maxTokens) {
        return text;
    }

    // Simple truncation - in practice, you'd want smarter truncation
    int targetLength = maxTokens * 4;
    if (targetLength >= static_cast<int>(text.length())) {
        return text;
    }

    return text.substr(0, targetLength) + "...";
}

void ContextEngine::loadDefaultTemplates() {
    // Default template
    std::string defaultTemplate = R"(
You are an expert programmer. Please analyze the following code:

{{CODE}}

Context information:
{{CONTEXT}}

Please provide your analysis and suggestions.
)";

    // AlphaCodium-style template
    std::string alphaCodiumTemplate = R"(
You are an expert programmer following test-driven development methodology.

## User Request
{{REQUEST}}

## Primary Code
```cpp
{{CODE}}
```

## Existing Tests
{{TESTS}}

## Related Code
{{CONTEXT}}

## Task
Follow these steps precisely:
1. Analyze the code and its context
2. Propose test cases if needed
3. Provide the solution that passes all tests
)";

    templates_["default"] = defaultTemplate;
    templates_["alpha_codium"] = alphaCodiumTemplate;

    std::cout << "ContextEngine: Loaded " << templates_.size() << " default templates" << std::endl;
}

} // namespace Ragger
