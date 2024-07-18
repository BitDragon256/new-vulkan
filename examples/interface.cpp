#include <stdio.h>

#include <chrono>
#include <functional>

#include <nve.h>
#include <glm/gtc/quaternion.hpp>
#include <vulkan/vulkan_handles.h>

int main(int argc, char** argv)
{
      RenderConfig renderConfig;
      renderConfig.title = "";
      Renderer renderer;
      renderer.init(renderConfig);

      const auto root = renderer.create_default_model(DefaultModel::Sphere);
      const auto beam = renderer.create_default_model(DefaultModel::Sphere);

      auto& rootTransform = renderer.m_ecs.get_component<Transform>(root);
      auto& midTransform = renderer.m_ecs.get_component<Transform>(beam);

      renderer.m_ecs.get_component<DynamicModel>(root).set_fragment_shader("fragments/unlit_wmat.frag.spv");
      renderer.m_ecs.get_component<DynamicModel>(beam).set_fragment_shader("fragments/unlit_wmat.frag.spv");
      midTransform.position.x = 2.f;

      const float beamLength = 2.f;
      const float wantedAngle = 0.f;
      const float damping = 0.5f;
      const float rotationalDamping = 0.97f;
      auto lastPos = midTransform.position;

      renderer.active_camera().m_position = Vector3{ 0.f, 0.f, 3.f };
      renderer.active_camera().m_rotation = Vector3{ 0.f, 90.f, 0.f };
      renderer.active_camera().m_orthographic = true;

      while (renderer.render() != NVE_RENDER_EXIT_SUCCESS)
      {
            const auto tempPos = midTransform.position;
            float gravity = 0.f;
            if (renderer.get_key(GLFW_KEY_SPACE))
                  gravity = 1000.f;
            midTransform.position += midTransform.position - lastPos + std::pow(renderer.delta_time(), 2.f) * Vector3(0.f,gravity,0.f);
            lastPos = tempPos;
            for (int simSteps = 0; simSteps < 10; ++simSteps)
            {
                  midTransform.position -= (1.f - damping) * (glm::length(midTransform.position) - beamLength) * glm::normalize(midTransform.position);
                  const auto angle = std::atan2(-midTransform.position.y, midTransform.position.x);
                  midTransform.position += Vector3(-midTransform.position.y, midTransform.position.x, 0.f) * (angle - wantedAngle) * (1.f - rotationalDamping);
            }
      }
      std::cout << "\n";
      renderer.clean_up();

      return 0;
}