#ifndef ECS_HPP
#define ECS_HPP

#include "Error.hpp"

#include <vector>
#include <iostream>
#include <array>
#include <tuple>

/*
TODO: Review _impl::SceneView::Iterator
TODO: Try and eliminate all "friend" declarations
TODO: Do away with AbstractComponentPool
TODO: Optimize, optimize, optimize (eg.: has_all, get_all)
TODO: Write documentation
TODO: Paging
TODO: Research groups
*/

namespace ECS
{
    using EntityIndex = unsigned int;
    using EntityVersion = unsigned int;
    using EntityID = unsigned long long int;

    class Scene;
    class Entity;

    namespace _impl
    {
        class AbstractComponentPool;

        template <typename T>
        class ComponentPool;

        template <typename... Ts>
        class SceneView;

        inline auto next_component_id() -> size_t
        {
            static size_t id = 0;
            return id++;
        }

        template <typename T>
        inline auto component_id() -> size_t
        {
            static size_t id = next_component_id();
            return id;
        }

        inline auto constexpr index_of(EntityID id) -> EntityIndex
        {
            return id >> 32;
        }
        inline auto constexpr version_of(EntityID id) -> EntityVersion
        {
            return static_cast<EntityVersion>(id);
        }

        inline auto constexpr make_id(EntityIndex index, EntityVersion version) -> EntityID
        {
            return (static_cast<EntityID>(index) << 32) | static_cast<EntityID>(version);
        }

        static const EntityIndex INVALID_INDEX = static_cast<EntityIndex>(-1);
        inline auto constexpr valid_id(EntityID entity_id) -> bool
        {
            return index_of(entity_id) != INVALID_INDEX;
        }

        // An abstract base-class for ComponentPool that provides access to non-type specific methods.
        class AbstractComponentPool
        {
        public:
            virtual ~AbstractComponentPool(){};
            virtual inline auto contains(EntityIndex entity_index) const -> bool = 0;
            virtual inline auto remove(EntityIndex entity_index) -> void = 0;
            virtual inline auto size() const -> size_t = 0;
            virtual inline auto reserve(size_t amount) -> void = 0;
        };

        // Sparse set based representation of AbstractComponentPool.
        template <typename T>
        class ComponentPool : AbstractComponentPool
        {
        public:
            ComponentPool() = default;

            template <typename... Ts>
            inline auto emplace(EntityIndex entity_index, Ts &&...args) -> T &
            {
                ASSERT(not contains(entity_index), "Tried to emplace to already occupied slot");

                if (sparse_array.size() <= entity_index)
                {
                    sparse_array.resize(entity_index + 1, _impl::INVALID_INDEX);
                }

                dense_array.push_back(entity_index);
                sparse_array[entity_index] = dense_array.size() - 1;

                component_array.emplace_back(std::forward<Ts>(args)...);
                return component_array.back();
            }

            virtual inline auto reserve(size_t amount) -> void override
            {
                sparse_array.reserve(amount);
                dense_array.reserve(amount);
                component_array.reserve(amount);
            }

            inline auto get(EntityIndex entity_index) -> T &
            {
                return component_array[sparse_array[entity_index]];
            }

            virtual inline auto size() const -> size_t override
            {
                return dense_array.size();
            }

            virtual inline auto contains(EntityIndex entity_index) const -> bool override
            {
                return sparse_array.size() > entity_index && sparse_array[entity_index] != _impl::INVALID_INDEX;
            }

            virtual inline auto remove(EntityIndex entity_index) -> void override
            {
                if (contains(entity_index))
                {
                    component_array[sparse_array[entity_index]] = std::move(component_array.back());
                    component_array.pop_back();

                    dense_array[sparse_array[entity_index]] = dense_array.back();
                    sparse_array[dense_array.back()] = sparse_array[entity_index];
                    sparse_array[entity_index] = _impl::INVALID_INDEX;
                    dense_array.pop_back();
                }
            }

        private:
            std::vector<EntityIndex> dense_array;
            std::vector<T> component_array;
            std::vector<EntityIndex> sparse_array;

            friend class ECS::Scene;

            template <typename... Ts>
            friend class SceneView;
        };

    }

    class Scene
    {

    public:
        static const unsigned max_entity_count = _impl::INVALID_INDEX;

        Scene() = default;
        Scene(const Scene &) = delete;
        Scene(Scene &&) = delete;
        inline auto operator=(const Scene &) = delete;
        inline auto operator=(Scene &&) = delete;

        ~Scene()
        {
            for (auto p : component_pools)
                delete p;
        }

        inline auto create() -> EntityID
        {
            if (free_entities.empty())
            {
                entities.push_back(_impl::make_id(entities.size(), 0));
                return entities.back();
            }

            EntityIndex index = free_entities.back();
            free_entities.pop_back();
            entities[index] = _impl::make_id(index, _impl::version_of(entities[index]));

            return entities[index];
        }

        template <typename T>
        inline auto reserve_component(size_t amount) -> void
        {
            assure_component_pool<T>().reserve(amount);
        }

        inline auto reserve_entity(size_t amount) -> void
        {
            entities.reserve(amount);
        }

        inline auto exists(EntityID entity_id) const -> bool
        {
            const auto index = _impl::index_of(entity_id);
            return entities.size() > index && entities[index] == entity_id;
        }

        template <typename T>
        inline auto has(EntityID entity_id) const -> bool
        {
            const auto component_id = _impl::component_id<T>();
            return valid_component_pool(component_id) && exists(entity_id) && component_pools[component_id]->contains(_impl::index_of(entity_id));
        }

        template <typename... Ts>
        inline auto has_all(EntityID entity_id) const -> bool
        {
            return (has<Ts>(entity_id) && ...);
        }

        template <typename... Ts>
        inline auto has_any(EntityID entity_id) const -> bool
        {
            return (has<Ts>(entity_id) || ...);
        }

        template <typename T, typename... Ts>
        inline auto assign(EntityID entity_id, Ts &&...args) -> T &
        {
            return assure_component_pool<T>().emplace(_impl::index_of(entity_id), std::forward<Ts>(args)...);
        }

        template <typename T>
        inline auto get(EntityID entity_id) const -> T &
        {
            auto &pool = component_pools[_impl::component_id<T>()];
            return reinterpret_cast<_impl::ComponentPool<T> *>(pool)->get(_impl::index_of(entity_id));
        }

        template <typename... Ts>
        inline auto get_all(EntityID entity_id) const -> std::tuple<Ts &...>
        {
            return std::tuple<Ts &...>(get<Ts>(entity_id)...);
        }

        template <typename T>
        inline auto remove(EntityID entity_id) -> void
        {
            component_pools[_impl::component_id<T>()]->remove(_impl::index_of(entity_id));
        }

        template <typename... Ts>
        inline auto remove_all(EntityID entity_id) -> void
        {

            std::array<size_t, sizeof...(Ts)> component_ids{_impl::component_id<Ts>()...};
            for (auto component_id : component_ids)
            {
                component_pools[component_id]->remove(_impl::index_of(entity_id));
            }
        }

        inline auto destroy(EntityID entity_id) -> void
        {
            entities[_impl::index_of(entity_id)] = _impl::make_id(_impl::INVALID_INDEX, _impl::version_of(entity_id) + 1);
            free_entities.push_back(_impl::index_of(entity_id));

            for (auto pool : component_pools)
            {
                if (pool)
                    pool->remove(_impl::index_of(entity_id));
            }
        }

        template <typename T, typename F>
        inline auto for_each_component(F function) const -> void
        {
            ASSERT(valid_component_pool(_impl::component_id<T>()), "Tried to access invalid component pool");

            auto pool = reinterpret_cast<_impl::ComponentPool<T> *>(component_pools[_impl::component_id<T>()]);
            for (auto itr = pool->component_array.begin(); itr != pool->component_array.end(); ++itr)
                function(*itr);
        }

        template <typename F>
        inline auto for_each_entity(F function) -> void
        {
            if (free_entities.empty())
            {
                for (auto entity_id : entities)
                    function(this, entity_id);
            }
            else
            {
                for (auto entity_id : entities)
                {
                    if (_impl::valid_id(entity_id))
                        function(this, entity_id);
                }
            }
        }

        inline auto entity_count() const -> size_t
        {
            return entities.size() - free_entities.size();
        }

        template <typename T>
        inline auto component_count() const -> size_t
        {
            const auto component_id = _impl::component_id<T>();
            if (valid_component_pool(component_id))
                return component_pools[component_id]->size();

            return 0;
        }

        template <typename... Ts>
        inline auto view() -> _impl::SceneView<Ts...>
        {
            return _impl::SceneView<Ts...>(this);
        }

    private:
        inline auto valid_component_pool(size_t component_id) const -> bool
        {
            return component_pools.size() > component_id && component_pools[component_id] != nullptr;
        }
        template <typename T>
        inline auto assure_component_pool() -> _impl::ComponentPool<T> &
        {
            const size_t component_id = _impl::component_id<T>();
            if (component_pools.size() <= component_id)
            {
                component_pools.resize(component_id + 1, nullptr);
            }
            if (component_pools[component_id] == nullptr)
            {
                component_pools[component_id] = reinterpret_cast<_impl::AbstractComponentPool *>(new _impl::ComponentPool<T>);
            }

            return reinterpret_cast<_impl::ComponentPool<T> &>(*component_pools[component_id]);
        }

    private:
        std::vector<_impl::AbstractComponentPool *> component_pools;
        std::vector<EntityID> entities;
        std::vector<EntityIndex> free_entities;

        template <typename... Ts>
        friend class _impl::SceneView;
    };

    class Entity
    {

    public:
        Entity(Scene *scene, EntityID entity_id) : scene(scene), id(entity_id)
        {
            ASSERT(scene, "Creation of entity unsuccessful: \'scene\' is nullptr");
        };

        operator EntityID() { return id; }
        operator bool() { return exists(); }

        auto operator==(Entity other) const -> bool
        {
            return id == other.id && scene == other.scene;
        }

        template <typename T>
        inline auto has() const -> bool
        {
            SOFT_ASSERT(exists(), "Tried to access an invalid entity");
            return scene->has<T>(id);
        }

        template <typename... Ts>
        inline auto has_all() const -> bool
        {
            SOFT_ASSERT(exists(), "Tried to access an invalid entity");
            return scene->has_all<Ts...>(id);
        }

        template <typename... Ts>
        inline auto has_any() const -> bool
        {
            SOFT_ASSERT(exists(), "Tried to access an invalid entity");
            return scene->has_any<Ts...>(id);
        }

        template <typename T, typename... Ts>
        inline auto assign(Ts &&...args) -> T &
        {
            ASSERT(exists(), "Tried to access an invalid entity");
            return scene->assign<T>(id, std::forward<Ts>(args)...);
        }

        template <typename T>
        inline auto get() const -> T &
        {
            ASSERT(exists(), "Tried to access an invalid entity");
            ASSERT(has<T>(), "Tried to access an invalid component");

            return scene->get<T>(id);
        }

        template <typename... Ts>
        inline auto get_all() const -> std::tuple<Ts &...>
        {
            ASSERT(exists(), "Tried to access an invalid entity");
            ASSERT(has_all<Ts...>(), "Tried to access an invalid component");

            return scene->get_all<Ts...>(id);
        }

        template <typename T>
        inline auto remove() -> void
        {
            ASSERT(exists(), "Tried to access an invalid entity");
            SOFT_ASSERT(has<T>(), "Tried to remove an invalid component");

            scene->remove<T>(id);
        }

        template <typename... Ts>
        inline auto remove_all() -> void
        {
            ASSERT(exists(), "Tried to access an invalid entity");
            SOFT_ASSERT(has_all<Ts...>(), "Tried to remove an invalid component");

            scene->remove_all<Ts...>(id);
        }

        auto exists() const -> bool
        {
            return scene->exists(id);
        }

        inline auto destroy() -> void
        {
            ASSERT(exists(), "Tried to destroy an invalid entity");

            scene->destroy(id);
        }

        inline auto get_scene() const -> Scene &
        {
            return *scene;
        }

    private:
        Scene *scene;
        EntityID id;
    };

    namespace _impl
    {
        template <typename... Ts>
        class SceneView
        {
        public:
            SceneView(Scene *scene) : scene(scene)
            {
                ASSERT(scene, "\'scene\' is nullptr");
            }
            // TODO: Redo this fucking mess...
            class Iterator
            {
            public:
                Iterator() : index(_impl::INVALID_INDEX)
                {
                }
                Iterator(Scene *scene) : scene(scene), component_ids({_impl::component_id<Ts>()...})
                {

                    // TODO: clean this up...
                    index = _impl::INVALID_INDEX;
                    for (auto component_id : component_ids)
                    {
                        if (not scene->valid_component_pool(component_id))
                        {
                            index = _impl::INVALID_INDEX;
                            break;
                        }
                        auto pool = scene->component_pools[component_id];
                        if (pool->size() <= index)
                        {
                            index = pool->size() - 1;
                            indexes_to_iterate = &(reinterpret_cast<_impl::ComponentPool<int> *>(pool)->dense_array);
                        }
                    }

                    if (sizeof...(Ts) == 0 || _impl::index_of(scene->entities[(*indexes_to_iterate)[index]]) == _impl::INVALID_INDEX)
                    {
                        index = _impl::INVALID_INDEX;
                    }
                    else
                    {
                        while (index != _impl::INVALID_INDEX && should_skip())
                            index--;
                    }
                }

                inline auto operator*() const -> std::tuple<EntityID, Ts &...>
                {
                    const auto id = scene->entities[(*indexes_to_iterate)[index]];
                    return std::tuple<EntityID, Ts &...>(id, scene->get<Ts>(id)...);
                }

                inline auto operator==(const Iterator &other) const -> bool
                {
                    return index == other.index || index == _impl::INVALID_INDEX;
                }

                inline auto operator!=(const Iterator &other) const -> bool
                {
                    return index != other.index && index != _impl::INVALID_INDEX;
                }
                inline auto operator++() -> Iterator &
                {
                    do
                    {
                        index--;
                    } while (index != _impl::INVALID_INDEX && should_skip());

                    return *this;
                }

            private:
                inline auto should_skip() const -> bool
                {
                    for (auto component_id : component_ids)
                    {
                        if (not scene->component_pools[component_id]->contains((*indexes_to_iterate)[index]) || _impl::index_of(scene->entities[(*indexes_to_iterate)[index]]) == _impl::INVALID_INDEX)
                            return true;
                    }
                    return false;
                }

            private:
                Scene *scene;
                EntityIndex index;
                std::vector<EntityIndex> *indexes_to_iterate;
                std::array<size_t, sizeof...(Ts)> component_ids;
            };

            inline auto begin() const -> const Iterator
            {
                return Iterator(scene);
            }

            inline auto end() const -> const Iterator
            {
                return Iterator();
            }

        private:
            Scene *scene;
        };
    }
}
#endif