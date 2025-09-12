#include "ragger_plugin_api.h"
#include <cmath>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <iostream>
#include <cstring>

namespace {

// BM25 Implementation
class BM25Scorer {
public:
    BM25Scorer(float k1 = 1.5f, float b = 0.75f)
        : k1_(k1), b_(b), totalDocs_(0) {}

    void addDocument(const std::string& docId, const std::vector<std::string>& terms) {
        docLengths_[docId] = terms.size();
        totalDocs_++;

        for (const auto& term : terms) {
            termFreq_[term][docId]++;
            docFreq_[term]++;
        }
    }

    float score(const std::string& docId, const std::vector<std::string>& queryTerms) {
        if (docLengths_.find(docId) == docLengths_.end()) {
            return 0.0f;
        }

        float score = 0.0f;
        size_t docLen = docLengths_[docId];
        float avgDocLen = computeAvgDocLength();

        for (const auto& term : queryTerms) {
            size_t tf = termFreq_[term][docId];
            size_t df = docFreq_[term];

            if (df == 0 || tf == 0) continue;

            float idf = std::log((totalDocs_ - df + 0.5f) / (df + 0.5f));
            float tfNorm = (tf * (k1_ + 1)) / (tf + k1_ * (1 - b_ + b_ * docLen / avgDocLen));

            score += idf * tfNorm;
        }

        return score;
    }

private:
    float k1_, b_;
    size_t totalDocs_;
    std::unordered_map<std::string, size_t> docLengths_;
    std::unordered_map<std::string, std::unordered_map<std::string, size_t>> termFreq_;
    std::unordered_map<std::string, size_t> docFreq_;

    float computeAvgDocLength() const {
        if (docLengths_.empty()) return 0.0f;

        size_t totalLen = 0;
        for (const auto& pair : docLengths_) {
            totalLen += pair.second;
        }
        return static_cast<float>(totalLen) / docLengths_.size();
    }
};

// Plugin state
struct BM25RankerState {
    BM25Scorer* scorer;
    float defaultWeight;
    std::string rankingType;

    BM25RankerState() : scorer(nullptr), defaultWeight(0.6f), rankingType("bm25") {
        scorer = new BM25Scorer();
    }

    ~BM25RankerState() {
        delete scorer;
    }
};

BM25RankerState* g_state = nullptr;

} // anonymous namespace

// Plugin API implementation
extern "C" {

int plugin_get_abi_version() {
    return RAGGER_PLUGIN_ABI_VERSION;
}

const char* plugin_get_name() {
    return "BM25 Ranker";
}

const char* plugin_get_version() {
    return "1.0.0";
}

const char* plugin_get_description() {
    return "BM25 ranking algorithm for relevance scoring";
}

int plugin_initialize(RaggerCoreHandle core) {
    if (g_state) {
        return RAGGER_ERROR_OPERATION_NOT_SUPPORTED; // Already initialized
    }

    g_state = new BM25RankerState();
    std::cout << "BM25RankerPlugin: Initialized" << std::endl;
    return RAGGER_SUCCESS;
}

void plugin_shutdown() {
    if (g_state) {
        delete g_state;
        g_state = nullptr;
    }
    std::cout << "BM25RankerPlugin: Shutdown" << std::endl;
}

void plugin_register_events(EventCallback callback, void* userData) {
    // Register for indexing events to update the scorer
    // This would be implemented with the actual event system
}

void plugin_unregister_events() {
    // Unregister events
}

const char* plugin_get_capabilities() {
    return R"({
        "ranking": {
            "type": "bm25",
            "default_weight": 0.6,
            "supports": ["keyword_relevance", "document_scoring"]
        }
    })";
}

// Ranker-specific functions are now defined in the extern "C" block below

// Plugin interface functions (exported)
extern "C" int ragger_plugin_get_interface(PluginHandle handle, PluginInterface* interface) {
    if (!interface) {
        return RAGGER_ERROR_INVALID_ARGUMENT;
    }

    interface->type = PLUGIN_TYPE_RANKER;
    interface->api = nullptr; // API functions are called directly

    return RAGGER_SUCCESS;
}

// Export ranking functions with proper names
const char* ragger_ranker_get_ranking_type() {
    return g_state ? g_state->rankingType.c_str() : "bm25";
}

float ragger_ranker_get_default_weight() {
    return g_state ? g_state->defaultWeight : 0.6f;
}

int ragger_ranker_rank_blocks(const ContextRequest* request, RankingResult** results, size_t* numResults) {
    if (!g_state || !request || !results || !numResults) {
        return RAGGER_ERROR_INVALID_ARGUMENT;
    }

    // For this implementation, we'll create some dummy results
    // In a real implementation, this would score actual code blocks
    *numResults = 3;
    *results = new RankingResult[*numResults];

    for (size_t i = 0; i < *numResults; ++i) {
        (*results)[i].block = nullptr; // Would point to actual CodeBlock
        (*results)[i].score = 0.8f - (i * 0.1f); // Decreasing scores
        (*results)[i].rankingType = new char[5];
        strcpy(const_cast<char*>((*results)[i].rankingType), "bm25");
    }

    std::cout << "BM25RankerPlugin: Ranked " << *numResults << " blocks" << std::endl;
    return RAGGER_SUCCESS;
}

void ragger_ranker_free_results(RankingResult* results, size_t numResults) {
    if (!results) return;

    for (size_t i = 0; i < numResults; ++i) {
        delete[] results[i].rankingType;
    }
    delete[] results;
}

int ragger_ranker_update_weights(const float* newWeights, size_t numWeights) {
    if (!g_state || !newWeights || numWeights == 0) {
        return RAGGER_ERROR_INVALID_ARGUMENT;
    }

    // Update weights (this is a simple implementation)
    g_state->defaultWeight = newWeights[0];
    std::cout << "BM25RankerPlugin: Updated weights" << std::endl;
    return RAGGER_SUCCESS;
}

int ragger_ranker_train_on_feedback(const ContextRequest* request, const ContextResponse* response) {
    if (!g_state || !request || !response) {
        return RAGGER_ERROR_INVALID_ARGUMENT;
    }

    // Learning-to-rank implementation would go here
    // For now, just log the feedback
    std::cout << "BM25RankerPlugin: Received training feedback" << std::endl;
    return RAGGER_SUCCESS;
}

} // extern "C"
