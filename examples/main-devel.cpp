#include <stdio.h>

#include <chrono>
#include <sstream>

#include <nve.h>

float turningSpeed = 0.5f;
float moveSpeed = 0.008f;
float camDistance = 5;
Vector3 camPos = { 0,0,0 };
Vector3 camRot = { 0,0,0 };

void str_camera_movement(Renderer& renderer, Camera& camera)
{
    Vector3 forward = { cos(glm::radians(camRot.z)), sin(glm::radians(camRot.z)), 0 };
    Vector3 right = { -sin(glm::radians(camRot.z)), cos(glm::radians(camRot.z)), 0 };

    if (renderer.get_key(GLFW_KEY_W) == GLFW_PRESS)
        camPos += forward * moveSpeed;
    if (renderer.get_key(GLFW_KEY_S) == GLFW_PRESS)
        camPos -= forward * moveSpeed;

    if (renderer.get_key(GLFW_KEY_A) == GLFW_PRESS)
        camPos -= right * moveSpeed;
    if (renderer.get_key(GLFW_KEY_D) == GLFW_PRESS)
        camPos += right * moveSpeed;

    if (renderer.get_key(GLFW_KEY_E) == GLFW_PRESS)
        camPos.z += moveSpeed;
    if (renderer.get_key(GLFW_KEY_Q) == GLFW_PRESS)
        camPos.z -= moveSpeed;

    if (renderer.get_key(GLFW_KEY_UP) == GLFW_PRESS)
        camRot.y -= turningSpeed;
    if (renderer.get_key(GLFW_KEY_DOWN) == GLFW_PRESS)
        camRot.y += turningSpeed;

    if (renderer.get_key(GLFW_KEY_LEFT) == GLFW_PRESS)
        camRot.z -= turningSpeed;
    if (renderer.get_key(GLFW_KEY_RIGHT) == GLFW_PRESS)
        camRot.z += turningSpeed;

    camRot.y = math::clamp(camRot.y, 0, 90);

    camera.m_position = camPos + Vector3 { -cos(glm::radians(camRot.z)), -sin(glm::radians(camRot.z)), sin(glm::radians(camRot.y))} * camDistance;
    camera.m_rotation = { camRot.x, camRot.y, camRot.z };
}
void fps_camera_movement(Renderer& renderer, Camera& camera)
{
    Vector3 forward = { cos(glm::radians(camera.m_rotation.z)), sin(glm::radians(camera.m_rotation.z)), 0 };
    Vector3 right = { -sin(glm::radians(camera.m_rotation.z)), cos(glm::radians(camera.m_rotation.z)), 0 };

    if (renderer.get_key(GLFW_KEY_W) == GLFW_PRESS)
        camera.m_position += forward * moveSpeed;
    if (renderer.get_key(GLFW_KEY_S) == GLFW_PRESS)
        camera.m_position -= forward * moveSpeed;

    if (renderer.get_key(GLFW_KEY_A) == GLFW_PRESS)
        camera.m_position -= right * moveSpeed;
    if (renderer.get_key(GLFW_KEY_D) == GLFW_PRESS)
        camera.m_position += right * moveSpeed;

    if (renderer.get_key(GLFW_KEY_E) == GLFW_PRESS)
        camera.m_position.z += moveSpeed * 10;
    if (renderer.get_key(GLFW_KEY_Q) == GLFW_PRESS)
        camera.m_position.z -= moveSpeed * 10;

    if (renderer.get_key(GLFW_KEY_UP) == GLFW_PRESS)
        camera.m_rotation.y -= turningSpeed;
    if (renderer.get_key(GLFW_KEY_DOWN) == GLFW_PRESS)
        camera.m_rotation.y += turningSpeed;

    if (renderer.get_key(GLFW_KEY_LEFT) == GLFW_PRESS)
        camera.m_rotation.z -= turningSpeed;
    if (renderer.get_key(GLFW_KEY_RIGHT) == GLFW_PRESS)
        camera.m_rotation.z += turningSpeed;

    camera.m_rotation.y = math::clamp(camera.m_rotation.y, -50, 50);
}
void cart_camera_movement(Renderer& renderer, Camera& camera)
{
    if (renderer.get_key(GLFW_KEY_W) == GLFW_PRESS)
        camera.m_position += VECTOR_FORWARD * moveSpeed;
    if (renderer.get_key(GLFW_KEY_S) == GLFW_PRESS)
        camera.m_position -= VECTOR_FORWARD * moveSpeed;

    if (renderer.get_key(GLFW_KEY_A) == GLFW_PRESS)
        camera.m_position -= VECTOR_RIGHT * moveSpeed;
    if (renderer.get_key(GLFW_KEY_D) == GLFW_PRESS)
        camera.m_position += VECTOR_RIGHT * moveSpeed;

    if (renderer.get_key(GLFW_KEY_E) == GLFW_PRESS)
        camera.m_position += VECTOR_UP * moveSpeed;
    if (renderer.get_key(GLFW_KEY_Q) == GLFW_PRESS)
        camera.m_position -= VECTOR_UP * moveSpeed;
}

int main(int argc, char** argv)
{
    Renderer renderer;
    RenderConfig renderConfig;
    renderConfig.width = 2000;
    renderConfig.height = 1200;
    renderConfig.title = "Vulkan";
    renderConfig.dataMode = RenderConfig::Indexed;
    renderConfig.enableValidationLayers = true;
    renderConfig.clearColor = Vector3(0, 187, 233);
    renderConfig.cameraEnabled = true;
    renderConfig.autoECSUpdate = false;

    renderer.init(renderConfig);

    // Camera

    Camera camera;
    camera.m_position = Vector3(0, 0, 10.f);
    camera.m_orthographic = true;
    renderer.set_active_camera(&camera);

    int fps = 0;
    int avgFps = 0;
    std::string fpsText;
    std::string avgFpsText;
    float deltaTime;
    auto lastTime = std::chrono::high_resolution_clock::now();
    uint32_t frame = 0;

    Vector3 lightPos = { 0,0,0 };

    PhysicsSystem physicsSystem;
    renderer.m_ecs.register_system(&physicsSystem);

    const int testEntityCount = 1;

    Profiler profiler;

    profiler.start_measure("complete model loading");

    DynamicModel quadModel;
    quadModel.load_mesh("/test-models/circle/quad.obj");

    EntityId quad = renderer.m_ecs.create_entity();
    auto& triTransform = renderer.m_ecs.add_component<Transform>(quad);
    triTransform.position = { 0,0,0 };
    auto& model = renderer.m_ecs.add_component<DynamicModel>(quad);
    model = quadModel;

    {
        EntityId e = renderer.m_ecs.create_entity();
        renderer.m_ecs.add_component<Transform>(e).position = { 0.5, 0, 0 };
        renderer.m_ecs.add_component<DynamicModel>(e) = quadModel;
    }

    /*std::vector<EntityId> planeTriangles;
    for (int x = 0; x < 50; x++)
    {
        for (int y = 0; y < 50; y++)
        {
            planeTriangles.push_back(renderer.m_ecs.create_entity());

            auto& t = renderer.m_ecs.add_component<Transform>(planeTriangles.back());
            t.position = { (float)x, (float)y, 0 };

            renderer.m_ecs.add_component<DynamicModel>(planeTriangles.back());
            renderer.m_ecs.get_component<DynamicModel>(planeTriangles.back()) = triangle;
        }
    }*/

    float colorA[3] = { 0,0,0 }, colorB[3] = { 0,0,0 };

    bool updateECS = true;
    bool running = true;
    while (running)
    {
        cart_camera_movement(renderer, camera);

        renderer.gui_begin();

        renderer.draw_engine_gui();

        ImGui::Begin("General");

        if (frame >= fps / 2)
        {
            fpsText = std::to_string(fps) + " fps";
            avgFpsText = std::to_string(avgFps) + " fps (avg)";
            frame = 0;

        }
        ImGui::Text(fpsText.c_str());
        ImGui::Text(avgFpsText.c_str());
        frame++;

        ImGui::SliderFloat("speed", &moveSpeed, 0.f, 0.03f);
        ImGui::SliderFloat("sensitivity", &turningSpeed, 0.f, 0.5f);

        ImGui::SliderFloat3("light pos", (float*) & lightPos, -10, 10);
        renderer.set_light_pos(lightPos);

        ImGui::SliderFloat3("cam pos", (float*) & camera.m_position, -10, 10);

        bool singleUpdateECS = ImGui::Button("step ecs");
        if (ImGui::Button("update ecs"))
            updateECS = !updateECS;
        if (updateECS || singleUpdateECS)
            renderer.m_ecs.update_systems(1.f / 50.f);

        if (updateECS)
            ImGui::Text("ECS activated");
        else
            ImGui::Text("ECS deactivated");

        if (ImGui::Button("Ortho / Persp"))
            camera.m_orthographic = !camera.m_orthographic;

        ImGui::End();

        NVE_RESULT res = renderer.render();
        if (res == NVE_RENDER_EXIT_SUCCESS)
            running = false;

        // time
        auto now = std::chrono::high_resolution_clock::now();
        deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTime).count();
        fps = 1000.f / deltaTime;
        avgFps += fps;
        avgFps /= 2.f;
        lastTime = std::chrono::high_resolution_clock::now();
    }

    return 0;
}