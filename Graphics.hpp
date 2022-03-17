#ifndef GRAPHICS_HPP
#define GRAPHICS_HPP

#include "Error.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <GL/glew.h>

namespace Graphics
{
    class Window
    {
    public:
        Window() : Window("Unnamed window", 640, 360)
        {
        }

        Window(const char *title, size_t width, size_t height)
        {

            if (not Window::INITIALIZED_DEPS)
            {
                [[maybe_unused]] const int glfw_success = glfwInit();
                ASSERT(glfw_success, "Could not initialize GLFW");

                Log::info(Log::format("Compiled against GLFW %i.%i.%i", GLFW_VERSION_MAJOR, GLFW_VERSION_MINOR, GLFW_VERSION_REVISION));
                Log::info(Log::format("Running against GLFW %s", glfwGetVersionString()));

                Window::INITIALIZED_DEPS = true;
            }

            window_handle = glfwCreateWindow((int)width, (int)height, title, nullptr, nullptr);
            ASSERT(window_handle, "Could not create window");

            activate();
            vsync(true);
            Window::WINDOWS_ALIVE++;
        }

        inline auto on_update() -> Window &
        {

            return *this;
        }

        inline auto activate() -> Window &
        {
            glfwMakeContextCurrent(window_handle);
            [[maybe_unused]] const GLenum glew_success = glewInit();
            ASSERT(glew_success == GLEW_OK, "Could not (re)initialize GLEW on active context");

            return *this;
        }

        inline auto set_size(size_t width, size_t height) -> Window &
        {
            glfwSetWindowSize(window_handle, (int)width, (int)height);
            return *this;
        }

        inline auto set_title(const char *title) -> Window &
        {
            glfwSetWindowTitle(window_handle, title);
            return *this;
        }

        inline auto vsync(bool value) -> Window &
        {
            glfwSwapInterval(static_cast<int>(value));
            return *this;
        }

        ~Window()
        {
            glfwDestroyWindow(window_handle);
            if (--Window::WINDOWS_ALIVE == 0)
            {
                glfwTerminate();
                Window::INITIALIZED_DEPS = false;
            }
        }

    private:
        static bool INITIALIZED_DEPS;
        static size_t WINDOWS_ALIVE;

        GLFWwindow *window_handle;
    };
    // Define Window's static members
    bool Window::INITIALIZED_DEPS = false;
    size_t Window::WINDOWS_ALIVE = 0;

    class Renderer
    {
    };

}

#endif