#pragma once

#include <assert.h>

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
	void add(EntityId entity)
	{
		assert(!m_entityToIndex.contains(entity));

		m_entityToIndex[entity] = m_components.size();
		m_components.push_back(T());
		m_entities.push_back(entity);
	}
	void remove(EntityId entity)
	{
		assert(m_entityToIndex.contains(entity));

		size_t index = m_entityToIndex[entity];
		m_components[index] = m_components.back();
		m_components.pop_back();
		m_entityToIndex[m_entities.back()] = index;
		m_entityToIndex.erase(entity);
		m_entities[index] = m_entities.back();
		m_entities.pop_back();
	}
	T& get(EntityId entity)
	{
		assert(m_entityToIndex.contains(entity));

		return m_components[m_entityToIndex[entity]];
	}
private:
	std::vector<T> m_components;
	std::unordered_map<EntityId, size_t> m_entityToIndex;
	std::vector<EntityId> m_entities; // basically a index -> entity map
};

class ComponentManager
{
public:
	~ComponentManager()
	{
		for (size_t i = 0; i < m_components.size(); i++)
		{
			delete m_components[i];
		}
	}
	template<typename T> void add_component(EntityId entity)
	{
		const char* typeName = typeid(T).name();

		if (!m_typeToId.contains(typeName))
		{
			m_typeToId[typeName] = m_components.size();
			m_components.push_back(new ComponentList<T>());
		}

		ComponentTypeId id = m_typeToId[typeName];
		list<T>(id)->add(entity);
		m_entityComponents[entity].set(id, true);
	}
	template<typename T> void remove_component(EntityId entity)
	{
		const char* typeName = typeid(T).name();
		assert(m_typeToId.contains(typeName));

		auto id = m_typeToId[typeName];
		list<T>(id)->remove(entity);
		m_entityComponents[entity].set(id, false);
	}
	template<typename T> T& get_component(EntityId entity)
	{
		const char* typeName = typeid(T).name();

		assert(m_typeToId.contains(typeName));

		auto id = m_typeToId[typeName];
		assert(m_entityComponents[entity].test(id));

		return list<T>(id)->get(entity);
	}
	std::bitset<ECS_MAX_COMPONENTS> used_components(EntityId entity)
	{
		if (!m_entityComponents.contains(entity))
			return std::bitset<ECS_MAX_COMPONENTS>();
		return m_entityComponents[entity];
	}

	void remove_entity(EntityId entity)
	{
		m_entityComponents.erase(entity);
	}
private:
	std::vector<IComponentList*> m_components; // indexed by ComponentTypeId
	std::unordered_map<const char*, ComponentTypeId> m_typeToId;
	std::unordered_map<EntityId, std::bitset<ECS_MAX_COMPONENTS>> m_entityComponents;

	template<typename T> ComponentList<T>* list(ComponentTypeId id)
	{
		return dynamic_cast<ComponentList<T>*>(m_components[id]);
	}
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
	virtual void start(ECSManager& ecs) {}
	virtual void awake(EntityId entity, ECSManager& ecs) {}
	virtual void update(float dt, ECSManager& ecs) {}
	virtual void update(float dt, EntityId entity, ECSManager& ecs) {}
	virtual std::vector<const char*> component_types() = 0;
	std::vector<EntityId> m_entities;
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
	std::vector<const char*> component_types() override
	{
		return m_types;
	}
private:
	std::vector<const char*> m_types;
};

class ECSManager
{
public:
	ECSManager() :
		m_maxEntities(0)
	{
		fill_available_entities();
	}
	~ECSManager()
	{
		for (size_t i = 0; i < m_systems.size(); i++)
			delete m_systems[i];
	}

	template<typename S> void register_system()
	{
		m_systems.push_back(new S());
		m_systemComponents.push_back(std::bitset<ECS_MAX_COMPONENTS>());

		const auto& systemTypeNames = m_systems.back()->component_types();

		for (auto typeName : systemTypeNames)
		{
			ensure_component(typeName);
			m_systemComponents.back().set(m_componentTypeToId[typeName], true);
		}

		m_systems.back()->start(*this);
	}
	EntityId create_entity()
	{
		if (m_availableEntities.size() == 0)
			fill_available_entities();

		EntityId entity = m_availableEntities.front();
		m_availableEntities.pop();

		m_newEntities.push_back(entity);

		return entity;
	}
	void delete_entity(EntityId entity)
	{
		m_availableEntities.push(entity);

		for (size_t i = 0; i < m_systems.size(); i++)
			std::erase(m_systems[i]->m_entities, entity);

		m_componentManager.remove_entity(entity);
		std::erase(m_newEntities, entity);
	}

	template<typename T> void add_component(EntityId entity)
	{
		m_componentManager.add_component<T>(entity);

		const char* typeName = typeid(T).name();
		ensure_component(typeName);

		ComponentTypeId id = m_componentTypeToId[typeName];
		auto entityComponents = used_components(entity);

		for (SystemId systemId = 0; systemId < m_systems.size(); systemId++)
			if ((entityComponents & m_systemComponents[systemId]) == m_systemComponents[systemId])
				m_systems[systemId]->m_entities.push_back(entity);
	}
	template<typename T> void remove_component(EntityId entity)
	{
		ComponentTypeId id = m_componentTypeToId[typeid(T).name()];
		auto entityComponents = used_components(entity);
		for (SystemId systemId = 0; systemId < m_systems.size(); systemId++)
			if ((entityComponents & m_systemComponents[systemId]) == m_systemComponents[systemId])
				std::erase(m_systems[systemId]->m_entities, entity);

		m_componentManager.remove_component<T>(entity);
	}
	template<typename T> T& get_component(EntityId entity)
	{
		return m_componentManager.get_component<T>(entity);
	}
	std::bitset<ECS_MAX_COMPONENTS> used_components(EntityId entity)
	{
		return m_componentManager.used_components(entity);
	}

	void update_systems(float dt)
	{
		if (m_newEntities.size() > 0)
		{
			awake_entities();
			m_newEntities.clear();
		}

		for (auto system : m_systems)
		{
			system->update(dt, *this);
			for (EntityId entity : system->m_entities)
				system->update(dt, entity, *this);
		}
	}
private:
	std::vector<ISystem*> m_systems;
	std::vector<std::bitset<ECS_MAX_COMPONENTS>> m_systemComponents; // bitset for all systems for used components
	std::unordered_map<const char*, ComponentTypeId> m_componentTypeToId;

	std::vector<EntityId> m_newEntities;
	void awake_entities()
	{
		for (auto system : m_systems)
		{
			std::vector<EntityId> newEntities;
			std::set_intersection(
				system->m_entities.begin(),
				system->m_entities.end(),
				m_newEntities.begin(),
				m_newEntities.end(),
				std::back_inserter(newEntities)
			);
			for (EntityId entity : newEntities)
				system->awake(entity, *this);
		}
	}

	std::queue<EntityId> m_availableEntities;
	size_t m_maxEntities;
	void fill_available_entities()
	{
		for (size_t i = m_maxEntities; i < m_maxEntities + ECS_START_ENTITIES; i++)
			m_availableEntities.push(i);
		m_maxEntities += ECS_START_ENTITIES;
	}

	ComponentManager m_componentManager;

	void ensure_component(const char* typeName)
	{
		if (!m_componentTypeToId.contains(typeName))
			m_componentTypeToId[typeName] = m_componentTypeToId.size();
	}
};