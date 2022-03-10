#ifndef ECS_HPP
#define ECS_HPP

#include <vector>
#include <stdexcept>

namespace ECS
{
    using EntityIndex = unsigned int;
    using EntityVersion = unsigned int;
    using EntityID = unsigned long long int;

    namespace _impl
    {
        // An abstract base-class for ComponentPool that provides access to non-type specific methods.
        class AbstractComponentPool
        {
        public:
            virtual ~AbstractComponentPool(){};
            virtual auto contains(EntityIndex entity_index) const -> bool = 0;
            virtual auto remove(EntityIndex entity_index) -> void = 0;
        };

        auto next_component_id() -> size_t
        {
            static size_t id = 0;
            return id++;
        }

        template <typename T>
        auto component_id() -> size_t
        {
            static size_t id = next_component_id();
            return id;
        }

        auto constexpr index_of(EntityID id) -> EntityIndex
        {
            return id >> 32;
        }
        auto constexpr version_of(EntityID id) -> EntityVersion
        {
            return static_cast<EntityVersion>(id);
        }

        auto constexpr make_id(EntityIndex index, EntityVersion version) -> EntityID
        {
            return (static_cast<EntityID>(index) << 32) | static_cast<EntityID>(version);
        }

        static const EntityIndex INVALID_INDEX = static_cast<EntityIndex>(-1);
        auto constexpr valid_id(EntityID entity_id) -> bool
        {
            return index_of(entity_id) != INVALID_INDEX;
        }
    }

    // Sparse set based representation of AbstractComponentPool.
    template <typename T>
    class ComponentPool : _impl::AbstractComponentPool
    {
    public:
        ComponentPool()
        {
            dense_array.reserve(DEFAULT_DENSE_CAPACITY);
            component_array.reserve(DEFAULT_DENSE_CAPACITY);
            sparse_array.reserve(DEFAULT_SPARSE_CAPACITY);
        }

        template <typename... Ts>
        auto emplace(EntityIndex entity_index, Ts &&...args) -> T &
        {
            if (sparse_array.size() <= entity_index)
            {
                sparse_array.resize(entity_index + 1, _impl::INVALID_INDEX);
            }

            dense_array.push_back(entity_index);
            sparse_array[entity_index] = dense_array.size() - 1;

            component_array.emplace_back(std::forward<Ts>(args)...);
            return component_array.back();
        }
        auto get(EntityIndex entity_index) -> T &
        {
            if (not contains(entity_index))
                throw std::runtime_error("Tried to get component that doesn't exists.");

            return component_array[sparse_array[entity_index]];
        }
        auto begin() -> std::vector<T>::iterator
        {
            return component_array.begin();
        }

        auto end() -> std::vector<T>::iterator
        {
            return component_array.end();
        }

        auto begin() const -> std::vector<T>::const_iterator
        {
            return component_array.begin();
        }

        auto end() const -> std::vector<T>::const_iterator
        {
            return component_array.end();
        }

        virtual auto contains(EntityIndex entity_index) const -> bool override
        {
            return sparse_array.size() > entity_index && sparse_array[entity_index] != _impl::INVALID_INDEX;
        }
        virtual auto remove(EntityIndex entity_index) -> void override
        {
            if (contains(entity_index))
            {
                component_array[sparse_array[entity_index]] = std::move(component_array.back());
                component_array.pop_back();

                dense_array[sparse_array[entity_index]] = dense_array.back();
                sparse_array[entity_index] = -1;
                sparse_array[dense_array.back()] = sparse_array[entity_index];
                dense_array.pop_back();
            }
        }

    private:
        static constexpr size_t DEFAULT_SPARSE_CAPACITY = 512;
        static constexpr size_t DEFAULT_DENSE_CAPACITY = 1024 / sizeof(T);

        std::vector<EntityIndex> dense_array;
        std::vector<T> component_array;
        std::vector<EntityIndex> sparse_array;
    };

    class Scene
    {

    public:
        static const unsigned max_entity_count = _impl::INVALID_INDEX;

        Scene() = default;
        ~Scene()
        {
            for (auto p : component_pools)
                delete p;
        }

        auto create() -> EntityID
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

        auto exists(EntityID entity_id) const -> bool
        {
            return entities.size() > entity_id && entities[_impl::index_of(entity_id)] == entity_id;
        }

        template <typename T>
        auto has(EntityID entity_id) const -> bool
        {
            const auto component_id = _impl::component_id<T>();
            return valid_component_pool(component_id) && component_pools[component_id]->contains(entity_id);
        }

        template <typename... Ts>
        auto has_all(EntityID entity_id) const -> bool
        {
            return (has<Ts>(entity_id) && ...);
        }

        template <typename... Ts>
        auto has_any(EntityID entity_id) const -> bool
        {
            return (has<Ts>(entity_id) || ...);
        }

        template <typename T, typename... Ts>
        auto assign(EntityID entity_id, Ts &&...args) -> T &
        {
            const size_t component_id = _impl::component_id<T>();
            if (component_pools.size() <= component_id)
            {
                component_pools.resize(component_id + 1, nullptr);
            }
            if (component_pools[component_id] == nullptr)
            {
                component_pools[component_id] = reinterpret_cast<_impl::AbstractComponentPool *>(new ComponentPool<T>);
            }

            return reinterpret_cast<ComponentPool<T> *>(component_pools[component_id])
                ->emplace(_impl::index_of(entity_id), std::forward<Ts>(args)...);
        }
        template <typename T>
        auto get(EntityID entity_id) const -> T &
        {
            const auto component_id = _impl::component_id<T>();
            if (not valid_component_pool(component_id))
                throw std::runtime_error("Tried to get non-existing component pool");

            auto &pool = component_pools[component_id];
            return reinterpret_cast<ComponentPool<T> *>(pool)->get(_impl::index_of(entity_id));
        }
        template <typename T>
        auto remove(EntityID entity_id) -> void
        {
            using namespace _impl;
            if (exists(entity_id))
            {
                component_pools[component_id<T>()]->remove(index_of(entity_id));
            }
            else
            {
                throw std::runtime_error("Tried to access a deleted entity.");
            }
        }
        auto destroy(EntityID entity_id) -> void
        {
            using namespace _impl;

            entities[index_of(entity_id)] = make_id(_impl::INVALID_INDEX, version_of(entity_id) + 1);
            free_entities.push_back(index_of(entity_id));

            for (auto &pool : component_pools)
            {
                pool->remove(index_of(entity_id));
            }
        }

        template <typename T, typename F>
        auto for_each_component(F function) const -> void
        {
            const auto component_id = _impl::component_id<T>();
            if (not valid_component_pool(component_id))
                throw std::runtime_error("Tried to get non-existing component pool");

            for (T &x : *reinterpret_cast<ComponentPool<T> *>(component_pools[component_id]))
                function(x);
        }

        template <typename F>
        auto for_each_entity(F function) -> void
        {
            if (free_entities.empty())
            {
                for (auto entity_id : entities)
                    function(entity_id);
            }
            else
            {
                for (auto entity_id : entities)
                {
                    if (_impl::valid_id(entity_id))
                        function(entity_id);
                }
            }
        }

    private:
        auto valid_component_pool(size_t component_id) const -> bool
        {
            return component_pools.size() > component_id && component_pools[component_id] != nullptr;
        }

    private:
        std::vector<_impl::AbstractComponentPool *> component_pools;
        std::vector<EntityID> entities;
        std::vector<EntityIndex> free_entities;
    };
}
#endif