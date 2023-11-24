#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <chrono>
#include <sstream>

#include <nve.h>

#include "simple_fluid.h"

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

    Profiler profiler;

    profiler.start_measure("complete model loading");

    DynamicModel ball;
    ball.load_mesh("/default_models/circle/quad.obj");

    // simple fluid
    SimpleFluid simpleFluid;
    renderer.m_ecs.register_system<SimpleFluid>(&simpleFluid);
    simpleFluid.m_active = true;

    int particleCount = 0;
    std::vector<EntityId> particles;

    auto generate_particles = [particleCount, &particles, &renderer, ball]()
    {
        int width = (int)sqrt(particleCount);
        float downScale = fminf(SF_BOUNDING_WIDTH, SF_BOUNDING_HEIGHT) / width / 1.5f;
        for (int i = particles.size(); i < particleCount; i++)
        {
            EntityId id = renderer.m_ecs.create_entity();
            auto& part = renderer.m_ecs.add_component<Particle>(id);
            renderer.m_ecs.add_component<DynamicModel>(id) = ball;
            auto& transform = renderer.m_ecs.add_component<Transform>(id);
            transform.scale = Vector3(0.05f);

            int x = i % (width)-width / 2;
            int y = (int)i / width - width / 2;
            part.position = { x * downScale, y * downScale};

            particles.push_back(id);
        }
    };
    generate_particles();

    DynamicModel quad;
    quad.load_mesh("/default_models/quad/quad.obj");
    quad.m_children[0].material.m_diffuse = { 1, 1, 1 };

    EntityId top, bottom, left, right;
    top = renderer.m_ecs.create_entity();
    bottom = renderer.m_ecs.create_entity();
    left = renderer.m_ecs.create_entity();
    right = renderer.m_ecs.create_entity();

    renderer.m_ecs.add_component<DynamicModel>(top) = quad;
    renderer.m_ecs.add_component<DynamicModel>(bottom) = quad;
    renderer.m_ecs.add_component<DynamicModel>(left) = quad;
    renderer.m_ecs.add_component<DynamicModel>(right) = quad;

    renderer.m_ecs.add_component<Transform>(top);
    renderer.m_ecs.add_component<Transform>(bottom);
    renderer.m_ecs.add_component<Transform>(left);
    renderer.m_ecs.add_component<Transform>(right);

    Transform & tTop = renderer.m_ecs.get_component<Transform>(top);
    Transform & tBottom = renderer.m_ecs.get_component<Transform>(bottom);
    Transform & tLeft = renderer.m_ecs.get_component<Transform>(left);
    Transform & tRight = renderer.m_ecs.get_component<Transform>(right);

    tTop.scale.x = simpleFluid.m_maxBounds.x - simpleFluid.m_minBounds.x;
    tTop.position.y = simpleFluid.m_maxBounds.y + 0.5f;

    tBottom.scale.x = simpleFluid.m_maxBounds.x - simpleFluid.m_minBounds.x;
    tBottom.position.y = simpleFluid.m_minBounds.y - 0.5f;

    tLeft.scale.y = simpleFluid.m_maxBounds.y - simpleFluid.m_minBounds.y;
    tLeft.position.x = simpleFluid.m_minBounds.x - 0.5f;

    tRight.scale.y = simpleFluid.m_maxBounds.y - simpleFluid.m_minBounds.y;
    tRight.position.x = simpleFluid.m_maxBounds.x + 0.5f;

    float renderAvg = 0.f;
    float profilerTime = 0.f;
    
    bool updateECS = true;
    bool singleUpdateECS = false;
    bool running = true;
    while (running)
    {
        cart_camera_movement(renderer, camera);

        // mouse interaction
        auto mousePos = renderer.mouse_to_screen(renderer.get_mouse_pos()) / 2.f;
        if (renderer.get_mouse_button(GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
            simpleFluid.m_mousePos = {
            mousePos.y * SF_BOUNDING_HEIGHT,
            -mousePos.x * SF_BOUNDING_WIDTH
        };
        else
            simpleFluid.m_mousePos = Vector2(std::numeric_limits<float>::max());

        renderer.gui_begin();
        
        profiler.start_measure("total time");

        //profiler.start_measure("gui");

        // GUI
        {
            renderer.draw_engine_gui();

            ImGui::Begin("General");

            // -----------------------------------------------
            // PARTICLE SPAWNING
            // -----------------------------------------------
            if (ImGui::Button("Create Particles"))
            {
                srand(time(NULL));

                int width = 12;
                int newParticleCount = width * width;
                float downScale = fminf(SF_BOUNDING_WIDTH, SF_BOUNDING_HEIGHT) / width / 1.5f;
                for (int i = 0; i < newParticleCount; i++)
                {
                    EntityId id = renderer.m_ecs.create_entity();
                    auto& part = renderer.m_ecs.add_component<Particle>(id);
                    renderer.m_ecs.add_component<DynamicModel>(id) = ball;
                    auto& transform = renderer.m_ecs.add_component<Transform>(id);
                    transform.scale = Vector3(0.1f);

                    int x = i % (width)-width / 2;
                    int y = (int)i / width - width / 2;
                    part.position = { x * downScale, y * downScale + (rand() % 100) / 1000.f - 0.05f };

                    particles.push_back(id);
                }
            }
            if (ImGui::Button("Reset Velocity"))
            {
                for (auto pId : particles)
                {
                    auto& p = renderer.m_ecs.get_component<Particle>(pId);
                    p.lastPosition = p.position;
                    p.velocity = Vector2(0);
                }
            }

            if (frame >= fps / 2)
            {
                fpsText = std::to_string(fps) + " fps";
                avgFpsText = std::to_string(avgFps) + " fps (avg)";
                frame = 0;

            }
            ImGui::Text(fpsText.c_str());
            ImGui::Text(avgFpsText.c_str());
            frame++;

            ImGui::DragFloat("Target Density", &simpleFluid.m_targetDensity);
            ImGui::DragFloat("Pressure Multiplier", &simpleFluid.m_pressureMultiplier, 0.1f);
            ImGui::DragFloat("Smoothing Radius", &simpleFluid.m_smoothingRadius);
            //ImGui::DragInt("Particle Count", &particleCount);
            ImGui::DragFloat("Gravity", &simpleFluid.m_gravity, 0.5f);
            ImGui::DragFloat("Wall Force", &simpleFluid.m_wallForceMultiplier, 0.2f);
            ImGui::DragFloat("Collision Damping", &simpleFluid.m_collisionDamping);

            ImGui::DragFloat("Influence Inner", &simpleFluid.m_influenceInner);
            ImGui::DragFloat("Influence Power", &simpleFluid.m_influencePower);
            ImGui::DragFloat("Mouse Radius", &simpleFluid.m_mouseRadius);
            ImGui::DragFloat("Mouse Strength", &simpleFluid.m_mouseStrength);

            ImGui::DragInt("Thread Bucket Difference", &simpleFluid.m_threadBucketDiff, 1, 1);

            if (ImGui::Button("Play / Pause"))
                simpleFluid.m_active ^= 1;

            generate_particles();

            ImGui::SliderFloat("speed", &moveSpeed, 0.f, 0.03f);
            ImGui::SliderFloat("sensitivity", &turningSpeed, 0.f, 0.5f);

            ImGui::SliderFloat3("cam pos", (float*)&camera.m_position, -10, 10);

            singleUpdateECS = ImGui::Button("step ecs");
            if (ImGui::Button("update ecs"))
                updateECS = !updateECS;

            if (updateECS)
                ImGui::Text("ECS activated");
            else
                ImGui::Text("ECS deactivated");

            if (ImGui::Button("Ortho / Persp"))
                camera.m_orthographic = !camera.m_orthographic;

            ImGui::End();

            // profiler.end_measure("gui", true);
        }

        profiler.start_measure("ecs");
        if (updateECS || singleUpdateECS)
            renderer.m_ecs.update_systems(1.f / 120.f);
        profiler.end_measure("ecs", true);

        profiler.start_measure("render");
        NVE_RESULT res = renderer.render();
        if (res == NVE_RENDER_EXIT_SUCCESS)
            running = false;
        renderAvg += profiler.end_measure("render", true);
        renderAvg /= 2;
        profiler.end_measure("total time", false);

        // time
        auto now = std::chrono::high_resolution_clock::now();
        deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTime).count();
        fps = 1000.f / deltaTime;
        avgFps += fps;
        avgFps /= 2.f;
        lastTime = std::chrono::high_resolution_clock::now();

        profilerTime += deltaTime;
        if (profilerTime > 1.f)
            Profiler::print_buf();
        profiler.out_buf() << "\nprint\n";
    }
    std::cout << "avg render time: " << renderAvg << " seconds\n";

    return 0;
}