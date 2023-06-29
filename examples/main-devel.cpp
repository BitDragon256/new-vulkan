#include <stdio.h>

#include <chrono>
#include <sstream>

#include <nve.h>

float turningSpeed = 0.2f;
float moveSpeed = 0.01f;
void camera_movement(Renderer& renderer, Camera& camera)
{
    Vector3 forward = { cos(glm::radians(camera.m_rotation.z)), sin(glm::radians(camera.m_rotation.z)), 0 };
    Vector3 right = { -sin(glm::radians(camera.m_rotation.z)), cos(glm::radians(camera.m_rotation.z)), 0 };

    if (renderer.get_key(GLFW_KEY_W) == GLFW_PRESS)
        camera.m_position += forward * moveSpeed;
    if (renderer.get_key(GLFW_KEY_S) == GLFW_PRESS)
        camera.m_position -= forward * moveSpeed;

    if (renderer.get_key(GLFW_KEY_A) == GLFW_PRESS)
        camera.m_position += right * moveSpeed;
    if (renderer.get_key(GLFW_KEY_D) == GLFW_PRESS)
        camera.m_position -= right * moveSpeed;

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

    camera.m_rotation.y = math::clamp(camera.m_rotation.y, -50, 50);
}

int main(int argc, char** argv)
{
    Renderer renderer;
    RenderConfig renderConfig;
    renderConfig.width = 1920;
    renderConfig.height = 1080;
    renderConfig.title = "Vulkan";
    renderConfig.dataMode = RenderConfig::Indexed;
    renderConfig.enableValidationLayers = true;
    renderConfig.clearColor = Vector3(0, 187, 233);
    renderConfig.cameraEnabled = true;

    renderer.init(renderConfig);

    // Camera

    Camera camera;
    camera.m_position = Vector3(-1, 0, -1);
    camera.m_rotation = Vector3(0, 0, 0);
    renderer.set_active_camera(&camera);

    float cubePosition[3]{ 0, 0, 0 };
    float cubeScale[3]{ 1, 1, 1 };
    float cubeEulerRotation[3]{ 0, 0, 0 };

    int fps = 0;
    int avgFps = 0;
    std::string fpsText;
    float deltaTime;
    auto lastTime = std::chrono::high_resolution_clock::now();

    auto testEntity = renderer.m_ecs.create_entity();
    renderer.m_ecs.add_component<Transform>(testEntity);
    renderer.m_ecs.add_component<StaticMesh>(testEntity);
    auto& testEntityMesh = renderer.m_ecs.get_component<StaticMesh>(testEntity);
    testEntityMesh.vertices = {
        { { -0.5f, -0.5f,  0.0f }, { 1.f, 0.f, 0.f }, { 0.f, 0.f } },
        { {  0.5f,  0.0f,  0.0f }, { 0.f, 1.f, 0.f }, { 0.f, 0.f } },
        { { -0.5f,  0.5f,  0.0f }, { 0.f, 0.f, 1.f }, { 0.f, 0.f } },
    };
    testEntityMesh.indices = {
        0, 1, 2,
        0, 2, 1,
    };
    testEntityMesh.material.m_fragmentShader.load_shader("static_geometry/default_unlit.frag.spv");
    testEntityMesh.material.m_vertexShader.load_shader("static_geometry/default_unlit.vert.spv");

    auto& testEntityTransform = renderer.m_ecs.get_component<Transform>(testEntity);

    bool running = true;
    while (running)
    {
        camera_movement(renderer, camera);

        renderer.gui_begin();

        ImGui::Begin("FPS");

        fpsText = std::to_string(fps);
        ImGui::Text(fpsText.c_str());
        fpsText = std::to_string(avgFps);
        ImGui::Text(fpsText.c_str());

        ImGui::End();

        ImGui::Begin("Cube Transform");

        ImGui::SliderFloat3("position", cubePosition, -1, 1);
        ImGui::SliderFloat3("scale", cubeScale, 0, 10);
        ImGui::SliderFloat3("euler rotation", cubeEulerRotation, -PI, PI);

        /*cube.m_info.position.x = cubePosition[0];
        cube.m_info.position.y = cubePosition[1];
        cube.m_info.position.z = cubePosition[2];

        cube.m_info.scale.x = cubeScale[0];
        cube.m_info.scale.y = cubeScale[1];
        cube.m_info.scale.z = cubeScale[2];*/

        Vector3 EulerAngle(cubeEulerRotation[0], cubeEulerRotation[1], cubeEulerRotation[2]);

        Quaternion finalOrientation(EulerAngle);

        //cube.m_info.rotation = finalOrientation;

        std::stringstream quatDisplay;
        quatDisplay << finalOrientation.x << " " << finalOrientation.y << " " << finalOrientation.z << " " << finalOrientation.w;
        ImGui::Text(quatDisplay.str().c_str());

        ImGui::End();

        NVE_RESULT res = renderer.render();
        if (res == NVE_RENDER_EXIT_SUCCESS)
            running = false;

        // time
        auto now = std::chrono::high_resolution_clock::now();
        deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTime).count();
        fps = 1000 / deltaTime;
        avgFps += fps;
        avgFps /= 2;
        lastTime = std::chrono::high_resolution_clock::now();
    }

    return 0;
}