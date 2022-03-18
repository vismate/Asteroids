#ifndef EVENT_HPP
#define EVENT_HPP

#include "Error.hpp"
#include <vector>

#define EVT_IMPL_BOILERPLATE(_type, categories, debug_name)                     \
    virtual inline auto type() const->Type override                             \
    {                                                                           \
        return _type;                                                           \
    }                                                                           \
    virtual inline auto in_category(Category category) const->bool override     \
    {                                                                           \
        return static_cast<size_t>(categories) & static_cast<size_t>(category); \
    }                                                                           \
    virtual inline auto debug_string() const->std::string override { return debug_name; }

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
        Input =       (1 << 1),
        Keyboard =    (1 << 2),
        Mouse =       (1 << 3),
        MouseButton = (1 << 4),
        Window =      (1 << 5)
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

    class AbstractEvent
    {
    public:
        virtual inline auto type() const -> Type = 0;
        virtual inline auto in_category(Category category) const -> bool = 0;
        virtual inline auto debug_string() const -> std::string { return "Undefined event"; };
    };

    class AbstractLayer
    {
    public:
        virtual inline auto on_attach() -> void {}
        virtual inline auto on_detach() -> void {}
        virtual inline auto on_event(const AbstractEvent &event) -> bool = 0;
    };

    class LayerStack
    {
    public:
        inline auto push(AbstractLayer *layer) -> void
        {
            layers.push_back(layer);
            layer->on_attach();
        }

        inline auto pop(AbstractLayer *layer) -> void
        {
            const auto itr = std::find(layers.begin(), layers.end(), layer);

            SOFT_ASSERT(itr != layers.end(), "Tried to pop layer from LayerStack that was not in it to begin with.");

            if (itr != layers.end())
            {
                layer->on_detach();
                layers.erase(itr);
            }
        }

        inline auto begin() -> std::vector<AbstractLayer *>::reverse_iterator { return layers.rbegin(); }
        inline auto end() -> std::vector<AbstractLayer *>::reverse_iterator { return layers.rend(); }

    private:
        std::vector<AbstractLayer *> layers;
    };

    // Implement basic events

    class WindowClose : public AbstractEvent
    {
    public:
        EVT_IMPL_BOILERPLATE(Type::WindowClose, Category::Application | Category::Window, "WindowClose");
    };

    class WindowResize : public AbstractEvent
    {
    public:
        WindowResize(size_t width, size_t height) : width(width), height(height){};
        EVT_IMPL_BOILERPLATE(Type::WindowResize, Category::Window, Log::format("WindowResize: width=%u, height=%u", width, height));

        const size_t width, height;
    };

    class WindowFocus : public AbstractEvent
    {
    public:
        EVT_IMPL_BOILERPLATE(Type::WindowFocus, Category::Window, "WindowFocus");
    };

    class WindowLostFocus : public AbstractEvent
    {
    public:
        EVT_IMPL_BOILERPLATE(Type::WindowLostFocus, Category::Window, "WindowLostFocus");
    };

    class WindowMoved : public AbstractEvent
    {
    public:
        WindowMoved(int x, int y)
            : x(x), y(y) {}
        EVT_IMPL_BOILERPLATE(Type::WindowMoved, Category::Window, Log::format("WindowMoved: x:%u y:%u", x, y));

        const int x, y;
    };

    class AppTick : public AbstractEvent
    {
    public:
        AppTick(double dt) : dt(dt) {}
        EVT_IMPL_BOILERPLATE(Type::AppTick, Category::Application, Log::format("AppTick: dt=%f", dt));

        const double dt;
    };

    class KeyPressed : public AbstractEvent
    {
    public:
        KeyPressed(int key, bool repeats)
            : key(key), repeats(repeats) {}
        EVT_IMPL_BOILERPLATE(Type::KeyPressed, Category::Keyboard | Category::Input, Log::format("KeyPressed: %u repeats=%u", key, repeats));

        const int key;
        const bool repeats;
    };

    class KeyReleased : public AbstractEvent
    {
    public:
        KeyReleased(int key)
            : key(key) {}
        EVT_IMPL_BOILERPLATE(Type::KeyReleased, Category::Keyboard | Category::Input, Log::format("KeyReleased: %u", key));

        const int key;
    };

    class MouseButtonPressed : public AbstractEvent
    {
    public:
        MouseButtonPressed(int button)
            : button(button) {}
        EVT_IMPL_BOILERPLATE(Type::MouseButtonPressed, Category::Mouse | Category::Input, Log::format("MouseButtonPressed: %u", button));

        const int button;
    };

    class MouseButtonReleased : public AbstractEvent
    {
    public:
        MouseButtonReleased(int button)
            : button(button) {}
        EVT_IMPL_BOILERPLATE(Type::MouseButtonReleased, Category::Mouse | Category::Input, Log::format("MouseButtonReleased: %u", button));

        const int button;
    };

    class MouseScrolled : public AbstractEvent
    {
    public:
        MouseScrolled(double x_offset, double y_offset)
            : x_offset(x_offset), y_offset(y_offset) {}
        EVT_IMPL_BOILERPLATE(Type::MouseScrolled, Category::Mouse | Category::Input, Log::format("MouseScrolled: x:%f y:%f", x_offset, y_offset));

        const double x_offset, y_offset;
    };

    class MouseMoved : public AbstractEvent
    {
    public:
        MouseMoved(double x, double y)
            : x(x), y(y) {}
        EVT_IMPL_BOILERPLATE(Type::MouseMoved, Category::Mouse | Category::Input, Log::format("MouseMoved: x:%f y:%f", x, y));

        const double x, y;
    };

    
    template<typename T>
    inline auto event_cast(const AbstractEvent &event) -> const T &
    {
        static_assert(std::is_base_of_v<AbstractEvent, T>, "T must derive from AbstractEvent to use event_cast ");
        return *reinterpret_cast<const T *>(&event);
    }
}

#endif