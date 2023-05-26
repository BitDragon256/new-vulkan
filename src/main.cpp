#include <stdio.h>

#include "render.h"

int main(int argc, char** argv)
{
    Renderer renderer;
    RenderConfig renderConfig;
    renderConfig.width = 800;
    renderConfig.height = 500;
    renderConfig.title = "Vulkan";

    renderer.init(renderConfig);
    renderer.clean_up();

    return 0;
}