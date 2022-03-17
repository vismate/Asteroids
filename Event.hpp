#ifndef EVENT_HPP
#define EVENT_HPP

#include "Error.hpp"
#include <vector>

namespace Event
{
    enum class Type
    {
        None = 0,
        WindowClose,
        WindowResize,
        WindowFocus,
        WindowLostFocus,
        WindowMoved,
        AppTick,
        KeyPressed,
        KeyReleased,
        MouseButtonPressed,
        MouseButtonReleased,
        MouseMoved,
        MouseScrolled

    };

    enum class Category
    {
        None = 0,
        Application = (1 << 0),
        Input = (1 << 1),
        Keyboard = (1 << 2),
        Mouse = (1 << 3),
        MouseButton = (1 << 4)
    };

    class AbstractEvent
    {
    public:
        virtual auto type() const -> Type = 0;
        virtual auto in_category(Category category) const -> bool = 0;
        virtual auto debug_string() const -> std::string { return "Undefined event"; };
    };

    inline auto operator|(Category a, Category b) -> size_t
    {
        return static_cast<size_t>(a) | static_cast<size_t>(b);
    }

    inline auto operator|(size_t a, Category b) -> size_t
    {
        return a | static_cast<size_t>(b);
    }

    inline auto operator|(Category a, size_t b) -> size_t
    {
        return static_cast<size_t>(a) | b;
    }
}

#endif