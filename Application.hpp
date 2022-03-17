#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "Event.hpp"
#include "Graphics.hpp"

namespace Application
{
    class AbstractApplication
    {
    public:
        AbstractApplication() : window("Application Window", 640, 480){};
        virtual ~AbstractApplication(){};

        inline auto run() -> void
        {
            while (running)
            {
                
            }
        }

    private:

        bool running{true};
        Graphics::Window window;
    };
}

#endif