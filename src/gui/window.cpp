#include "window.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include "../utils/config.hpp"

namespace gui {

    BaseWindow::BaseWindow() : m_isRunning(false), m_window(nullptr) {}
    BaseWindow::~BaseWindow() {
        if (m_isRunning) {
            destroy();
        }
    }

    void BaseWindow::init() {
        if(!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_FOCUSED, 1);
        glfwWindowHint(0x2000C, 1);

        // Load the saved window size and position
        auto& config = config::get();
        if (config.window_maximized)
            glfwWindowHint(0x20008, 1);

        m_window = glfwCreateWindow(config.window_w, config.window_h, "Oops! Something went wrong.", nullptr, nullptr);
        if (!m_window) {
            glfwTerminate();
            return;
        }

        if (!config.window_maximized)
            glfwSetWindowPos(reinterpret_cast<GLFWwindow *>(m_window), config.window_x, config.window_y);

        glfwMakeContextCurrent(reinterpret_cast<GLFWwindow*>(m_window));
        glfwSwapInterval(1);

        onInit();
    }

    void BaseWindow::run() {
        m_isRunning = true;

        auto* window = reinterpret_cast<GLFWwindow*>(m_window);
        while (!glfwWindowShouldClose(window) && m_isRunning) {
            glfwPollEvents();
            onDraw();
            glfwSwapBuffers(window);
        }

        // Save the window size and position
        int x, y, w, h;
        bool isMaximized = glfwGetWindowAttrib(window, 0x20008);
        auto& config = config::get();
        if (!isMaximized) {
            glfwGetWindowPos(window, &x, &y);
            glfwGetWindowSize(window, &w, &h);
            config.window_x = x;
            config.window_y = y;
            config.window_w = w;
            config.window_h = h;
        }
        config.window_maximized = isMaximized;
        config::save();

        destroy();
    }

    void BaseWindow::destroy() {
        onDestroy();
        glfwDestroyWindow(reinterpret_cast<GLFWwindow*>(m_window));
        glfwTerminate();
    }

    void ImGuiWindow::onInit() {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForOpenGL(reinterpret_cast<GLFWwindow*>(m_window), true);
        ImGui_ImplOpenGL3_Init("#version 150");

        auto& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.IniFilename = nullptr;

        m_initCallback();

        io.Fonts->Build();
    }

    void ImGuiWindow::onDraw() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

        m_drawCallback();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(reinterpret_cast<GLFWwindow*>(m_window), &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void ImGuiWindow::onDestroy() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

}