#include "Application.hpp"
#include "Input.hpp"

#include <cmath>

struct GraphicsLayer : Event::AbstractLayer
{
    inline virtual auto on_event(const Event::AbstractEvent &event) -> bool override
    {
        using namespace Event;
        using namespace Input;

        static double theta{0}, speed{1};

        if (event.type() == Type::AppTick)
        {
            auto dt = event.as<AppTick>().dt;

            if (is_pressed(Key::LEFT))
                theta -= dt * speed;
            if (is_pressed(Key::RIGHT))
                theta += dt * speed;
            if (std::abs(theta) > 6.28)
                theta -= 6.28;

            glClearColor(std::sin(theta), std::sin(theta + 6.28 / 3.0), std::sin(theta + 12.56 / 3.0), 1);
            glClear(GL_COLOR_BUFFER_BIT);
        }
        else if (event.type() == Type::WindowRedraw)
        {
            glClear(GL_COLOR_BUFFER_BIT);
        }

        return false;
    }
};

struct MenuLayer : Event::AbstractLayer
{
    Event::EventLoggerLayer &logger_layer;
    App::Application &app;

    const float vertices[6] = {
        -0.25f, -0.50f,
        0.25f, 0.0f,
        -0.25f, 0.50f};

    const unsigned int indices[3] = {
        0, 1, 2 // first triangle
    };

    std::shared_ptr<Graphics::IndexBuffer> ib;
    std::shared_ptr<Graphics::VertexBuffer> vb;
    Graphics::VertexArray va;
    Graphics::Shader shader;


    MenuLayer()
        : logger_layer(*(new Event::EventLoggerLayer())), app(App::Application::get_instance()), shader("vertex","fragment")
    {
        logger_layer
            .blacklist_type(Event::Type::AppTick)
            .logger.set_datetime_format("[%H:%M:%S]");
    }

    inline virtual auto on_attach() -> void override
    {
        ib = std::make_shared<Graphics::IndexBuffer>(indices, 3);
        vb = std::make_shared<Graphics::VertexBuffer>(vertices, sizeof(float) * 6);
        vb->set_layout({{Graphics::GLtype::Float, 2, false}});
        va.add_vertex_buffer(vb);
        va.set_index_buffer(ib);
    }

    inline virtual auto on_event(const Event::AbstractEvent &event) -> bool override
    {
        using namespace Event;
        static bool active{false}, logging{false}, fullscreen{false};
        
        vb->bind();
        ib->bind();
        shader.bind();
        va.bind();

        if (event.type() == Type::KeyPressed)
        {
            switch (event.as<KeyPressed>().key)
            {
            case Input::Key::SPACE:
                Log::debug((active = !active) ? "Paused..." : "Unpaused...");
                return true;
            case Input::Key::F10:
                (logging = !logging) ? app.push_layer(&logger_layer) : app.pop_layer(&logger_layer);
                return true;
            case Input::Key::F11:
                app.get_window().set_fullscreen(fullscreen = !fullscreen);
                return true;
            case Input::Key::ESCAPE:
                app.close();
            default:
                break;
            }
        }

        if (active)
        {
            glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);
        }

        return event.type() != Type::WindowRedraw && active;
    }
};

struct ColorSine : public App::Application
{
    ColorSine(App::Args args = App::Args()) : App::Application::Application("ColorSine", args)
    {
        window
            .set_size(1366, 768)
            .set_aspect_constraints(16, 9)
            .set_vsync(true);

        push_layer(new GraphicsLayer());
        push_layer(new MenuLayer());
    }
};

int main()
{
    ColorSine app;
    app.run();
}