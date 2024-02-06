#include <stdio.h>

#include <nve.h>

int main(int argc, char** argv)
{
      Renderer renderer;
      RenderConfig renderConfig;
      renderConfig.title = "";
      renderer.init(renderConfig);
      const auto cube = renderer.create_default_model(DefaultModel::Cube);
      renderer.m_ecs.get_component<DynamicModel>(cube).m_children.front().material->m_shader->fragment.load_shader("fragments/lamb_wmat.frag.spv");
      renderer.active_camera().m_position = Vector3{ -2.f, 0.f, 1.f };
      renderer.active_camera().m_rotation = Vector3{ 0.f, 20.f, 0.f };
      for (int i = 0; i < 10000; i++)
            renderer.render();
      renderer.clean_up();

      return 0;
}