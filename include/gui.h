#pragma once

#include <array>
#include <sstream>

#include "nve_types.h"

template<typename T> struct gui_print_component;

#define GUI_PRINT_COMPONENT_START(TYPE) template<> struct gui_print_component<TYPE> { std::string operator()(const TYPE & component) {
#define GUI_PRINT_COMPONENT_END }};

template<typename T>
struct gui_print_component
{
	std::string operator()(const T& component) const
	{
		return "no data";
	}
};

std::ostream& operator<< (std::ostream& os, const Vector3& vec);

class ECSManager;

class GUIManager
{
public:
	void initialize(ECSManager* ecs);
	void activate();

	void draw_entity_info(); // all entities with their respective components
	void draw_system_info(); // all systems and all affected entities

	std::array<float, 4> viewport(std::array<float, 4> defaultViewport);
private:
	ECSManager* m_ecs;
	bool m_activated;
};