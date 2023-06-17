#include <stdio.h>

#include "render.h"

int main(int argc, char** argv)
{
    Renderer renderer;
    RenderConfig renderConfig;
    renderConfig.width = 1400;
    renderConfig.height = 1000;
    renderConfig.title = "Vulkan";
    renderConfig.dataMode = RenderConfig::DataMode::Indexed;
    renderConfig.cameraEnabled = true;
    renderConfig.useModelHandler = true;
    renderConfig.clearColor = glm::vec3(0, 187, 233);

    renderConfig.enabledValidationLayers.push_back("VK_LAYER_KHRONOS_validation");

    renderer.init(renderConfig);
    
    // Models

    ModelHandler modelHandler;
    renderer.bind_model_handler(&modelHandler);

    Model cube = Model::create_model(Mesh::create_cube());
    cube.m_info.position = glm::vec3(0, 1, 0);

    modelHandler.add_model(&cube);

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