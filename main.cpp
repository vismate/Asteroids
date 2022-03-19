#include "ECS.hpp"
#include "Application.hpp"
#include <iostream>
#include <fstream>
#include <chrono>

struct S
{
    int a, b;
    S(int a, int b) : a(a), b(b)
    {
        // std::cout << "CONSTRUCTED S : {" << a << ", " << b << "}" << std::endl;
    }
    ~S()
    {
        //  std::cout << "DESTROYED S : {" << a << ", " << b << "}" << std::endl;
    }
};

struct G
{
    int a, b;
    G(int a, int b) : a(a), b(b)
    {
        // std::cout << "CONSTRUCTED G : {" << a << ", " << b << "}" << std::endl;
    }
    ~G()
    {
        // std::cout << "DESTROYED G : {" << a << ", " << b << "}" << std::endl;
    }
};

int f(S &)
{
    std::cout << "HANDLE S" << std::endl;
    return 2;
}

#include <unistd.h>
void log2file()
{
    using namespace Log;

    std::ofstream file("../log.txt");
    Logger log;

    log
        .set_flag(Flag::Ansi, false)
        .set_flag(Flag::Tag, false)
        .set_stream(file)
        .set_datetime_format("%Y. %B %d. %H:%M:%S");

    for (unsigned i = 0; i < 10; i++)
    {
        int sleep_for{rand() % 3};
        log(format("Iteration: %u; Will sleep for: %ds", i, sleep_for));
        sleep(sleep_for);
    }
}

#include <unistd.h>
#include <cmath>

class CustomApp;

class GameLayer : public Event::AbstractLayer
{
    virtual inline auto on_attach() -> void override {}
    virtual inline auto on_detach() -> void override {}
    virtual inline auto on_event(const Event::AbstractEvent &event) -> bool override
    {
        static float theta = 0;
        const static float speed = 0.5;
        if (event.type() == Event::Type::AppTick)
        {
            theta += Event::event_cast<Event::AppTick>(event).dt * speed;
            if (theta >= 6.28)
                theta -= 6.28;
            glClearColor(std::sin(theta) / 2 + 0.5, std::sin(theta + 6.28 / 3) / 2 + 0.5, std::sin(theta + 2 * 6.28 / 3) / 2 + 0.5, 1);
            glClear(GL_COLOR_BUFFER_BIT);
        }
        else if (event.type() == Event::Type::WindowRedraw)
            glClear(GL_COLOR_BUFFER_BIT);

        return false;
    }
};

class BlockingLayer : public Event::AbstractLayer
{
    virtual inline auto on_attach() -> void override {}
    virtual inline auto on_detach() -> void override {}
    virtual inline auto on_event(const Event::AbstractEvent &event) -> bool override;
};

class CustomApp : public Application::AbstractApplication
{
private:
    Event::EventLoggerLayer *logger_layer;

public:
    void init()
    {
        window
            .set_size(1366, 768)
            .set_title("Custom application");

        layer_stack.push(new GameLayer());
        layer_stack.push(new BlockingLayer());

        logger_layer = new Event::EventLoggerLayer();

        (*logger_layer)
            .blacklist_type(Event::Type::AppTick)
            .blacklist_type(Event::Type::MouseMoved)
            .logger
                .set_datetime_format("[%H:%M:%S]");

        layer_stack.push(logger_layer);
    }

    inline auto get_window() -> Graphics::Window &
    {
        return window;
    }

    static inline auto get_instance() -> CustomApp &
    {
        static CustomApp instance;
        return instance;
    }
};

inline auto BlockingLayer::on_event(const Event::AbstractEvent &event) -> bool
{
    using namespace Event;
    static bool active{false};
    static bool fs{false};

    if (event.type() == Type::MouseButtonPressed && event_cast<MouseButtonPressed>(event).button == 0)
    {
        active = !active;
        Log::debug(active ? "Paused..." : "Unpaused...");
        return true;
    }
    else if (event.type() == Type::KeyPressed && event_cast<KeyPressed>(event).key == 300)
        CustomApp::get_instance().get_window().set_fullscreen(fs = !fs);

    return event.type() != Type::WindowRedraw && active;
}

int main()
{

    // Application
    {
        auto &app = CustomApp::get_instance();
        app.init();
        app.run();
    }

    /*

    // Log
    {
        Log::GLOBAL_LOG_LEVEL = Log::info;

        Log::error("Hello");
        Log::debug("Haha, fake error");
        Log::info.set_flag(Log::Flag::Enabled, false).set_flag(Log::Flag::SuccesIfDisabled, false);
        Log::debug(Log::format("Disabled logger returned %s", Log::info("This will not be visible") ? "true" : "false"));
        Log::info.set_flag(Log::Flag::Enabled, true);
        Log::info(Log::format("%s is an info of priority %u", "Formatted information", 5));

        Log::info(sizeof(Log::info));
    }

    // ECS
    {
        ECS::Scene scene;
        scene.reserve_component<S>(2560);
        scene.reserve_component<G>(2560);
        scene.reserve_component<int>(5120);
        scene.reserve_entity(5120);

        std::vector<ECS::Entity> entities;
        entities.reserve(5120);

        for (int i = 0; i < 5120; i++)
        {
            ECS::Entity entity(&scene, scene.create());
            if (rand() % 33 == 0)
            {
                entity.assign<S>(1, 2);
                entity.assign<G>(3, 4);
            }
            else if (rand() % 2)
                entity.assign<G>(1, 2);
            else
                entity.assign<S>(3, 4);

            entity.assign<int>(7);

            entities.push_back(entity);
        }

        auto start = std::chrono::high_resolution_clock::now();

        int ss{0}, gs{0}, ss_and_gs{0};

        ss = scene.component_count<S>();
        gs = scene.component_count<G>();
        ss_and_gs = ss + gs - scene.entity_count();

        std::cout << ss << ", " << gs << ", " << ss_and_gs << std::endl;

        for (auto [id, s, g] : scene.view<S, G>())
        {
            if (rand() % 2)
                scene.remove<G>(id);
            else
                scene.remove<S>(id);
        }

        std::stringstream stream;
        Log::debug.set_stream(stream);
        Log::debug("before full itr");

        size_t db{0};
        scene.for_each_entity([&db](ECS::Scene *sc, ECS::EntityID id)
                              { sc->get<int>(id) = 8; });
        for (auto [id, g] : scene.view<G>())
        {
            g.a = (rand() % g.b) * 4;
            for (int i = 0; i < 5; i++)
                g.a *= 2;
        }

        Log::debug.set_stream(std::clog);
        Log::debug(stream.str());
        Log::debug("after full itr");

        ss = scene.component_count<S>();
        gs = scene.component_count<G>();
        ss_and_gs = ss + gs - scene.entity_count();

        entities[0].destroy();
        entities[0].has<S>();

        std::cout << ss << ", " << gs << ", " << ss_and_gs << std::endl;
        std::cout << scene.entity_count() << std::endl;
        auto end = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        Log::debug(Log::format("Took: %u", duration.count()));
    }

    */
    return 0;
}