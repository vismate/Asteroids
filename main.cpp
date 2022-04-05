#include "Application.hpp"
#include "Input.hpp"
#include "ECS.hpp"

/*
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
            const auto dt = event.as<AppTick>().dt;

            theta += dt * speed / 10;

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
    std::shared_ptr<Graphics::VertexArray> va;
    std::shared_ptr<Graphics::Shader> shader;

    MenuLayer()
        : logger_layer(*(new Event::EventLoggerLayer())), app(App::Application::get_instance())
    {
        logger_layer
            .blacklist_type(Event::Type::AppTick)
            .logger.set_datetime_format("[%H:%M:%S]");
    }

    inline virtual auto on_attach() -> void override
    {
        using namespace Graphics;
        using std::make_shared;

        ib = make_shared<IndexBuffer>(indices, 3);
        vb = make_shared<VertexBuffer>(vertices, sizeof(float) * 6);
        va = make_shared<VertexArray>();
        shader = make_shared<Shader>("vertex", "fragment");

        vb->set_layout({{GLtype::Float, 2, false}});
        va->add_vertex_buffer(vb);
        va->set_index_buffer(ib);
    }

    inline virtual auto on_event(const Event::AbstractEvent &event) -> bool override
    {
        using namespace Event;
        static bool active{false}, logging{false}, fullscreen{false};

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
            vb->bind();
            ib->bind();
            shader->bind();
            va->bind();
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
*/

class AsteroidsDemo;

class MenuLayer : public Event::AbstractLayer
{

public:
    MenuLayer()
        : active(false),
          app(App::Application::get_instance_as<AsteroidsDemo>())
    {
    }

    inline virtual auto on_event(const Event::AbstractEvent &event) -> bool override
    {
        using namespace Event;

        if (event.type() == Type::KeyPressed && event.as<KeyPressed>().key == Input::Key::ESCAPE)
        {
            Log::info((active = not active) ? "Paused game" : "Unpaused game");
        }

        return active && not(event.type() == Type::WindowRedraw);
    }

private:
    bool active;
    AsteroidsDemo &app;
};

class GameLayer : public Event::AbstractLayer
{
public:
    GameLayer() : app(App::Application::get_instance_as<AsteroidsDemo>())
    {
    }

    inline virtual auto on_event(const Event::AbstractEvent &event) -> bool override
    {
        using namespace Event;
        switch (event.type())
        {
        case Type::AppTick:
        {
            glClearColor(1.0,0,0,1);
            glClear(GL_COLOR_BUFFER_BIT);
            draw();
        }
        break;

        case Type::WindowRedraw:
        {

            draw();
        }
        break;
        default:
            return false;
        }

        return false;
    }

private:
    struct Polygon
    {
        std::shared_ptr<Graphics::VertexBuffer> vb;
        Polygon(const std::vector<std::pair<float, float>> &data)
            : vb(std::make_shared<Graphics::VertexBuffer>(data.data(), data.size() * sizeof(std::pair<float, float>)))
        {
            vb->set_layout({{Graphics::GLtype::Float, 2, false}});
        }
    };

    inline auto draw() -> void
    {
        static Polygon p({{-0.5,-0.5},{0,0.5}});
        static Graphics::Shader shader("vertex", "fragment");
        static Graphics::VertexArray va;
        static bool x{false};
        if(not x) {va.add_vertex_buffer(p.vb); x = true; }

        p.vb->bind();
        va.bind();
        shader.bind();
        glDrawArrays(GL_LINES, 0, 2);
    }

    AsteroidsDemo &app;
};

class AsteroidsDemo : public App::Application
{
public:
    AsteroidsDemo() : App::Application::Application("Asteroids Demo")
    {
        window
            .set_size(1366, 768)
            .set_aspect_constraints(16, 9)
            .set_vsync(true);

        push_layer(new GameLayer());
        push_layer(new MenuLayer());

#ifdef LOG_EVENTS
        auto logger_layer = new Event::EventLoggerLayer();
        logger_layer->blacklist_type(Event::Type::AppTick);
        push_layer(logger_layer);
#endif
    }
};

int main()
{
    AsteroidsDemo app;
    app.run();
}