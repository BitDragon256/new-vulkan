#include <stdio.h>

#include "render.h"

int main(int argc, char** argv)
{
    Renderer renderer;
    RenderConfig renderConfig;
    renderConfig.width = 800;
    renderConfig.height = 500;
    renderConfig.title = "Vulkan";
    renderConfig.vertexIndexMode = true;

    renderer.init(renderConfig);
    renderer.set_vertices(std::vector<Vertex>
    {
        { {0.5f, -0.5f}, { 1.0f, 0.0f, 0.0f }},
        { {0.5f, 0.5f}, {0.0f, 1.0f, 0.0f} },
        { {-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f} },

        { {-0.5f, -0.5f}, {0.0f, 1.0f, 0.0f} },
        { {0.5f, -0.5f}, { 1.0f, 0.0f, 0.0f } },
        { {-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f} }
    });

    bool running = true;
    while (running)
    {
        NVE_RESULT res = renderer.render();
        if (res == NVE_RENDER_EXIT_SUCCESS)
            running = false;
    }

    return 0;
}