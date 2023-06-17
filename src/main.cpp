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
    renderConfig.useModelHandler = true;
    renderConfig.cameraEnabled = true;

    renderer.init(renderConfig);
    
    // Models

    ModelHandler modelHandler;
    renderer.bind_model_handler(&modelHandler);

    Model triangle = Model::create_model(Mesh::create_triangle());
    triangle.m_info.position = glm::vec3(0, 0, 0);

    Model cube = Model::create_model(Mesh::create_cube());

    modelHandler.add_model(&cube);

    // Camera

    Camera camera;
    camera.m_position = glm::vec3(-1, 0, -1);
    camera.m_rotation = glm::vec3(0, 0, 0);
    renderer.set_active_camera(&camera);

    float turningSpeed = 0.2f;
    float moveSpeed = 0.01f;

    bool running = true;
    while (running)
    {
        if (renderer.get_key(GLFW_KEY_W) == GLFW_PRESS)
            camera.m_position.x += moveSpeed;
        if (renderer.get_key(GLFW_KEY_S) == GLFW_PRESS)
            camera.m_position.x -= moveSpeed;

        if (renderer.get_key(GLFW_KEY_A) == GLFW_PRESS)
            camera.m_position.y += moveSpeed;
        if (renderer.get_key(GLFW_KEY_D) == GLFW_PRESS)
            camera.m_position.y -= moveSpeed;

        if (renderer.get_key(GLFW_KEY_E) == GLFW_PRESS)
            camera.m_position.z -= moveSpeed;
        if (renderer.get_key(GLFW_KEY_Q) == GLFW_PRESS)
            camera.m_position.z += moveSpeed;

        if (renderer.get_key(GLFW_KEY_UP) == GLFW_PRESS)
            camera.m_rotation.y += turningSpeed;
        if (renderer.get_key(GLFW_KEY_DOWN) == GLFW_PRESS)
            camera.m_rotation.y -= turningSpeed;

        if (renderer.get_key(GLFW_KEY_LEFT) == GLFW_PRESS)
            camera.m_rotation.z += turningSpeed;
        if (renderer.get_key(GLFW_KEY_RIGHT) == GLFW_PRESS)
            camera.m_rotation.z -= turningSpeed;

        NVE_RESULT res = renderer.render();
        if (res == NVE_RENDER_EXIT_SUCCESS)
            running = false;
    }

    return 0;
}