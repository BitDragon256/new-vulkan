#pragma once

#include <vector>

#include "nve_types.h"
#include "model-handler.h"
#include "ecs.h"

struct GizmosModel : Model
{

};

class GizmosHandler : public GeometryHandler, System<GizmosModel, Transform>
{
public:
      
      void draw_line(Vector3 start, Vector3 end, Color color, float width = 0.1f);
      void draw_ray(Vector3 start, Vector3 direction, Color color, float width = 0.1f);

private:

	// -----------------------------------
	// GEOMETRY HANDLER STUFF
	// -----------------------------------

public:

	GizmosHandler();
	void initialize(GeometryHandlerVulkanObjects vulkanObjects, GUIManager* guiManager);
	void awake(EntityId entity) override;
	void update(float dt) override;

protected:

	void record_command_buffer(uint32_t subpass, size_t frame, const MeshGroup& meshGroup, size_t meshGroupIndex) override;
	std::vector<VkDescriptorSetLayoutBinding> other_descriptors() override;

private:

	void add_model(GizmosModel& model, Transform& transform);
};