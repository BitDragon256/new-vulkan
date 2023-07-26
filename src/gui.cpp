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

	std::string label;

	const auto& allEntities = m_ecs->entities();

	ImGui::Begin("Entity Info");
	ImGui::Text("Entity Info");

	size_t labelIndex = 0;
	for (auto entity : allEntities)
	{
		std::stringstream labelSS;
		labelSS << "Entity " << entity << "##" << labelIndex;
		labelIndex++;

		if (ImGui::TreeNode(labelSS.str().c_str()))
		{
			auto usedComponents = m_ecs->used_components(entity);
			for (size_t i = 0; i < usedComponents.size(); i++)
			{
				if (usedComponents[i])
				{
					auto componentData = m_ecs->m_componentManager.m_components[i]->print_component(entity);
					auto componentType = m_ecs->m_componentManager.m_components[i]->print_type();
					if (ImGui::TreeNode(componentType.c_str()))
					{
						ImGui::Text(componentData.c_str());
						ImGui::TreePop();
					}
				}
			}
			ImGui::TreePop();
		}
	}

	ImGui::End();
}

std::array<float, 4> GUIManager::viewport(std::array<float, 4> defaultViewport)
{
	if (m_activated)
		return defaultViewport;
	return { 0, 0, defaultViewport[2] * 0.7f, defaultViewport[3] * 0.7f};
}

std::ostream& operator<< (std::ostream& ostream, const Vector3& vec)
{
	ostream << vec.x << " | " << vec.y << " | " << vec.z << '\n';
	return ostream;
}