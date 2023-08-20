#include "gui.h"

#include <imgui.h>

#include "ecs.h"

void GUIManager::initialize(ECSManager* ecs)
{
	m_ecs = ecs;
	m_activated = false;
}
void GUIManager::activate()
{
	m_activated = true;
}

void GUIManager::draw_entity_info()
{
	if (!m_activated)
		return;

	ImGui::Begin("Entity Info");

	const auto& allEntities = m_ecs->entities();

	size_t labelIndex = 0;
	for (auto entity : allEntities)
	{
		std::stringstream labelSS;
		labelSS << "Entity " << entity;
		labelIndex++;

		if (ImGui::TreeNode(labelSS.str().c_str()))
		{
			auto usedComponents = m_ecs->used_components(entity);
			for (size_t i = 0; i < usedComponents.size(); i++)
			{
				if (usedComponents[i])
				{
					auto componentType = m_ecs->m_componentManager.m_components[i]->print_type();
					if (ImGui::TreeNode(componentType.c_str()))
					{
						m_ecs->m_componentManager.m_components[i]->gui_show_component(entity);
						ImGui::TreePop();
					}
				}
			}
			ImGui::TreePop();
		}
	}

	ImGui::End();
}
void GUIManager::draw_system_info()
{
	if (!m_activated)
		return;

	ImGui::Begin("System Info");

	const auto& systems = m_ecs->m_systems;
	for (auto system : systems)
	{
		if (ImGui::TreeNode(system->type_name()))
		{
			ImGui::SeparatorText("Entities");

			const auto& entities = system->m_entities;
			if (entities.empty())
				ImGui::Text("No Entities");

			for (auto entity : entities)
			{
				std::stringstream label; label << "Entity " << entity;
				if (ImGui::TreeNode(label.str().c_str()))
				{
					for (auto typeName : system->component_types())
					{
						if (ImGui::TreeNode(typeName))
						{
							m_ecs->m_componentManager.m_components[m_ecs->m_componentManager.m_typeToId[typeName]]->gui_show_component(entity);
							ImGui::TreePop();
						}
					}

					ImGui::TreePop();
				}
			}

			ImGui::TreePop();
		}
	}

	ImGui::End();
}

std::array<float, 4> GUIManager::viewport(std::array<float, 4> defaultViewport)
{
	if (!m_activated)
		return defaultViewport;
	return { 0, 0, defaultViewport[2] * 0.7f, defaultViewport[3] * 0.7f};
}

std::ostream& operator<< (std::ostream& ostream, const Vector3& vec)
{
	ostream << vec.x << " | " << vec.y << " | " << vec.z << '\n';
	return ostream;
}

void ImGui_DragVector(const char* label, Vector3& vec) {
	float vector[] = { vec.x, vec.y, vec.z };
	ImGui::DragFloat3(label, vector);
	vec = { vector[0], vector[1], vector[2] };
}