#include <stdio.h>

#include <chrono>
#include <sstream>

#include <nve.h>

float turningSpeed = 0.4f;
float moveSpeed = 0.005f;
void camera_movement(Renderer& renderer, Camera& camera)
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
        camera.m_position.z += moveSpeed;
    if (renderer.get_key(GLFW_KEY_Q) == GLFW_PRESS)
        camera.m_position.z -= moveSpeed;

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
    renderConfig.autoECSUpdate = false;

    renderer.init(renderConfig);

    // Camera

    Camera camera;
    camera.m_position = Vector3(-2.5, 0, 0);
    camera.m_rotation = Vector3(0, 0, 0);
    renderer.set_active_camera(&camera);

    float cubePosition[3]{ 0, 0, 0 };
    float cubeScale[3]{ 1, 1, 1 };
    float cubeEulerRotation[3]{ 0, 0, 0 };

    int fps = 0;
    int avgFps = 0;
    std::string fpsText;
    std::string avgFpsText;
    float deltaTime;
    auto lastTime = std::chrono::high_resolution_clock::now();
    uint32_t frame = 0;

    float lightPos[3] = { 0, 0, 0 };

    //auto testEntity = renderer.m_ecs.create_entity();
    //renderer.m_ecs.add_component<Transform>(testEntity);
    //renderer.m_ecs.add_component<DynamicModel>(testEntity);
    //auto& testEntityModel = renderer.m_ecs.get_component<DynamicModel>(testEntity);
    //testEntityModel.load_mesh("/test-models/sphere/sphere-cylcoords-16k.obj");
    //auto& testEntityTransform = renderer.m_ecs.get_component<Transform>(testEntity);
    ////testEntityTransform.rotation = Quaternion({ -PI/2,0,0 });

    //float modelPos[3] = { 0,0,0 }, modelRot[3] = { 0,0,0 }, modelScale[3] = { 0.005,0.005,0.005 };

    PhysicsSystem physicsSystem;
    renderer.m_ecs.register_system(&physicsSystem);

    const int testEntityCount = 1;

    Profiler profiler;

    profiler.start_measure("complete model loading");

    DynamicModel prefab;
    prefab.load_mesh("/test-models/sphere/sphere-cylcoords-16k.obj");

    //std::vector<EntityId> testEntities(testEntityCount);
    //for (int i = 0; i < testEntityCount; i++)
    //{
    //    testEntities[i] = renderer.m_ecs.create_entity();
    //    renderer.m_ecs.add_component<Transform>(testEntities[i]);
    //    renderer.m_ecs.add_component<DynamicModel>(testEntities[i]);
    //    //renderer.m_ecs.add_component<CheckMovement>(testEntities[i]);
    //    renderer.m_ecs.add_component<Rigidbody>(testEntities[i]);

    //    renderer.m_ecs.get_component<Rigidbody>(testEntities[i]).radius = .1f;

    //    renderer.m_ecs.get_component<Transform>(testEntities[i]).position = Vector3(0, 1, 0);
    //    renderer.m_ecs.get_component<Transform>(testEntities[i]).scale = Vector3(0.001f);

    //    renderer.m_ecs.get_component<DynamicModel>(testEntities[i]) = prefab;
    //}

    float ballRadius = .115f;

    std::vector<EntityId> entities;
    auto add_object = [&renderer, prefab, ballRadius, &entities] {
        EntityId entity = renderer.m_ecs.create_entity();
        renderer.m_ecs.add_component<Transform>(entity);
        renderer.m_ecs.add_component<DynamicModel>(entity);
        renderer.m_ecs.add_component<Rigidbody>(entity);

        renderer.m_ecs.get_component<Rigidbody>(entity).radius = ballRadius;

        renderer.m_ecs.get_component<Transform>(entity).position = Vector3((float) rand() / INT32_MAX * 2 - 1, 1, 0);
        renderer.m_ecs.get_component<Transform>(entity).scale = Vector3(0.001f);

        renderer.m_ecs.get_component<DynamicModel>(entity) = prefab;

        entities.push_back(entity);
    };

    profiler.end_measure("complete model loading");
    profiler.print_last_measure("complete model loading");

    bool updateECS = true;

    bool running = true;
    while (running)
    {
        camera_movement(renderer, camera);

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

        ImGui::SliderFloat3("light pos", lightPos, -10, 10);
        renderer.set_light_pos(Vector3{ lightPos[0], lightPos[1], lightPos[2] });

        //ImGui::SliderFloat3("model pos", modelPos, -10, 10);
        //ImGui::SliderFloat3("model rot", modelRot, -180, 180);
        //ImGui::SliderFloat3("model scale", modelScale, 0.01, 10);
        //testEntityTransform.position = { modelPos[0], modelPos[1] , modelPos[2] };
        //testEntityTransform.scale = { modelScale[0], modelScale[1] , modelScale[2] };
        //testEntityTransform.rotation.euler(Vector3{ modelRot[0], modelRot[1] , modelRot[2] });

        ImGui::DragFloat("ball radius", &ballRadius);
        for (auto entity : entities)
            renderer.m_ecs.get_component<Rigidbody>(entity).radius = ballRadius;

        if (ImGui::Button("add object"))
            add_object();

        bool singleUpdateECS = ImGui::Button("step ecs");
        if (ImGui::Button("update ecs"))
            updateECS = !updateECS;
        if (updateECS || singleUpdateECS)
            renderer.m_ecs.update_systems(1.f / 50.f);

        if (updateECS)
            ImGui::Text("ECS activated");
        else
            ImGui::Text("ECS deactivated");

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