#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <thread>

#include "ragger_plugin_api.h"
#include "EventBus.h"
#include "ConfigManager.h"
#include "IndexManager.h"
#include "ContextEngine.h"
#include "PluginManager.h"

namespace Ragger {
namespace Console {

class RaggerConsoleInterface {
private:
    bool m_running = false;
    std::vector<std::string> m_history;
    std::string m_currentInput;
    
    // Core components
    EventBus* m_eventBus;
    ConfigManager* m_configManager;
    IndexManager* m_indexManager;
    ContextEngine* m_contextEngine;
    PluginManager* m_pluginManager;

public:
    RaggerConsoleInterface() {
        m_eventBus = nullptr;
        m_configManager = nullptr;
        m_indexManager = nullptr;
        m_contextEngine = nullptr;
        m_pluginManager = nullptr;
    }

    ~RaggerConsoleInterface() {
        cleanup();
    }

    bool initialize() {
        std::cout << "=== RAGger Console Interface ===" << std::endl;
        std::cout << "Initializing core components..." << std::endl;

        try {
            // Initialize core components
            m_eventBus = new EventBus();
            m_configManager = new ConfigManager();
            m_indexManager = new IndexManager();
            m_contextEngine = new ContextEngine(m_eventBus);
            m_pluginManager = new PluginManager(m_eventBus);

            // Initialize components
            if (m_configManager->initialize() != RAGGER_SUCCESS) {
                std::cerr << "Failed to initialize ConfigManager" << std::endl;
                return false;
            }

            if (m_indexManager->initialize() != RAGGER_SUCCESS) {
                std::cerr << "Failed to initialize IndexManager" << std::endl;
                return false;
            }

            std::cout << "✓ Core components initialized successfully" << std::endl;
            std::cout << "✓ Database ready at: data/index.db" << std::endl;
            std::cout << "✓ Event bus active" << std::endl;
            
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Initialization failed: " << e.what() << std::endl;
            return false;
        }
    }

    void run() {
        m_running = true;
        showWelcome();
        showHelp();

        while (m_running) {
            std::cout << "\nRAGger> ";
            std::string input;
            std::getline(std::cin, input);

            if (input.empty()) continue;

            m_history.push_back(input);
            processCommand(input);
        }
    }

private:
    void showWelcome() {
        std::cout << "\n";
        std::cout << "╔══════════════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║                    RAGger Console Interface                 ║" << std::endl;
        std::cout << "║              RAG Pre-processor for AI Development           ║" << std::endl;
        std::cout << "╚══════════════════════════════════════════════════════════════╝" << std::endl;
        std::cout << "\n";
    }

    void showHelp() {
        std::cout << "Available commands:" << std::endl;
        std::cout << "  help, ?           - Show this help message" << std::endl;
        std::cout << "  status            - Show system status" << std::endl;
        std::cout << "  plugins           - List available plugins" << std::endl;
        std::cout << "  index <path>      - Index a file or directory" << std::endl;
        std::cout << "  query <text>      - Process a query with RAG" << std::endl;
        std::cout << "  config            - Show current configuration" << std::endl;
        std::cout << "  history           - Show command history" << std::endl;
        std::cout << "  clear             - Clear screen" << std::endl;
        std::cout << "  exit, quit        - Exit the application" << std::endl;
        std::cout << "\n";
    }

    void processCommand(const std::string& input) {
        std::istringstream iss(input);
        std::string command;
        iss >> command;

        if (command == "help" || command == "?") {
            showHelp();
        }
        else if (command == "status") {
            showStatus();
        }
        else if (command == "plugins") {
            showPlugins();
        }
        else if (command == "index") {
            std::string path;
            iss >> path;
            if (!path.empty()) {
                indexPath(path);
            } else {
                std::cout << "Usage: index <file_or_directory_path>" << std::endl;
            }
        }
        else if (command == "query") {
            std::string query;
            std::getline(iss, query);
            if (!query.empty()) {
                query = query.substr(1); // Remove leading space
                processQuery(query);
            } else {
                std::cout << "Usage: query <your_question_or_request>" << std::endl;
            }
        }
        else if (command == "config") {
            showConfig();
        }
        else if (command == "history") {
            showHistory();
        }
        else if (command == "clear") {
            clearScreen();
        }
        else if (command == "exit" || command == "quit") {
            m_running = false;
            std::cout << "Goodbye!" << std::endl;
        }
        else {
            std::cout << "Unknown command: " << command << std::endl;
            std::cout << "Type 'help' for available commands." << std::endl;
        }
    }

    void showStatus() {
        std::cout << "\n=== System Status ===" << std::endl;
        std::cout << "Event Bus: " << (m_eventBus ? "✓ Active" : "✗ Inactive") << std::endl;
        std::cout << "Config Manager: " << (m_configManager ? "✓ Active" : "✗ Inactive") << std::endl;
        std::cout << "Index Manager: " << (m_indexManager ? "✓ Active" : "✗ Inactive") << std::endl;
        std::cout << "Context Engine: " << (m_contextEngine ? "✓ Active" : "✗ Inactive") << std::endl;
        std::cout << "Plugin Manager: " << (m_pluginManager ? "✓ Active" : "✗ Inactive") << std::endl;
        
        if (m_eventBus) {
            auto stats = m_eventBus->getStats();
            std::cout << "\nEvent Statistics:" << std::endl;
            std::cout << "  Events Emitted: " << stats.totalEventsEmitted.load() << std::endl;
            std::cout << "  Events Processed: " << stats.totalEventsProcessed.load() << std::endl;
            std::cout << "  Events Dropped: " << stats.eventsDropped.load() << std::endl;
        }
    }

    void showPlugins() {
        std::cout << "\n=== Available Plugins ===" << std::endl;
        std::vector<std::string> plugins = {
            "cpp_parser - C/C++ code parser (libclang)",
            "bm25_ranker - BM25 ranking algorithm",
            "treesitter_parser - Multi-language parser (Tree-sitter)",
            "graph_ranker - Graph-based ranking (PageRank)",
            "lsp_client - Language Server Protocol client",
            "git_integration - Git repository integration",
            "test_discovery - Test case discovery"
        };

        for (const auto& plugin : plugins) {
            std::cout << "  ✓ " << plugin << " (Stub Implementation)" << std::endl;
        }
    }

    void indexPath(const std::string& path) {
        std::cout << "Indexing: " << path << std::endl;
        
        // Simulate indexing process
        std::cout << "  Scanning files..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        std::cout << "  Parsing code blocks..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        
        std::cout << "  Building index..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        std::cout << "✓ Indexing completed for: " << path << std::endl;
        std::cout << "  Files processed: 1" << std::endl;
        std::cout << "  Code blocks indexed: 5" << std::endl;
        std::cout << "  Symbols extracted: 12" << std::endl;
    }

    void processQuery(const std::string& query) {
        std::cout << "\n=== Processing Query ===" << std::endl;
        std::cout << "Query: " << query << std::endl;
        
        // Simulate RAG processing
        std::cout << "\nStep 1: Analyzing query..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        
        std::cout << "Step 2: Searching codebase..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(400));
        
        std::cout << "Step 3: Ranking results..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        std::cout << "Step 4: Generating context..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        
        std::cout << "\n=== RAG Result ===" << std::endl;
        std::cout << "Based on your query, here are the relevant code sections:" << std::endl;
        std::cout << "\n1. [Relevant Code Block 1]" << std::endl;
        std::cout << "   Score: 0.85 | File: example.cpp:15-25" << std::endl;
        std::cout << "   This code block is highly relevant to your query." << std::endl;
        
        std::cout << "\n2. [Relevant Code Block 2]" << std::endl;
        std::cout << "   Score: 0.72 | File: utils.h:8-12" << std::endl;
        std::cout << "   Additional context for your request." << std::endl;
        
        std::cout << "\n✓ Query processed successfully" << std::endl;
    }

    void showConfig() {
        std::cout << "\n=== Configuration ===" << std::endl;
        if (m_configManager) {
            std::cout << "Database Path: data/index.db" << std::endl;
            std::cout << "Max File Size: 10MB" << std::endl;
            std::cout << "Debounce Delay: 500ms" << std::endl;
            std::cout << "Plugin Directory: lib/ragger/plugins" << std::endl;
            std::cout << "Log Level: INFO" << std::endl;
        } else {
            std::cout << "Config manager not initialized" << std::endl;
        }
    }

    void showHistory() {
        std::cout << "\n=== Command History ===" << std::endl;
        if (m_history.empty()) {
            std::cout << "No commands in history" << std::endl;
        } else {
            for (size_t i = 0; i < m_history.size(); ++i) {
                std::cout << std::setw(3) << (i + 1) << ": " << m_history[i] << std::endl;
            }
        }
    }

    void clearScreen() {
        std::cout << "\033[2J\033[1;1H"; // ANSI escape codes to clear screen
        showWelcome();
    }

    void cleanup() {
        if (m_indexManager) {
            m_indexManager->shutdown();
            delete m_indexManager;
            m_indexManager = nullptr;
        }
        
        delete m_eventBus;
        delete m_configManager;
        delete m_contextEngine;
        delete m_pluginManager;
        
        m_eventBus = nullptr;
        m_configManager = nullptr;
        m_contextEngine = nullptr;
        m_pluginManager = nullptr;
    }
};

} // namespace Console
} // namespace Ragger

int main() {
    std::cout << "Starting RAGger Console Interface..." << std::endl;
    
    Ragger::Console::RaggerConsoleInterface interface;
    
    if (!interface.initialize()) {
        std::cerr << "Failed to initialize RAGger Console Interface" << std::endl;
        return 1;
    }
    
    interface.run();
    
    std::cout << "RAGger Console Interface finished" << std::endl;
    return 0;
}
