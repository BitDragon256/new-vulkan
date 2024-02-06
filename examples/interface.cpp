#include <stdio.h>

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
      for (int i = 0; i < 5000; i++)
      {
            auto res = renderer.render();
            if (res == NVE_RENDER_EXIT_SUCCESS)
                  break;
            std::cout << "\r" << i;
      }
      std::cout << "\n";
      renderer.clean_up();

      return 0;
}