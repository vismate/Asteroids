#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "Event.hpp"
#include "Graphics.hpp"

namespace App
{
    struct Args
    {
        int argc{0};
        char **argv{nullptr};

        inline auto operator[](int ind) -> const char *
        {
            ASSERT(ind < argc, "Tried to overindex arguments");
            return argv[ind];
        }
    };

    class Application
    {

    public:
        Application(const Application &) = delete;
        Application(Application &&) = delete;

        inline auto operator=(const Application &) = delete;
        inline auto operator=(Application &&) = delete;

        Application(const char *name = "Application", Args args = Args{}) : window(name, 640, 480), args(args)
        {
            ASSERT(!instance, "Application instance already exists!");
            window.set_event_callback(std::bind(&Application::on_event, this, std::placeholders::_1));

            instance = this;
        };

        inline auto on_event(const Event::AbstractEvent &event) -> bool
        {

            if (event.type() == Event::Type::WindowClose)
            {
                running = false;
            }

            layer_stack.propegate_event(event);

            return false;
        }

        static inline auto get_instance() -> Application &
        {
            ASSERT(instance, "Tried to access instance before creating it");
            return *instance;
        }

        template <typename T>
        static inline auto get_instance_as() -> T &
        {
            static_assert(std::is_base_of_v<Application, T>, "T must be derived from App:Application");
            return *reinterpret_cast<T *>(instance);
        }

        inline auto get_window() -> Graphics::Window &
        {
            return window;
        }

        inline auto push_layer(Event::AbstractLayer *layer) -> void
        {
            layer_stack.push(layer);
        }

        inline auto pop_layer(Event::AbstractLayer *layer) -> void
        {
            layer_stack.pop(layer);
        }

        inline auto close() -> void
        {
            running = false;
        }

        inline auto run() -> void
        {
            while (running)
            {
                window.on_update();
            }
        }

    protected:
        Graphics::Window window;
        Args args;

    private:
        bool running{true};
        Event::LayerStack layer_stack;

        static Application *instance;
    };
    Application *Application::instance = nullptr;
}

#endif