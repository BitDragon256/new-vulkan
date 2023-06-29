//#include "render.h"
//
//const float turningSpeed = 0.5f;
//const float moveSpeed = 0.015f;
//
//void camera_movement(Renderer& renderer, Camera& camera)
//{
//    Vector3 forward = { cos(glm::radians(camera.m_rotation.z)), sin(glm::radians(camera.m_rotation.z)), 0 };
//    Vector3 right = { -sin(glm::radians(camera.m_rotation.z)), cos(glm::radians(camera.m_rotation.z)), 0 };
//
//    if (renderer.get_key(GLFW_KEY_W) == GLFW_PRESS)
//        camera.m_position += forward * moveSpeed;
//    if (renderer.get_key(GLFW_KEY_S) == GLFW_PRESS)
//        camera.m_position -= forward * moveSpeed;
//
//    if (renderer.get_key(GLFW_KEY_A) == GLFW_PRESS)
//        camera.m_position += right * moveSpeed;
//    if (renderer.get_key(GLFW_KEY_D) == GLFW_PRESS)
//        camera.m_position -= right * moveSpeed;
//
//    if (renderer.get_key(GLFW_KEY_E) == GLFW_PRESS)
//        camera.m_position.z -= moveSpeed;
//    if (renderer.get_key(GLFW_KEY_Q) == GLFW_PRESS)
//        camera.m_position.z += moveSpeed;
//
//    if (renderer.get_key(GLFW_KEY_UP) == GLFW_PRESS)
//        camera.m_rotation.y += turningSpeed;
//    if (renderer.get_key(GLFW_KEY_DOWN) == GLFW_PRESS)
//        camera.m_rotation.y -= turningSpeed;
//
//    if (renderer.get_key(GLFW_KEY_LEFT) == GLFW_PRESS)
//        camera.m_rotation.z += turningSpeed;
//    if (renderer.get_key(GLFW_KEY_RIGHT) == GLFW_PRESS)
//        camera.m_rotation.z -= turningSpeed;
//
//    camera.m_rotation.y = math::clamp(camera.m_rotation.y, -50, 50);
//}
//
//int main()
//{
//    Renderer renderer;
//    RenderConfig renderConfig;
//    renderConfig.width = 1920;
//    renderConfig.height = 1080;
//    renderConfig.title = "Render Loop Example";
//    renderConfig.dataMode = RenderConfig::Indexed;
//    renderConfig.enableValidationLayers = true;
//    renderConfig.clearColor = Vector3(0, 187, 233);
//    renderConfig.useModelHandler = true;
//    renderConfig.cameraEnabled = true;
//
//    renderer.init(renderConfig);
//
//    // Models
//
//    ModelHandler modelHandler;
//    renderer.bind_model_handler(&modelHandler);
//
//    Model cube = Model::create_model(Mesh::create_cube());
//    modelHandler.add_model(&cube);
//
//    // Camera
//
//    Camera camera;
//    camera.m_position = Vector3(-1, 0, -1);
//    camera.m_rotation = Vector3(0, 0, 0);
//    renderer.set_active_camera(&camera);
//
//    bool running = true;
//    while (running)
//    {
//        camera_movement(renderer, camera);
//
//        NVE_RESULT res = renderer.render();
//        if (res == NVE_RENDER_EXIT_SUCCESS)
//            running = false;
//    }
//
//    return 0;
//}

int main()
{
	return 0;
}