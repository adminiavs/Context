#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <map>
#include "ragger_plugin_api.h"

// Forward declarations for Dear ImGui types (stubs for now)
struct ImVec4 {
    float x, y, z, w;
    ImVec4(float x = 0.0f, float y = 0.0f, float z = 0.0f, float w = 1.0f) : x(x), y(y), z(z), w(w) {}
};

namespace Ragger {
namespace GUI {

// Forward declarations
class CodeEditor;
class PromptPanel;
class SyntaxHighlighter;
class SplitView;

// Token types for syntax highlighting
enum class TokenType {
    Keyword,
    String,
    Comment,
    Number,
    Identifier,
    Operator,
    Punctuation,
    Default
};

// Highlight token structure
struct HighlightToken {
    std::string text;
    int startPos;
    int endPos;
    TokenType type;
    ImVec4 color;
    
    HighlightToken() : startPos(0), endPos(0), type(TokenType::Default) {}
    HighlightToken(const std::string& t, int start, int end, TokenType ty, const ImVec4& col = ImVec4())
        : text(t), startPos(start), endPos(end), type(ty), color(col) {}
};

// Color scheme for syntax highlighting
struct ColorScheme {
    ImVec4 keyword;
    ImVec4 string;
    ImVec4 comment;
    ImVec4 number;
    ImVec4 identifier;
    ImVec4 operator_;
    ImVec4 punctuation;
    ImVec4 default_;
    
    ColorScheme();
};

// GUI Application class
class GUIApplication {
public:
    GUIApplication();
    ~GUIApplication();

    // Initialize the GUI system
    bool initialize();
    
    // Main event loop
    void run();
    
    // Cleanup resources
    void shutdown();

    // Load a file into the editor
    void loadFile(const std::string& filePath);
    
    // Set the prompt content
    void setPromptContent(const std::string& content);
    
    // Get the current prompt content
    std::string getPromptContent() const;
    
    // Register a callback for when code blocks are selected
    void setCodeSelectionCallback(std::function<void(const CodeBlock&)> callback);
    
    // Register a callback for when prompt is generated
    void setPromptGeneratedCallback(std::function<void(const std::string&)> callback);

private:
    bool m_initialized;
    std::unique_ptr<SplitView> m_splitView;
    std::unique_ptr<CodeEditor> m_codeEditor;
    std::unique_ptr<PromptPanel> m_promptPanel;
    std::unique_ptr<SyntaxHighlighter> m_syntaxHighlighter;
    
    // Callbacks
    std::function<void(const CodeBlock&)> m_codeSelectionCallback;
    std::function<void(const std::string&)> m_promptGeneratedCallback;
    
    // GUI state
    bool m_showDemoWindow;
    bool m_showAboutWindow;
    
    void renderMenuBar();
    void renderStatusBar();
    void handleKeyboardShortcuts();
};

// Code Editor with syntax highlighting
class CodeEditor {
public:
    CodeEditor();
    ~CodeEditor();

    // Load file content
    void loadContent(const std::string& content, const std::string& language);
    
    // Get current content
    std::string getContent() const;
    
    // Render the editor
    void render(float width, float height);
    
    // Get selected text
    std::string getSelectedText() const;
    
    // Set selection callback
    void setSelectionCallback(std::function<void(const std::string&, int startLine, int endLine)> callback);

private:
    std::string m_content;
    std::string m_language;
    std::string m_selectedText;
    int m_selectionStartLine;
    int m_selectionEndLine;
    bool m_textChanged;
    
    std::function<void(const std::string&, int startLine, int endLine)> m_selectionCallback;
    
    void renderTextWithHighlighting();
    void applySyntaxHighlighting(const std::string& text);
    std::vector<std::string> splitLines(const std::string& text);
};

// Prompt Panel for displaying and editing prompts
class PromptPanel {
public:
    PromptPanel();
    ~PromptPanel();

    // Set prompt content
    void setContent(const std::string& content);
    
    // Get current content
    std::string getContent() const;
    
    // Render the panel
    void render(float width, float height);
    
    // Show token count
    void showTokenCount(bool show);
    
    // Show raw prompt preview
    void showRawPreview(bool show);

private:
    std::string m_content;
    bool m_showTokenCount;
    bool m_showRawPreview;
    bool m_contentChanged;
    
    void renderTokenCount();
    void renderRawPreview();
    int estimateTokenCount(const std::string& text);
};

// Syntax Highlighter using Tree-sitter
class SyntaxHighlighter {
public:
    SyntaxHighlighter();
    ~SyntaxHighlighter();

    // Set language
    void setLanguage(const std::string& language);
    
    // Highlight text
    std::vector<HighlightToken> highlight(const std::string& text);

private:
    std::string m_currentLanguage;
    std::map<std::string, std::vector<HighlightToken>> m_cache;
    
    HighlightToken createToken(const std::string& text, int start, int end, TokenType type);
};

// Split View for code and prompt panels
class SplitView {
public:
    SplitView();
    ~SplitView();

    // Set the child components
    void setCodeEditor(std::unique_ptr<CodeEditor> editor);
    void setPromptPanel(std::unique_ptr<PromptPanel> panel);
    
    // Render the split view
    void render(float width, float height);
    
    // Set split ratio (0.0 to 1.0)
    void setSplitRatio(float ratio);
    
    // Get current split ratio
    float getSplitRatio() const;

private:
    std::unique_ptr<CodeEditor> m_codeEditor;
    std::unique_ptr<PromptPanel> m_promptPanel;
    float m_splitRatio;
    bool m_dragging;
    
    void renderSplitter();
    void handleDrag();
};


} // namespace GUI
} // namespace Ragger
