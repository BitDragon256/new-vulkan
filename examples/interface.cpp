#include <chrono>
#include <functional>
#include <iostream>

#include <nve.h>
#include <vulkan/vulkan_handles.h>

int main(int argc, char** argv)
{
      RenderConfig renderConfig;
      renderConfig.title = "";
      Renderer renderer;
      renderer.init(renderConfig);
      const auto cube = renderer.create_default_model(DefaultModel::Cube);
      renderer.m_ecs.get_component<DynamicModel>(cube).set_fragment_shader("fragments/lamb_wmat.frag.spv");
      renderer.active_camera().m_position = Vector3{ -2.f, 0.f, 1.f };
      renderer.active_camera().m_rotation = Vector3{ 0.f, 20.f, 0.f };
      float total = 0.f;
      while (true)
      {
            auto res = renderer.render();
            if (res == NVE_RENDER_EXIT_SUCCESS || total >= 10.f)
                  break;
      }
      std::cout << "\n";
      renderer.clean_up();

      return 0;
}