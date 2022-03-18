#ifndef GRAPHICS_HPP
#define GRAPHICS_HPP

#include "Error.hpp"
#include "Event.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <GL/glew.h>

#include <functional>

#define WIN_EVT_CALLBACK(winptr) *static_cast<EventCallbackFn *>(glfwGetWindowUserPointer(winptr))

namespace Graphics
{
    class Window
    {
    public:
        using EventCallbackFn = std::function<bool(const Event::AbstractEvent &)>;

        Window() : Window("Unnamed window", 640, 360)
        {
        }

        Window(const char *title, size_t width, size_t height)
        {

            if (not Window::INITIALIZED_DEPS)
            {
                [[maybe_unused]] const int glfw_success = glfwInit();
                ASSERT(glfw_success, "Could not initialize GLFW");

                glfwSetErrorCallback([](int code, const char *desc){Log::error(Log::format("GLFW error(%u): %s", code, desc));});

                Log::info(Log::format("Compiled against GLFW %i.%i.%i", GLFW_VERSION_MAJOR, GLFW_VERSION_MINOR, GLFW_VERSION_REVISION));
                Log::info(Log::format("Running against GLFW %s", glfwGetVersionString()));

                Window::INITIALIZED_DEPS = true;
            }

            window_handle = glfwCreateWindow((int)width, (int)height, title, nullptr, nullptr);
            ASSERT(window_handle, "Could not create window");

            init_glfw_event_callbacks();

            make_current();
            set_vsync(true);
            Window::WINDOWS_ALIVE++;
        }

        inline auto on_update() -> void
        {
            static double prev_time{0}, curr_time{0};
                        
            curr_time = glfwGetTime();
            const double dt = curr_time - prev_time;
            prev_time = curr_time;

            glfwPollEvents();
            event_callback(Event::AppTick(dt));
            glfwSwapBuffers(window_handle);
        }

        inline auto make_current() -> Window &
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

        inline auto set_vsync(bool value) -> Window &
        {
            glfwSwapInterval(static_cast<int>(value));
            return *this;
        }

        inline auto set_event_callback(const EventCallbackFn &event_callback) -> Window &
        {
            this->event_callback = event_callback;
            return *this;
        }

        inline auto set_size_constraints(size_t min_width, size_t min_height, size_t max_width, size_t max_height) -> Window &
        {
            glfwSetWindowSizeLimits(window_handle, min_width, min_height, max_width, max_height);
            return *this;
        }

        inline auto set_aspect_constraints(size_t width, size_t height) -> Window &
        {
            glfwSetWindowAspectRatio(window_handle, width, height);
            return *this;
        }

        inline auto set_icon(const std::vector<GLFWimage> &icons) -> Window &
        {
            glfwSetWindowIcon(window_handle, icons.size(), icons.data());
            return *this;
        }

        inline auto get_size() const -> std::pair<int, int>
        {
            int width, height;
            glfwGetWindowSize(window_handle, &width, &height);

            return std::make_pair(width, height);
        }

        inline auto set_fullscreen(bool val) -> Window &
        {
            //In case one would call set_fullscreen(false) before set_fullscreen(true) first
            static int x{100}, y{100}, width{640}, height(480);

            if(val)
            {
                glfwGetWindowSize(window_handle, &width, &height);
                glfwGetWindowPos(window_handle, &x, &y);
                
                const auto monitor =  glfwGetPrimaryMonitor();
                const auto mode = glfwGetVideoMode(monitor);
                glfwSetWindowMonitor(window_handle, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            }
            else 
            {
                glfwSetWindowMonitor(window_handle, nullptr, x, y, width, height, GLFW_DONT_CARE);
            }

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
        inline auto init_glfw_event_callbacks() -> void
        {
            // Set WindowUserPointer to our event callback function, to be able to access it elsewhere.
            glfwSetWindowUserPointer(window_handle, &event_callback);

            glfwSetWindowSizeCallback(window_handle, [](GLFWwindow *window, int width, int height) -> void
            {
                auto &callback = WIN_EVT_CALLBACK(window);
                callback(Event::WindowResize(width, height)); 
            });

            glfwSetWindowCloseCallback(window_handle, [](GLFWwindow *window) -> void
            {
                auto &callback = WIN_EVT_CALLBACK(window);
                callback(Event::WindowClose()); 
            });

            glfwSetKeyCallback(window_handle, [](GLFWwindow *window, int key, int, int action, int) -> void
            {
                auto &callback = WIN_EVT_CALLBACK(window);
                switch (action)
                {
                case GLFW_PRESS:
                    callback(Event::KeyPressed(key, false));
                    break;
                case GLFW_RELEASE:
                    callback(Event::KeyReleased(key));
                    break;
                case GLFW_REPEAT:
                    callback(Event::KeyPressed(key, true));
                    break;                
                } 
            });

            glfwSetMouseButtonCallback(window_handle, [](GLFWwindow *window, int button, int action, int) -> void
            {
                auto &callback = WIN_EVT_CALLBACK(window);
                switch (action)
                {
                case GLFW_PRESS:
                    callback(Event::MouseButtonPressed(button));
                    break;
                case GLFW_RELEASE:
                    callback(Event::MouseButtonReleased(button));
                    break;            
                } 
            });

            glfwSetScrollCallback(window_handle, [](GLFWwindow *window, double x_offset, double y_offset) -> void
            {
                auto &callback = WIN_EVT_CALLBACK(window);
                callback(Event::MouseScrolled(x_offset, y_offset));
            });

            glfwSetCursorPosCallback(window_handle, [](GLFWwindow *window, double x, double y) -> void
            {
                auto &callback = WIN_EVT_CALLBACK(window);
                callback(Event::MouseMoved(x, y));
            });

            glfwSetWindowFocusCallback(window_handle, [](GLFWwindow *window, int focused) -> void
            {
                auto &callback = WIN_EVT_CALLBACK(window);
                
                if(focused)
                {
                    callback(Event::WindowFocus()); 
                }
                else
                {
                    callback(Event::WindowLostFocus());
                }
               
            });

            glfwSetWindowPosCallback(window_handle, [](GLFWwindow *window, int x, int y) -> void
            {
                auto &callback = WIN_EVT_CALLBACK(window);
                callback(Event::WindowMoved(x, y));
            });

        }

    private:
        static bool INITIALIZED_DEPS;
        static size_t WINDOWS_ALIVE;

        GLFWwindow *window_handle;
        EventCallbackFn event_callback;
    };
    // Define Window's static members
    bool Window::INITIALIZED_DEPS = false;
    size_t Window::WINDOWS_ALIVE = 0;

    class Renderer
    {
    };

}

#endif