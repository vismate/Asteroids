#ifndef GRAPHICS_HPP
#define GRAPHICS_HPP

#include "Error.hpp"
#include "Event.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <GL/glew.h>

#include <functional>
#include <unordered_map>
#include <fstream>
#include <initializer_list>
#include <memory>

#define WIN_EVT_CALLBACK(winptr) *static_cast<EventCallbackFn *>(glfwGetWindowUserPointer(winptr))

// TODO: GL error handling

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

                glfwSetErrorCallback([](int code, const char *desc)
                                     { Log::error(Log::format("GLFW error(%u): %s", code, desc)); });

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
            // In case one would call set_fullscreen(false) before set_fullscreen(true) first
            static int x{100}, y{100}, width{640}, height(480);

            if (val)
            {
                glfwGetWindowSize(window_handle, &width, &height);
                glfwGetWindowPos(window_handle, &x, &y);

                const auto monitor = glfwGetPrimaryMonitor();
                const auto mode = glfwGetVideoMode(monitor);
                glfwSetWindowMonitor(window_handle, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            }
            else
            {
                glfwSetWindowMonitor(window_handle, nullptr, x, y, width, height, GLFW_DONT_CARE);
            }

            return *this;
        }

        inline auto get_native_handle() -> GLFWwindow *
        {
            return window_handle;
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
                callback(Event::WindowResize(width, height)); });

            glfwSetWindowCloseCallback(window_handle, [](GLFWwindow *window) -> void
                                       {
                auto &callback = WIN_EVT_CALLBACK(window);
                callback(Event::WindowClose()); });

            glfwSetKeyCallback(window_handle, [](GLFWwindow *window, int key, int, int action, int) -> void
                               {
                auto &callback = WIN_EVT_CALLBACK(window);
                switch (action)
                {
                case GLFW_PRESS:
                    callback(Event::KeyPressed(static_cast<Input::Key>(key), false));
                    break;
                case GLFW_RELEASE:
                    callback(Event::KeyReleased(static_cast<Input::Key>(key)));
                    break;
                case GLFW_REPEAT:
                    callback(Event::KeyPressed(static_cast<Input::Key>(key), true));
                    break;                
                } });

            glfwSetMouseButtonCallback(window_handle, [](GLFWwindow *window, int button, int action, int) -> void
                                       {
                auto &callback = WIN_EVT_CALLBACK(window);
                switch (action)
                {
                case GLFW_PRESS:
                    callback(Event::MouseButtonPressed(static_cast<Input::Mouse>(button)));
                    break;
                case GLFW_RELEASE:
                    callback(Event::MouseButtonReleased(static_cast<Input::Mouse>(button)));
                    break;            
                } });

            glfwSetScrollCallback(window_handle, [](GLFWwindow *window, double x_offset, double y_offset) -> void
                                  {
                auto &callback = WIN_EVT_CALLBACK(window);
                callback(Event::MouseScrolled(x_offset, y_offset)); });

            glfwSetCursorPosCallback(window_handle, [](GLFWwindow *window, double x, double y) -> void
                                     {
                auto &callback = WIN_EVT_CALLBACK(window);
                callback(Event::MouseMoved(x, y)); });

            glfwSetWindowFocusCallback(window_handle, [](GLFWwindow *window, int focused) -> void
                                       {
                                           auto &callback = WIN_EVT_CALLBACK(window);

                                           if (focused)
                                           {
                                               callback(Event::WindowFocus());
                                           }
                                           else
                                           {
                                               callback(Event::WindowLostFocus());
                                           } });

            glfwSetWindowPosCallback(window_handle, [](GLFWwindow *window, int x, int y) -> void
                                     {
                auto &callback = WIN_EVT_CALLBACK(window);
                callback(Event::WindowMoved(x, y)); });

            glfwSetWindowRefreshCallback(window_handle, [](GLFWwindow *window) -> void
                                         {
                auto &callback = WIN_EVT_CALLBACK(window);
                callback(Event::WindowRedraw()); });

            glfwSetFramebufferSizeCallback(window_handle, [](GLFWwindow *, int width, int height) -> void {glViewport(0, 0, width, height);});
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

    enum class GLtype
    {
        None = -1,
        Float = GL_FLOAT,
        Float2 = GL_FLOAT_VEC2,
        Float3 = GL_FLOAT_VEC3,
        Float4 = GL_FLOAT_VEC4,
        UnsignedInt = GL_UNSIGNED_INT,
        Byte = GL_UNSIGNED_BYTE,
    };

    class IndexBuffer
    {
    public:
        IndexBuffer(const unsigned int *data, size_t count)
        {
            auto p = new unsigned int;
            glGenBuffers(1, p);
            id.reset(p);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *id);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * count, data, GL_STATIC_DRAW);
        }

        inline auto operator=(IndexBuffer &&other) -> IndexBuffer &
        {
            id = std::move(other.id);
            return *this;
        }

        ~IndexBuffer()
        {
            if (id)
            {
                glDeleteBuffers(1, id.get());
                id.release();
            }
        }

        inline auto bind() const -> void
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *id);
        }

        inline auto unbind() const -> void
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        }

    private:
        std::unique_ptr<unsigned int> id;
    };

    class VertexBuffer
    {
    public:
        struct LayoutElement
        {
            GLtype type;
            size_t count;
            bool normalized;

            inline auto size_of_type() const -> size_t
            {
                switch (type)
                {
                case GLtype::Float:
                    return sizeof(float);
                case GLtype::Float2:
                    return sizeof(float) * 2;
                case GLtype::Float3:
                    return sizeof(float) * 3;
                case GLtype::Float4:
                    return sizeof(float) * 4;
                case GLtype::UnsignedInt:
                    return sizeof(unsigned int);
                case GLtype::Byte:
                    return sizeof(unsigned char);
                default:
                    ASSERT(false, "None type");
                }
            }
        };

        struct Layout
        {
            size_t stride;
            std::vector<LayoutElement> elements;
        };

    public:
        VertexBuffer(const void *data, size_t size)
        {
            auto p = new unsigned int;
            glGenBuffers(1, p);
            id.reset(p);
            glBindBuffer(GL_ARRAY_BUFFER, *id);
            glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
        }

        inline auto operator=(VertexBuffer &&other) -> VertexBuffer &
        {
            id = std::move(other.id);
            layout = std::move(other.layout);
            return *this;
        }

        ~VertexBuffer()
        {
            if (id)
            {
                glDeleteBuffers(1, id.get());
                id.release();
            }
        }

        inline auto bind() const -> void
        {
            glBindBuffer(GL_ARRAY_BUFFER, *id);
        }

        inline auto unbind() const -> void
        {
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }

        inline auto set_layout(std::initializer_list<LayoutElement> elements) -> void
        {
            layout.elements.clear();
            layout.stride = 0;
            for (const auto &e : elements)
            {
                layout.elements.push_back(e);
                layout.stride += e.size_of_type() * e.count;
            }
        }

    private:
        std::unique_ptr<unsigned int> id;
        Layout layout;

        friend class VertexArray;
    };

    class VertexArray
    {
    public:
        VertexArray()
        {
            auto p = new unsigned int;
            glCreateVertexArrays(1, p);
            id.reset(p);
        }

        ~VertexArray()
        {
            if (id)
            {
                glDeleteVertexArrays(1, id.get());
                id.release();
            }
        }

        inline auto operator=(VertexArray &&other) -> VertexArray &
        {
            id = std::move(other.id);
            ib = std::move(other.ib);
            vbs = std::move(other.vbs);
            return *this;
        }

        inline auto bind() const -> void
        {
            glBindVertexArray(*id);
        }

        inline auto unbind() const -> void
        {
            glBindVertexArray(0);
        }

        inline auto add_vertex_buffer(const std::shared_ptr<VertexBuffer> &vb) -> void
        {
            bind();
            vb->bind();

            ASSERT(vb->layout.elements.size(), "VertexBuffer has no layout set");

            size_t offset{0};
            for (size_t i = 0; i < vb->layout.elements.size(); i++)
            {
                const auto &e = vb->layout.elements[i];
                glEnableVertexAttribArray(i);
                glVertexAttribPointer(i, e.count, static_cast<unsigned int>(e.type),
                                      e.normalized, vb->layout.stride, reinterpret_cast<const void *>(offset));
                offset += e.size_of_type() * e.count;
            }
        }

        inline auto set_index_buffer(const std::shared_ptr<IndexBuffer> &ib) -> void
        {
            bind();
            ib->bind();
            this->ib = ib;
        }

        inline auto get_vertex_buffers() const -> const std::vector<std::shared_ptr<VertexBuffer>> &
        {
            return vbs;
        }

        inline auto get_index_buffer() const -> const std::shared_ptr<IndexBuffer> &
        {
            return ib;
        }

    private:
        std::unique_ptr<unsigned int> id;
        std::shared_ptr<IndexBuffer> ib;
        std::vector<std::shared_ptr<VertexBuffer>> vbs;
    };

    /*
        template <>
        inline auto VertexArray::Layout::push<float>(size_t count) -> void
        {
            elements.emplace_back(GL_FLOAT, count, false);
            stride += sizeof(GLfloat) * count;
        }

        template <>
        inline auto VertexArray::Layout::push<unsigned int>(size_t count) -> void
        {
            elements.emplace_back(GL_UNSIGNED_INT, count, false);
            stride += sizeof(GLuint) * count;
        }

        template <>
        inline auto VertexArray::Layout::push<unsigned char>(size_t count) -> void
        {
            elements.emplace_back(GL_UNSIGNED_BYTE, count, true);
            stride += sizeof(GLbyte) * count;
        }
    */
    class Shader
    {
    public:
        Shader(const char *vertex_path, const char *fragment_path)
        {
            // Read sources
            std::ifstream vertex_f(vertex_path), fragment_f(fragment_path);

            ASSERT(vertex_f.is_open() && fragment_f.is_open(), "Source file failed to open");
            std::stringstream vertex_stream, fragment_stream;
            vertex_stream << vertex_f.rdbuf();
            fragment_stream << fragment_f.rdbuf();

            vertex_source = vertex_stream.str();
            fragment_source = fragment_stream.str();

            const char *vsptr = vertex_source.c_str();
            const char *fsptr = fragment_source.c_str();

            // Compile sources
            unsigned int vertex, fragment;

            int success;
            char info[512];

            vertex = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(vertex, 1, &vsptr, nullptr);
            glCompileShader(vertex);

            glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
            glGetShaderInfoLog(vertex, 512, NULL, info);
            ASSERT(success, info);

            fragment = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(fragment, 1, &fsptr, nullptr);
            glCompileShader(fragment);

            glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
            glGetShaderInfoLog(fragment, 512, NULL, info);
            ASSERT(success, info);

            // Create shader program
            id = glCreateProgram();
            glAttachShader(id, vertex);
            glAttachShader(id, fragment);
            glLinkProgram(id);

            glGetProgramiv(id, GL_LINK_STATUS, &success);
            glGetProgramInfoLog(id, 512, NULL, info);

            ASSERT(success, info);

            glDeleteShader(vertex);
            glDeleteShader(fragment);

            vertex_f.close();
            fragment_f.close();
        }

        template <typename T>
        inline auto set_uniform(const char *name, T value) -> void
        {
            ASSERT(false, "Type specialization not implemented");
        }

        inline auto bind() const -> void
        {
            glUseProgram(id);
        }

        inline auto unbind() const -> void
        {
            glUseProgram(0);
        }

    private:
        inline auto uniform_location(const char *name) -> int
        {
            const auto itr = uniform_cache.find(name);

            if (itr != uniform_cache.end())
            {
                return itr->second;
            }
            else
            {
                auto location = glGetUniformLocation(id, name);

                ASSERT(location != -1, Log::format("Uniform with name %s does not exist.", name));

                uniform_cache.emplace(name, location);

                return location;
            }
        }

        unsigned int id;
        std::string vertex_source, fragment_source;
        std::unordered_map<const char *, int> uniform_cache;
    };

    // Specializations of Shader::set_uniform;
    template <>
    inline auto Shader::set_uniform<float>(const char *name, float value) -> void
    {
        glUniform1f(uniform_location(name), value);
    }

}

#endif