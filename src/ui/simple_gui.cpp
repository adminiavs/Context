#include <iostream>
#include <string>
#include <vector>

#ifdef RAGGER_HAS_IMGUI
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#endif

#include "GUI.h"

namespace Ragger {
namespace GUI {

class SimpleGUIApplication {
private:
    bool m_running = false;
    std::string m_status = "RAGger GUI Ready";
    char m_inputText[4096] = "";
    std::vector<std::string> m_logMessages;

public:
    SimpleGUIApplication() {
        m_logMessages.push_back("RAGger GUI Application started");
        m_logMessages.push_back("Dear ImGui framework loaded");
        m_logMessages.push_back("Ready for user interaction");
    }

    bool initialize() {
#ifdef RAGGER_HAS_IMGUI
        // Initialize GLFW
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            return false;
        }

        // Create window
        GLFWwindow* window = glfwCreateWindow(1200, 800, "RAGger - RAG Pre-processor", nullptr, nullptr);
        if (!window) {
            std::cerr << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return false;
        }

        glfwMakeContextCurrent(window);
        glfwSwapInterval(1); // Enable vsync

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 130");

        m_running = true;
        m_status = "GUI initialized successfully";

        // Main loop
        while (!glfwWindowShouldClose(window) && m_running) {
            glfwPollEvents();

            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // Main window
            ImGui::Begin("RAGger - RAG Pre-processor", &m_running);
            
            ImGui::Text("Status: %s", m_status.c_str());
            ImGui::Separator();

            // Input section
            ImGui::Text("Enter your code or prompt:");
            ImGui::InputTextMultiline("##input", m_inputText, sizeof(m_inputText), ImVec2(-1, 200));
            
            if (ImGui::Button("Process with RAG")) {
                if (strlen(m_inputText) > 0) {
                    std::string inputStr(m_inputText);
                    m_logMessages.push_back("Processing: " + inputStr.substr(0, 50) + "...");
                    m_status = "Processing request...";
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Clear")) {
                m_inputText[0] = '\0';
                m_logMessages.push_back("Input cleared");
            }

            ImGui::Separator();

            // Log section
            ImGui::Text("Activity Log:");
            ImGui::BeginChild("Log", ImVec2(0, 300), true);
            for (const auto& msg : m_logMessages) {
                ImGui::Text("%s", msg.c_str());
            }
            ImGui::EndChild();

            // Plugin status
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

            // Rendering
            ImGui::Render();
            int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(window);
        }

        // Cleanup
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(window);
        glfwTerminate();

        return true;
#else
        std::cout << "Dear ImGui not available. Running in console mode." << std::endl;
        std::cout << "Available commands:" << std::endl;
        std::cout << "1. Process text input" << std::endl;
        std::cout << "2. List plugins" << std::endl;
        std::cout << "3. Exit" << std::endl;
        
        std::string input;
        while (true) {
            std::cout << "\nRAGger> ";
            std::getline(std::cin, input);
            
            if (input == "exit" || input == "quit") break;
            else if (input == "plugins") {
                std::cout << "Available plugins: C++ Parser, BM25 Ranker, Tree-sitter Parser, Graph Ranker, LSP Client, Git Integration, Test Discovery" << std::endl;
            }
            else if (!input.empty()) {
                std::cout << "Processing: " << input << std::endl;
                std::cout << "Result: [This would be the RAG-processed output]" << std::endl;
            }
        }
        
        return true;
#endif
    }

    void shutdown() {
        m_running = false;
        m_status = "Shutting down...";
    }
};

} // namespace GUI
} // namespace Ragger

int main() {
    std::cout << "Starting RAGger Simple GUI Application..." << std::endl;
    
    Ragger::GUI::SimpleGUIApplication app;
    
    if (app.initialize()) {
        std::cout << "GUI Application completed successfully" << std::endl;
    } else {
        std::cerr << "GUI Application failed to initialize" << std::endl;
        return 1;
    }
    
    std::cout << "RAGger Simple GUI Application finished" << std::endl;
    return 0;
}
