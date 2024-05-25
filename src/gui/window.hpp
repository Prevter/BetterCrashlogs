#include <utility>

#pragma once

namespace gui {

    /// @brief Base class for a window, which handles all GLFW initialization and destruction
    class BaseWindow {
    public:
        /// @brief Default constructor
        BaseWindow();
        /// @brief Default destructor
        virtual ~BaseWindow();

        /// @brief Initializes the GLFW window
        void init();
        /// @brief Starts the message loop
        void run();
        /// @brief Destroys the GLFW window
        void destroy();
        /// @brief Closes the window
        void close() { m_isRunning = false; }

    protected:
        /// @brief Called when the window is initialized
        virtual void onInit() = 0;
        /// @brief Called when the window is drawn every frame
        virtual void onDraw() = 0;
        /// @brief Called when the window is destroyed
        virtual void onDestroy() = 0;

        void* m_window;

    private:
        bool m_isRunning;
    };

    /// @brief A window that uses ImGui for rendering
    class ImGuiWindow : public BaseWindow {
    public:
        /// @brief Constructor
        /// @param initCallback Function to call when the window is initialized
        /// @param drawCallback Function to call when the window is drawn
        ImGuiWindow(std::function<void()> initCallback, std::function<void()> drawCallback)
            : m_initCallback(std::move(initCallback)), m_drawCallback(std::move(drawCallback)) {}

        /// @brief Reload the ImGui context
        void reload();

    protected:
        /// @brief Sets up ImGui context
        void onInit() override;
        /// @brief Renders ImGui tabs
        void onDraw() override;
        /// @brief Destroys ImGui context
        void onDestroy() override;

    private:
        std::function<void()> m_initCallback;
        std::function<void()> m_drawCallback;
        bool m_shouldReload = false;
    };

}