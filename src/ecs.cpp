#include "ecs.h"

#include <assert.h>

// -----------------------------------
// ComponentManager
// -----------------------------------

ComponentManager::~ComponentManager()
{
	for (size_t i = 0; i < m_components.size(); i++)
	{
		delete m_components[i];
	}
}

template<typename T>
void ComponentManager::add_component(EntityId entity)
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

template<typename T>
void ComponentManager::remove_component(EntityId entity)
{
	const char* typeName = typeid(T).name();
	assert(m_typeToId.contains(typeName));

	auto id = m_typeToId[typeName];
	list<T>(id)->remove(entity);
	m_entityComponents[entity].set(id, false);
}

template<typename T>
T& ComponentManager::get_component(EntityId entity)
{
	const char* typeName = typeid(T).name();

	assert(m_typeToId.contains(typeName));

	auto id = m_typeToId[typeName];
	assert(m_entityComponents[entity].test(id));

	return list<T>(id)->get(entity);
}

std::bitset<ECS_MAX_COMPONENTS> ComponentManager::used_components(EntityId entity)
{
	if (!m_entityComponents.contains(entity))
		return std::bitset<ECS_MAX_COMPONENTS>();
	return m_entityComponents[entity];
}

template<typename T>
ComponentList<T>* ComponentManager::list(ComponentTypeId id)
{
	return dynamic_cast<ComponentList<T>*>(m_components[id]);
}

void ComponentManager::remove_entity(EntityId entity)
{
	m_entityComponents.erase(entity);
}

// -----------------------------------
// ComponentList
// -----------------------------------

template<typename T>
void ComponentList<T>::add(EntityId entity)
{
	assert(!m_entityToIndex.contains(entity));

	m_entityToIndex[entity] = m_components.size();
	m_components.push_back(T());
	m_entities.push_back(entity);
}

template<typename T>
void ComponentList<T>::remove(EntityId entity)
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

template<typename T>
T& ComponentList<T>::get(EntityId entity)
{
	assert(m_entityToIndex.contains(entity));

	return m_components[m_entityToIndex[entity]];
}

// -----------------------------------
// ECSManager
// -----------------------------------

ECSManager::ECSManager() :
	m_maxEntities(0)
{
	fill_available_entities();
}
ECSManager::~ECSManager()
{
	for (size_t i = 0; i < m_systems.size(); i++)
		delete m_systems[i];
}

template<typename S>
void ECSManager::register_system()
{
	m_systems.push_back(new S());
	m_systemEntities.push_back(std::vector<EntityId>());
	m_systemComponents.push_back(std::bitset<ECS_MAX_COMPONENTS>());

	const auto& systemTypeNames = m_systems.back()->component_types();

	for (auto typeName : systemTypeNames)
	{
		ensure_component(typeName);
		m_systemComponents.back().set(m_componentTypeToId[typeName], true);
	}
}
EntityId ECSManager::create_entity()
{
	if (m_availableEntities.size() == 0)
		fill_available_entities();

	EntityId entity = m_availableEntities.front();
	m_availableEntities.pop();

	return entity;
}
void ECSManager::delete_entity(EntityId entity)
{
	m_availableEntities.push(entity);

	for (size_t i = 0; i < m_systemEntities.size(); i++)
		std::erase(m_systemEntities[i], entity);

	m_componentManager.remove_entity(entity);
}

void ECSManager::ensure_component(const char* typeName)
{
	if (!m_componentTypeToId.contains(typeName))
		m_componentTypeToId[typeName] = m_componentTypeToId.size();
}

template<typename T> void ECSManager::add_component(EntityId entity)
{
	m_componentManager.add_component<T>(entity);

	const char* typeName = typeid(T).name();
	ensure_component(typeName);

	ComponentTypeId id = m_componentTypeToId[typeName];
	auto entityComponents = used_components(entity);

	for (SystemId system = 0; system < m_systems.size(); system++)
		if ((entityComponents & m_systemComponents[system]) == m_systemComponents[system])
			m_systemEntities[system].push_back(entity);
}
template<typename T> void ECSManager::remove_component(EntityId entity)
{
	ComponentTypeId id = m_componentTypeToId[typeid(T).name()];
	auto entityComponents = used_components(entity);
	for (SystemId system = 0; system < m_systems.size(); system++)
		if ((entityComponents & m_systemComponents[system]) == m_systemComponents[system])
			std::erase(m_systemEntities[system], entity);

	m_componentManager.remove_component<T>(entity);
}
template<typename T> T& ECSManager::get_component(EntityId entity)
{
	return m_componentManager.get_component<T>(entity);
}
std::bitset<ECS_MAX_COMPONENTS> ECSManager::used_components(EntityId entity)
{
	return m_componentManager.used_components(entity);
}

void ECSManager::update_systems(float dt)
{
	for (SystemId systemId = 0; systemId < m_systems.size(); systemId++)
		for (EntityId entity : m_systemEntities[systemId])
			m_systems[systemId]->update(dt, entity, *this);
}

void ECSManager::fill_available_entities()
{
	for (size_t i = m_maxEntities; i < m_maxEntities + ECS_START_ENTITIES; i++)
		m_availableEntities.push(i);
	m_maxEntities += ECS_START_ENTITIES;
}

// -----------------------------------
// System
// -----------------------------------

template<typename... Types>
constexpr const std::vector<const char*>& System<Types...>::component_types() const
{
	return m_types;
}