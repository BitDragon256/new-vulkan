#include <stdio.h>

#include <nve.h>

int main(int argc, char** argv)
{
      Renderer renderer;
      RenderConfig renderConfig;
      renderer.init(renderConfig);
      for (int i = 0; i < 2000; i++)
            renderer.render();
      renderer.clean_up();

      return 0;
}