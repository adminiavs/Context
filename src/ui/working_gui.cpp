#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

// Dear ImGui headers
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

namespace Ragger {
namespace GUI {

class WorkingGUIApplication {
private:
    GLFWwindow* m_window = nullptr;
    bool m_running = false;
    std::string m_inputText = "";
    std::string m_outputText = "";
    std::vector<std::string> m_logMessages;
    bool m_showDemo = false;
    bool m_showAbout = false;
    
    // RAGger simulation data
    std::string m_status = "Ready";
    int m_filesIndexed = 0;
    int m_codeBlocks = 0;
    int m_queriesProcessed = 0;

public:
    WorkingGUIApplication() {
        m_logMessages.push_back("RAGger GUI Application started");
        m_logMessages.push_back("Dear ImGui framework loaded");
        m_logMessages.push_back("Ready for user interaction");
    }

    ~WorkingGUIApplication() {
        cleanup();
    }

    bool initialize() {
        // Initialize GLFW
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            return false;
        }

        // Configure GLFW
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        // Create window
        m_window = glfwCreateWindow(1400, 900, "RAGger - RAG Pre-processor", nullptr, nullptr);
        if (!m_window) {
            std::cerr << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return false;
        }

        glfwMakeContextCurrent(m_window);
        glfwSwapInterval(1); // Enable vsync

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(m_window, true);
        ImGui_ImplOpenGL3_Init("#version 130");

        m_running = true;
        m_status = "GUI initialized successfully";

        return true;
    }

    void run() {
        while (!glfwWindowShouldClose(m_window) && m_running) {
            glfwPollEvents();

            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // Setup docking
            ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
            ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
            
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

            if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
                window_flags |= ImGuiWindowFlags_NoBackground;

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGui::Begin("DockSpace", nullptr, window_flags);
            ImGui::PopStyleVar();
            ImGui::PopStyleVar(2);

            // Submit the DockSpace
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

            // Menu bar
            if (ImGui::BeginMenuBar()) {
                if (ImGui::BeginMenu("File")) {
                    if (ImGui::MenuItem("New Query")) {
                        m_inputText.clear();
                        m_outputText.clear();
                        m_logMessages.push_back("New query started");
                    }
                    if (ImGui::MenuItem("Load File")) {
                        m_logMessages.push_back("File load dialog would open here");
                    }
                    if (ImGui::MenuItem("Save Results")) {
                        m_logMessages.push_back("Results saved");
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Exit")) {
                        m_running = false;
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Tools")) {
                    if (ImGui::MenuItem("Index Directory")) {
                        m_logMessages.push_back("Indexing directory...");
                        simulateIndexing();
                    }
                    if (ImGui::MenuItem("Plugin Manager")) {
                        m_logMessages.push_back("Plugin manager would open here");
                    }
                    if (ImGui::MenuItem("Configuration")) {
                        m_logMessages.push_back("Configuration dialog would open here");
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Help")) {
                    if (ImGui::MenuItem("About")) {
                        m_showAbout = true;
                    }
                    if (ImGui::MenuItem("Demo Window")) {
                        m_showDemo = !m_showDemo;
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }

            // Main content area
            renderMainContent();

            ImGui::End();

            // Demo window
            if (m_showDemo) {
                ImGui::ShowDemoWindow(&m_showDemo);
            }

            // About window
            if (m_showAbout) {
                renderAboutWindow();
            }

            // Rendering
            ImGui::Render();
            int display_w, display_h;
            glfwGetFramebufferSize(m_window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            // Update and Render additional Platform Windows
            if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                GLFWwindow* backup_current_context = glfwGetCurrentContext();
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
                glfwMakeContextCurrent(backup_current_context);
            }

            glfwSwapBuffers(m_window);
        }
    }

private:
    void renderMainContent() {
        // Left panel - Input and controls
        ImGui::Begin("Input Panel");
        
        ImGui::Text("Status: %s", m_status.c_str());
        ImGui::Separator();
        
        ImGui::Text("Enter your code or question:");
        ImGui::InputTextMultiline("##input", &m_inputText, ImVec2(-1, 300), 
                                 ImGuiInputTextFlags_AllowTabInput);
        
        if (ImGui::Button("Process with RAG", ImVec2(-1, 0))) {
            if (!m_inputText.empty()) {
                processQuery();
            }
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Clear", ImVec2(-1, 0))) {
            m_inputText.clear();
            m_outputText.clear();
            m_logMessages.push_back("Input cleared");
        }
        
        ImGui::Separator();
        ImGui::Text("Quick Actions:");
        if (ImGui::Button("Index Current Directory")) {
            simulateIndexing();
        }
        ImGui::SameLine();
        if (ImGui::Button("Show Plugins")) {
            m_logMessages.push_back("Available plugins: C++ Parser, BM25 Ranker, Tree-sitter Parser, Graph Ranker, LSP Client, Git Integration, Test Discovery");
        }
        
        ImGui::End();

        // Right panel - Output and results
        ImGui::Begin("Output Panel");
        
        ImGui::Text("RAG Results:");
        ImGui::Separator();
        
        if (!m_outputText.empty()) {
            ImGui::TextWrapped("%s", m_outputText.c_str());
        } else {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No results yet. Enter a query and click 'Process with RAG'.");
        }
        
        ImGui::End();

        // Bottom panel - Log and status
        ImGui::Begin("Activity Log");
        
        ImGui::Text("System Activity:");
        ImGui::Separator();
        
        ImGui::BeginChild("LogContent", ImVec2(0, 200), true);
        for (const auto& msg : m_logMessages) {
            ImGui::Text("%s", msg.c_str());
        }
        ImGui::EndChild();
        
        ImGui::End();

        // Status panel
        ImGui::Begin("System Status");
        
        ImGui::Text("RAGger System Status:");
        ImGui::Separator();
        
        ImGui::Text("Files Indexed: %d", m_filesIndexed);
        ImGui::Text("Code Blocks: %d", m_codeBlocks);
        ImGui::Text("Queries Processed: %d", m_queriesProcessed);
        
        ImGui::Separator();
        ImGui::Text("Plugin Status:");
        ImGui::BulletText("C++ Parser: Available (Stub)");
        ImGui::BulletText("BM25 Ranker: Available (Stub)");
        ImGui::BulletText("Tree-sitter Parser: Available (Stub)");
        ImGui::BulletText("Graph Ranker: Available (Stub)");
        ImGui::BulletText("LSP Client: Available (Stub)");
        ImGui::BulletText("Git Integration: Available (Stub)");
        ImGui::BulletText("Test Discovery: Available (Stub)");
        
        ImGui::End();
    }

    void renderAboutWindow() {
        if (ImGui::Begin("About RAGger", &m_showAbout)) {
            ImGui::Text("RAGger - RAG Pre-processor");
            ImGui::Text("Version 1.0.0");
            ImGui::Separator();
            ImGui::Text("A powerful RAG (Retrieval-Augmented Generation) pre-processor");
            ImGui::Text("designed for AI-assisted software development.");
            ImGui::Separator();
            ImGui::Text("Features:");
            ImGui::BulletText("Code indexing and parsing");
            ImGui::BulletText("BM25 and graph-based ranking");
            ImGui::BulletText("Plugin architecture");
            ImGui::BulletText("Multi-language support");
            ImGui::BulletText("Git integration");
            ImGui::Separator();
            ImGui::Text("Built with:");
            ImGui::BulletText("C++17");
            ImGui::BulletText("Dear ImGui");
            ImGui::BulletText("SQLite3");
            ImGui::BulletText("OpenGL");
            ImGui::Separator();
            if (ImGui::Button("Close")) {
                m_showAbout = false;
            }
        }
        ImGui::End();
    }

    void processQuery() {
        m_logMessages.push_back("Processing query: " + m_inputText.substr(0, 50) + "...");
        m_status = "Processing...";
        
        // Simulate processing
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        m_outputText = "=== RAG Processing Results ===\n\n";
        m_outputText += "Query: " + m_inputText + "\n\n";
        m_outputText += "Relevant Code Blocks Found:\n";
        m_outputText += "1. [Code Block 1] - Score: 0.85\n";
        m_outputText += "   File: example.cpp:15-25\n";
        m_outputText += "   This code block is highly relevant to your query.\n\n";
        m_outputText += "2. [Code Block 2] - Score: 0.72\n";
        m_outputText += "   File: utils.h:8-12\n";
        m_outputText += "   Additional context for your request.\n\n";
        m_outputText += "Generated Prompt:\n";
        m_outputText += "Context: The following code blocks are relevant...\n";
        m_outputText += "Question: " + m_inputText + "\n";
        m_outputText += "\n✓ Ready for AI model input";
        
        m_queriesProcessed++;
        m_logMessages.push_back("Query processed successfully");
        m_status = "Ready";
    }

    void simulateIndexing() {
        m_logMessages.push_back("Starting indexing process...");
        m_status = "Indexing...";
        
        // Simulate indexing
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        
        m_filesIndexed += 5;
        m_codeBlocks += 25;
        m_logMessages.push_back("Indexing completed: 5 files, 25 code blocks");
        m_status = "Ready";
    }

    void cleanup() {
        if (m_window) {
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();

            glfwDestroyWindow(m_window);
            glfwTerminate();
            m_window = nullptr;
        }
    }
};

} // namespace GUI
} // namespace Ragger

int main() {
    std::cout << "Starting RAGger Working GUI Application..." << std::endl;
    
    Ragger::GUI::WorkingGUIApplication app;
    
    if (!app.initialize()) {
        std::cerr << "Failed to initialize RAGger GUI Application" << std::endl;
        return 1;
    }
    
    app.run();
    
    std::cout << "RAGger GUI Application finished" << std::endl;
    return 0;
}
