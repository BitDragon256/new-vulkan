#pragma once

#include <unordered_map>

#include "nve_types.h"
#include "gui.h"

// ----------------------------------------------------------------------
// TRANSFORM
// ----------------------------------------------------------------------

static std::unordered_map<Transform*, Vector3> absRotations;

GUI_PRINT_COMPONENT_START(Transform)

ImGui_DragVector("position", component.position);
Vector3 rot;
if (!absRotations.contains(&component))
	rot = component.rotation.to_euler();
else
	rot = absRotations[&component];

Vector3 lastRot = rot;
ImGui_DragVector("rotation", rot);
absRotations[&component] = rot;
if (lastRot != rot)
	component.rotation.euler(rot);
ImGui_DragVector("scale", component.scale);

GUI_PRINT_COMPONENT_END

