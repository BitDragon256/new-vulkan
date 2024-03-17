#include <stdio.h>

#include <chrono>
#include <functional>

#include <nve.h>

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
      auto time = std::chrono::high_resolution_clock::now();
      auto delta = [&]() {
            auto cur = std::chrono::high_resolution_clock::now();
            auto last = time;
            time = cur;
            return std::chrono::duration_cast<std::chrono::microseconds>(cur - last).count() / 1000000.f;
      };
      float total = 0.f;
      while (true)
      {
            auto res = renderer.render();
            if (res == NVE_RENDER_EXIT_SUCCESS || total >= 10.f)
                  break;
            total += delta();
            if (((int) total) % 2 == 0)
            {
                  renderer.m_ecs.get_component<DynamicModel>(cube).set_fragment_shader("fragments/lamb_wmat.frag.spv");
                  renderer.reload_pipelines();
            }
            else if (((int) total) % 2 == 1)
            {
                  renderer.m_ecs.get_component<DynamicModel>(cube).set_fragment_shader("fragments/unlit_wmat.frag.spv");
                  renderer.reload_pipelines();
            }
      }
      std::cout << "\n";
      renderer.clean_up();

      return 0;
}