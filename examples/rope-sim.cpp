#include <stdio.h>

#include <chrono>
#include <sstream>

#include <nve.h>

float turningSpeed = 0.5f;
float moveSpeed = 0.008f;
float camDistance = 5;
Vector3 camPos = { 0,0,0 };
Vector3 camRot = { 0,0,0 };

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

struct RopeSegment
{
    Vector2 pos;
    Vector2 lastPos;
    float mass;
};

GUI_PRINT_COMPONENT_START(RopeSegment)

ImGui::DragFloat2("pos", (float*)&component.pos);
ImGui::DragFloat("mass", &component.mass);

GUI_PRINT_COMPONENT_END

class Rope : public System<RopeSegment, Transform>
{
public:
    void update(float dt)
    {
        m_ecs->get_component<RopeSegment>(m_entities.front()).lastPos = m_ecs->get_component<RopeSegment>(m_entities.front()).pos;
        m_ecs->get_component<RopeSegment>(m_entities.back()).lastPos = m_ecs->get_component<RopeSegment>(m_entities.back()).pos;
        for (int i = 0; i < m_substeps; i++)
        {
            for (auto id : m_entities)
            {
                auto& segment = m_ecs->get_component<RopeSegment>(id);
                if (id == m_entities.front() || id == m_entities.back())
                {
                    segment.pos = segment.lastPos;
                    continue;
                }

                auto& s1 = m_ecs->get_component<RopeSegment>(id - 1);
                auto& s2 = m_ecs->get_component<RopeSegment>(id + 1);
                Vector2 d1 = s1.pos - segment.pos;
                Vector2 d2 = s2.pos - segment.pos;
                segment.pos += glm::normalize(d1) * (glm::length(d1) - m_dist) * m_ropeStrength / 2.f;
                s1.pos -= glm::normalize(d1) * (glm::length(d1) - m_dist) * m_ropeStrength / 2.f;
                segment.pos += glm::normalize(d2) * (glm::length(d2) - m_dist) * m_ropeStrength / 2.f;
                s2.pos -= glm::normalize(d2) * (glm::length(d2) - m_dist) * m_ropeStrength / 2.f;
            }
            for (auto id : m_entities)
            {
                auto& segment = m_ecs->get_component<RopeSegment>(id);
                if (id == m_entities.front() || id == m_entities.back())
                {
                    segment.pos = segment.lastPos;
                }
                else
                {
                    Vector2 force = { 0,0 };
                    if (i == 0)
                        force += Vector2 { 0, -m_gravity * segment.mass };

                    // verlet
                    Vector2 acc = force;// / segment.mass;
                    Vector2 nextPos = 2.f * segment.pos - segment.lastPos + 0.5f * acc * dt * dt;

                    segment.lastPos = segment.pos;
                    segment.pos = nextPos;
                }

                // sync transform
                m_ecs->get_component<Transform>(id).position = Vector3(segment.pos, 0);
            }
        }
    }

    float m_gravity = 9.81;
    int m_substeps = 100;
    float m_ropeStrength = 0.1f;
    float m_dist = .5f;
};

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

    Rope ropeSystem;
    renderer.m_ecs.register_system<Rope>(&ropeSystem);

    int particleCount = 40;
    int width = 10;
    std::vector<EntityId> particles;

    for (int i = 0; i < particleCount; i++)
    {
        EntityId id = renderer.m_ecs.create_entity();
        auto& part = renderer.m_ecs.add_component<RopeSegment>(id);
        renderer.m_ecs.add_component<DynamicModel>(id) = ball;
        auto& transform = renderer.m_ecs.add_component<Transform>(id);
        transform.scale = Vector3(0.05f);

        int x = i % (width)-width / 2;
        part.pos = { x, 0 };
        part.lastPos = part.pos;
        part.mass = 0.f;
        
        particles.push_back(id);
    }

   float renderAvg = 0;

    bool updateECS = true;
    bool singleUpdateECS = false;
    bool running = true;
    while (running)
    {
        cart_camera_movement(renderer, camera);

        renderer.gui_begin();

        profiler.start_measure("total time");

        profiler.start_measure("gui");

        // GUI
        {
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

            ImGui::DragFloat("gravity", &ropeSystem.m_gravity);
            ImGui::DragFloat("rope strength", &ropeSystem.m_ropeStrength, 0.05f);
            ImGui::DragFloat("rope segment length", & ropeSystem.m_dist, 0.2f);

            ImGui::End();

            profiler.end_measure("gui", true);
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
        profiler.end_measure("total time", true);

        std::cout << "---------------------------------------\n";

        // time
        auto now = std::chrono::high_resolution_clock::now();
        deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTime).count();
        fps = 1000.f / deltaTime;
        avgFps += fps;
        avgFps /= 2.f;
        lastTime = std::chrono::high_resolution_clock::now();
    }
    std::cout << "avg render time: " << renderAvg << " seconds\n";

    return 0;
}