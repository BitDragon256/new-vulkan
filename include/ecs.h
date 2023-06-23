#pragma once

#include <bitset>
#include <iostream>
#include <queue>
#include <unordered_map>
#include <tuple>
#include <vector>

#ifndef ECS_START_ENTITIES
#define ECS_START_ENTITIES 5000
#endif

#ifndef ECS_MAX_COMPONENTS
#define ECS_MAX_COMPONENTS 100
#endif

typedef uint32_t EntityId;
typedef uint32_t ComponentTypeId;
typedef uint32_t SystemId;

class IComponentList { virtual void polymorphism_proof() {} };

template<typename T>
class ComponentList : public IComponentList
{
public:
	void add(EntityId entity);
	void remove(EntityId entity);
	T& get(EntityId entity);
private:
	std::vector<T> m_components;
	std::unordered_map<EntityId, size_t> m_entityToIndex;
	std::vector<EntityId> m_entities; // basically a index -> entity map
};

class ComponentManager
{
public:
	~ComponentManager();
	template<typename T> void add_component(EntityId entity);
	template<typename T> void remove_component(EntityId entity);
	template<typename T> T& get_component(EntityId entity);
	std::bitset<ECS_MAX_COMPONENTS> used_components(EntityId entity);

	void remove_entity(EntityId entity);
private:
	std::vector<IComponentList*> m_components; // indexed by ComponentTypeId
	std::unordered_map<const char*, ComponentTypeId> m_typeToId;
	std::unordered_map<EntityId, std::bitset<ECS_MAX_COMPONENTS>> m_entityComponents;

	template<typename T> ComponentList<T>* list(ComponentTypeId id);
};

template<typename... Types>
struct typelist {};

template<typename Head, typename... Tail>
std::vector<const char*> get_type_names(typelist<Head, Tail...>& t)
{
	std::vector<const char*> types;
	types.push_back(typeid(Head).name());
	typelist<Tail...> remaining;
	auto rest = get_type_names(remaining);
	types.insert(types.end(), rest.begin(), rest.end());
	return types;
}
template<typename Head>
std::vector<const char*> get_type_names(typelist<Head>& t)
{
	std::vector<const char*> types;
	types.push_back(typeid(Head).name());
	return types;
}

class ECSManager;

class ISystem
{
public:
	virtual void update(float dt, EntityId entity, ECSManager& ecs) = 0;
	constexpr virtual const std::vector<const char*>& component_types() const = 0;
};

template<typename... Types>
class System : public ISystem
{
public:
	System()
	{
		typelist<Types...> t;
		m_types = get_type_names(t);
	}
	constexpr const std::vector<const char*>& component_types() const override;
private:
	std::vector<const char*> m_types;
};

class ECSManager
{
public:
	ECSManager();
	~ECSManager();

	template<typename S> void register_system();
	EntityId create_entity();
	void delete_entity(EntityId entity);

	template<typename T> void add_component(EntityId entity);
	template<typename T> void remove_component(EntityId entity);
	template<typename T> T& get_component(EntityId entity);
	std::bitset<ECS_MAX_COMPONENTS> used_components(EntityId entity);

	void update_systems(float dt);
private:
	std::vector<ISystem*> m_systems;
	std::vector<std::vector<EntityId>> m_systemEntities; // system buckets, in buckets are all entities affected by that system
	std::vector<std::bitset<ECS_MAX_COMPONENTS>> m_systemComponents; // bitset for all systems for used components
	std::unordered_map<const char*, ComponentTypeId> m_componentTypeToId;

	std::queue<EntityId> m_availableEntities;
	size_t m_maxEntities;
	void fill_available_entities();

	ComponentManager m_componentManager;

	void ensure_component(const char* typeName);
};