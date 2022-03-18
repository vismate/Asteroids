#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "Event.hpp"
#include "Graphics.hpp"

namespace Application
{
    class AbstractApplication
    {
    public:
        AbstractApplication(const AbstractApplication &) = delete;
        AbstractApplication(AbstractApplication &&) = delete;

        inline auto operator=(const AbstractApplication &) = delete;
        inline auto operator=(AbstractApplication &&) = delete;

        AbstractApplication() : window("Application Window", 640, 480)
        {
            window.set_event_callback(std::bind(&AbstractApplication::on_event, this, std::placeholders::_1));
        };
        virtual ~AbstractApplication(){};

        inline auto on_event(const Event::AbstractEvent &event) -> bool
        {       

            if (event.type() == Event::Type::WindowClose)
            {
                running = false;
            }

            for (auto layer : layer_stack)
            {
                if (layer->on_event(event))
                {
                    return true;
                }
            }

            return false;
        }

        inline auto run() -> void
        {
            while (running)
            {
                window.on_update();
            }
        }

    protected:
        bool running{true};
        Graphics::Window window;
        Event::LayerStack layer_stack;
    };
}

#endif