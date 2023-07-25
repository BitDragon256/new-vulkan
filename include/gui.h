#pragma once

#include <array>
#include <sstream>

template<typename T> struct gui_print_component;

#define GUI_PRINT_COMPONENT_START(TYPE) template<> struct gui_print_component<TYPE> { std::string operator()(const TYPE & component) {
#define GUI_PRINT_COMPONENT_END }};

#include "ecs.h"
#include "physics.h"

template<typename T>
struct gui_print_component
{
	std::string operator()(const T& component) const
	{
		return "no data";
	}
};

std::ostream& operator<< (std::ostream& os, const Vector3& vec);

template<>
struct gui_print_component<Transform>
{
	std::string operator()(const Transform& transform)
	{
		std::stringstream ss;
		ss << "position: " << transform.position.x << " | " << transform.position.y << " | " << transform.position.z << '\n';
		auto rot = transform.rotation.to_euler();
		ss << "rotation: " << rot.x << " | " << rot.y << " | " << rot.z << '\n';
		ss << "scale: " << transform.scale.x << " | " << transform.scale.y << " | " << transform.scale.z << '\n';

		return ss.str();
	}
};

GUI_PRINT_COMPONENT_START(Rigidbody)

std::stringstream ss;
ss	<< "pos: " << component.pos
	<< "vel: " << component.vel
	<< "acc: " << component.acc
	<< "mass: " << component.mass;
return ss.str();

GUI_PRINT_COMPONENT_END

class GUIManager
{
public:
	void initialize(ECSManager* ecs);
	void activate();

	void draw_entity_info(); // all entities with their respective components
	void draw_system_info(); // all systems and all affected entities
	void draw_scene_info(); // all transforms

	std::array<float, 4> viewport(std::array<float, 4> defaultViewport);
private:
	ECSManager* m_ecs;
	bool m_activated;
};