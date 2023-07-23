#pragma once

#include "ecs.h"

class GUIManager
{
public:
	void draw_entity_info(); // all entities with their respective components
	void draw_system_info(); // all systems and all affected entities
	void draw_scene_info(); // all transforms
private:
	ECSManager* m_ecs;
};