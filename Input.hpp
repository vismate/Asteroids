#ifndef INPUT_HPP
#define INPUT_HPP

#include "Application.hpp"
#include "InputCodes.hpp"

namespace Input
{

    inline auto is_pressed(Key key) -> bool
    {
        auto window_handle = App::Application::get_instance().get_window().get_native_handle();
        const auto state = glfwGetKey(window_handle, static_cast<int>(key));
        return state == GLFW_PRESS || state == GLFW_REPEAT;
    }

    inline auto is_pressed(Mouse button) -> bool
    {
        auto window_handle = App::Application::get_instance().get_window().get_native_handle();
        const auto state = glfwGetMouseButton(window_handle, static_cast<int>(button));
        return state == GLFW_PRESS;
    }

    inline auto get_mouse_position() -> std::pair<double, double>
    {
        auto window_handle = App::Application::get_instance().get_window().get_native_handle();

        double x, y;
        glfwGetCursorPos(window_handle, &x, &y);
        return std::make_pair(x, y);
    }
}

#endif