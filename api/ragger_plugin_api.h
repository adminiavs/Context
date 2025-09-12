#ifndef RAGGER_PLUGIN_API_H
#define RAGGER_PLUGIN_API_H

#include <cstdint>
#include <cstddef>

// ABI Version for compatibility checking
#define RAGGER_PLUGIN_ABI_VERSION 100

// Forward declarations for ABI stability
typedef void* RaggerCoreHandle;
typedef void* PluginHandle;
typedef void* EventHandle;

// Core data structures
struct CodeBlock {
    const char* name;
    const char* content;
    const char* filePath;
    int startLine;
    int endLine;
    const char* language;
    uint32_t visibility; // 0=private, 1=protected, 2=public
    void* metadata; // Plugin-specific data
};

struct CodeSymbol {
    const char* name;
    const char* type; // "function", "class", "variable", etc.
    const char* signature;
    CodeBlock* definition;
};

struct RankingResult {
    CodeBlock* block;
    float score;
    const char* rankingType; // "bm25", "graph", "hybrid"
};

struct ContextRequest {
    CodeBlock* primaryBlock;
    const char* userQuery;
    uint32_t maxTokens;
    float* rankingWeights; // Array of weights for different ranking types
    size_t numWeights;
};

struct ContextResponse {
    RankingResult* results;
    size_t numResults;
    uint32_t totalTokens;
    const char* promptTemplate;
};

// Event system
enum EventType {
    EVENT_INDEXING_STARTED = 1,
    EVENT_INDEXING_COMPLETED = 2,
    EVENT_FILE_PARSED = 3,
    EVENT_CODEBLOCK_INDEXED = 4,
    EVENT_RANKING_COMPLETED = 5,
    EVENT_CONTEXT_GENERATED = 6,
    EVENT_LLM_CHUNK_RECEIVED = 7,
    EVENT_PLUGIN_ERROR = 8
};

struct EventData {
    EventType type;
    uint64_t timestamp;
    const char* sourcePlugin;
    void* data;
    size_t dataSize;
};

typedef void (*EventCallback)(const EventData* event, void* userData);

// Plugin lifecycle functions (all plugins must implement these)
extern "C" {

    // Core lifecycle
    int plugin_get_abi_version();
    const char* plugin_get_name();
    const char* plugin_get_version();
    const char* plugin_get_description();

    // Initialization and shutdown
    int plugin_initialize(RaggerCoreHandle core);
    void plugin_shutdown();

    // Event system (optional, plugins can choose which events to handle)
    void plugin_register_events(EventCallback callback, void* userData);
    void plugin_unregister_events();

    // Plugin-specific capabilities (optional)
    const char* plugin_get_capabilities(); // JSON string describing capabilities
}

// Plugin type identification
enum PluginType {
    PLUGIN_TYPE_PARSER = 1,
    PLUGIN_TYPE_RANKER = 2,
    PLUGIN_TYPE_CONTEXT = 3,
    PLUGIN_TYPE_LLM_BACKEND = 4,
    PLUGIN_TYPE_LSP_CLIENT = 5,
    PLUGIN_TYPE_GIT_INTEGRATION = 6,
    PLUGIN_TYPE_UI_COMPONENT = 7
};

// Parser Plugin Interface
struct ParserPluginAPI {
    // Capabilities
    const char** (*get_supported_languages)(size_t* count);
    bool (*supports_file)(const char* filePath);

    // Parsing
    int (*parse_file)(const char* filePath, CodeBlock** blocks, size_t* numBlocks);
    void (*free_blocks)(CodeBlock* blocks, size_t numBlocks);

    // Symbol extraction
    int (*extract_symbols)(const char* filePath, CodeSymbol** symbols, size_t* numSymbols);
    void (*free_symbols)(CodeSymbol* symbols, size_t numSymbols);
};

// Ranker Plugin Interface
struct RankerPluginAPI {
    // Capabilities
    const char* (*get_ranking_type)();
    float (*get_default_weight)();

    // Ranking
    int (*rank_blocks)(const ContextRequest* request, RankingResult** results, size_t* numResults);
    void (*free_results)(RankingResult* results, size_t numResults);

    // Learning (optional)
    int (*update_weights)(const float* newWeights, size_t numWeights);
    int (*train_on_feedback)(const ContextRequest* request, const ContextResponse* response);
};

// Context Plugin Interface
struct ContextPluginAPI {
    // Context generation
    int (*generate_context)(const ContextRequest* request, ContextResponse** response);
    void (*free_response)(ContextResponse* response);

    // Template management
    int (*load_template)(const char* templateName, const char* templateContent);
    int (*get_available_templates)(const char*** templates, size_t* numTemplates);
    void (*free_templates)(const char** templates, size_t numTemplates);
};

// LLM Backend Plugin Interface
struct LLMBackendPluginAPI {
    // Capabilities
    const char** (*get_supported_models)(size_t* count);
    bool (*supports_streaming)();
    uint32_t (*get_max_context_length)();

    // Model management
    int (*load_model)(const char* modelPath, const char* modelName);
    int (*unload_model)(const char* modelName);

    // Inference
    int (*generate)(const char* prompt, const char* modelName,
                   char** response, size_t* responseLength);
    int (*generate_streaming)(const char* prompt, const char* modelName,
                             void (*chunk_callback)(const char* chunk, void* userData),
                             void* userData);

    // Session management (optional)
    int (*create_session)(const char* sessionId, const char* modelName);
    int (*add_to_session)(const char* sessionId, const char* message, bool isUser);
    int (*get_session_context)(const char* sessionId, char** context);
    int (*destroy_session)(const char* sessionId);
};

// Plugin registration (implemented by plugins)
typedef struct {
    PluginType type;
    void* api; // Generic API pointer - cast based on type
} PluginInterface;

// Plugin discovery and loading
typedef int (*PluginLoadFunc)(PluginHandle* handle);
typedef void (*PluginUnloadFunc)(PluginHandle handle);
typedef int (*PluginGetInterfaceFunc)(PluginHandle handle, PluginInterface* interface);

// Core engine functions (provided to plugins)
struct RaggerCoreAPI {
    // Event system
    int (*emit_event)(const EventData* event);
    int (*subscribe_event)(EventType type, EventCallback callback, void* userData);
    int (*unsubscribe_event)(EventType type, EventCallback callback);

    // Logging
    void (*log_info)(const char* message);
    void (*log_warning)(const char* message);
    void (*log_error)(const char* message);

    // Configuration
    int (*get_config_string)(const char* key, const char** value);
    int (*get_config_int)(const char* key, int* value);
    int (*get_config_float)(const char* key, float* value);

    // Resource management
    void* (*allocate_memory)(size_t size);
    void (*free_memory)(void* ptr);

    // File system utilities
    int (*read_file)(const char* path, char** content, size_t* size);
    int (*write_file)(const char* path, const char* content, size_t size);
    int (*get_file_hash)(const char* path, char* hash, size_t hashSize);
};

// Error codes
#define RAGGER_SUCCESS 0
#define RAGGER_ERROR_INVALID_ARGUMENT -1
#define RAGGER_ERROR_OUT_OF_MEMORY -2
#define RAGGER_ERROR_FILE_NOT_FOUND -3
#define RAGGER_ERROR_PLUGIN_LOAD_FAILED -4
#define RAGGER_ERROR_PLUGIN_UNLOAD_FAILED -5
#define RAGGER_ERROR_ABI_VERSION_MISMATCH -6
#define RAGGER_ERROR_OPERATION_NOT_SUPPORTED -7
#define RAGGER_ERROR_PARSING_FAILED -8
#define RAGGER_ERROR_INDEXING_FAILED -9
#define RAGGER_ERROR_RANKING_FAILED -10
#define RAGGER_ERROR_CONTEXT_GENERATION_FAILED -11
#define RAGGER_ERROR_LLM_INFERENCE_FAILED -12

#endif // RAGGER_PLUGIN_API_H
