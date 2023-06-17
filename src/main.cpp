#include <stdio.h>

#include "render.h"

int main(int argc, char** argv)
{
    Renderer renderer;
    RenderConfig renderConfig;
    renderConfig.width = 1400;
    renderConfig.height = 1000;
    renderConfig.title = "Vulkan";
    renderConfig.dataMode = RenderConfig::Indexed;
    renderConfig.enableValidationLayers = true;
    renderConfig.clearColor = glm::vec3(0, 187, 233);

    renderer.init(renderConfig);

    renderer.set_vertices({ { { 0, -0.5, 0 } }, { { 0, -0.5, 0 } }, { { 0, -0.5, 0 } } });
    renderer.set_indices({ 0, 1, 2, 0, 2, 1 });
    
    // Models

    ModelHandler modelHandler;
    //renderer.bind_model_handler(&modelHandler);

    Model cube = Model::create_model(Mesh::create_cube());
    cube.m_info.position = glm::vec3(0, 0, 0);

    Model triangle = Model::create_model(Mesh::create_triangle());
    triangle.m_info.position = glm::vec3(0, 0, 0);

    modelHandler.add_model(&triangle);

    // Camera

    Camera camera;
    camera.m_position = glm::vec3(-3, 0, 0);
    renderer.set_active_camera(&camera);

    bool running = true;
    while (running)
    {
        NVE_RESULT res = renderer.render();
        if (res == NVE_RENDER_EXIT_SUCCESS)
            running = false;
    }

    return 0;
}