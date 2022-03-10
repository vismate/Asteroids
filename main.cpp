#include "ECS.hpp"
#include <iostream>

struct S
{
    int a, b;
    ~S()
    {
        std::cout << "DESTROYED S : {" << a << ", " << b << "}" << std::endl;
    }
};

struct G
{
    int a, b;
    ~G()
    {
        std::cout << "DESTROYED G : {" << a << ", " << b << "}" << std::endl;
    }
};

int f(S &)
{
    std::cout << "HANDLE S" << std::endl;
    return 2;
}

int main()
{
    ECS::Scene scene;

    ECS::EntityID e1 = scene.create();
    scene.destroy(e1);
    e1 = scene.create();
    ECS::EntityID e2 = scene.create();

    std::cout << ECS::_impl::version_of(e1) << " " << ECS::_impl::version_of(e2) << std::endl;

    scene.assign<S>(e1, 1, 2);
    scene.assign<G>(e1, 1, 2);

    std::cout << "bim" << std::endl;
    scene.assign<S>(e2, S{3, 4});
    std::cout << "bom" << std::endl;
    scene.assign<G>(e2, 3, 4);
    std::cout << "bem" << std::endl;
    std::cout << scene.has_any<S, int, G>(e1) << std::endl;

    S &s = scene.get<S>(e1);
    s.b = 600;

    scene.for_each_component<S>(f);
    scene.for_each_entity([](ECS::EntityID entity_id)
                          { std::cout << "Entity: " << ECS::_impl::index_of(entity_id) << std::endl; });
    return 0;
}