#include <stdio.h>

#include "render.h"
#include "vulkan_types.h"

int main(int argc, char** argv)
{
    Renderer renderer;
    RenderConfig renderConfig;
    renderConfig.width = 800;
    renderConfig.height = 500;
    renderConfig.title = "Vulkan";

    renderer.init(renderConfig);

    bool running = true;
    while (running)
    {
        NVE_RESULT res = renderer.render();
        if (res == NVE_RENDER_EXIT_SUCCESS)
            running = false;
    }

    return 0;
}