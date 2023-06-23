#include <stdio.h>

#include <chrono>
#include <sstream>

#include <imgui.h>

#include "render.h"

#include "ecs.h"
#include "ecs.cpp"

struct FooComponentA
{
    float x, y;
};
struct FooComponentB
{
    int counter;
};

class FooSys : public System<FooComponentA, FooComponentB>
{
public:
    void update(float dt, EntityId entity, ECSManager& ecs) override
    {
        auto& fa = ecs.get_component<FooComponentA>(entity);
        auto& fb = ecs.get_component<FooComponentB>(entity);
        fa.x = entity;
        fa.y = dt;

        printf("FooComponentA: %f %f | FooComponentB: %i\n", fa.x, fa.y, fb.counter);

        fb.counter++;
    }
};

int main(int argc, char** argv)
{
    //*

    ECSManager ecs;
    ecs.register_system<FooSys>();

    std::vector<EntityId> entities(100);
    for (size_t i = 0; i < entities.size(); i++)
    {
        entities[i] = ecs.create_entity();
        ecs.add_component<FooComponentA>(entities[i]);
        ecs.add_component<FooComponentB>(entities[i]);
    }

    while (entities.size() > 0)
    {
        ecs.update_systems(0.1f);

        int random = rand() % entities.size();
        ecs.delete_entity(entities[random]);
        entities.erase(entities.begin() + random);
    }

    return 0;

    //*/

    Renderer renderer;
    RenderConfig renderConfig;
    renderConfig.width = 1920;
    renderConfig.height = 1080;
    renderConfig.title = "Vulkan";
    renderConfig.dataMode = RenderConfig::Indexed;
    renderConfig.enableValidationLayers = true;
    renderConfig.clearColor = Vector3(0, 187, 233);
    renderConfig.useModelHandler = true;
    renderConfig.cameraEnabled = true;

    renderer.init(renderConfig);
    
    // Models

    ModelHandler modelHandler;
    renderer.bind_model_handler(&modelHandler);

    Model triangle = Model::create_model(Mesh::create_triangle());
    triangle.m_info.position = Vector3(0, 0, 0);

    Model cube = Model::create_model(Mesh::create_cube());
    cube.m_info.scale = { 1, 1, 1 };

    modelHandler.add_model(&cube);

    // Camera

    Camera camera;
    camera.m_position = Vector3(-1, 0, -1);
    camera.m_rotation = Vector3(0, 0, 0);
    renderer.set_active_camera(&camera);

    float turningSpeed = 0.2f;
    float moveSpeed = 0.01f;

    float cubePosition[3] { 0, 0, 0 };
    float cubeScale[3] { 1, 1, 1 };
    float cubeEulerRotation[3] { 0, 0, 0 };

    std::vector<Vertex> verts(8);
    cube.write_vertices_to(verts.begin());

    int fps = 0;
    int avgFps = 0;
    char fpsText[10];
    float deltaTime;
    auto lastTime = std::chrono::high_resolution_clock::now();

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

        renderer.gui_begin();

        ImGui::Begin("FPS");

        itoa(fps, fpsText, 10);
        ImGui::Text(fpsText);
        itoa(avgFps, fpsText, 10);
        ImGui::Text(fpsText);

        ImGui::End();

        ImGui::Begin("Cube Transform");

        ImGui::SliderFloat3("position", cubePosition, -1, 1);
        ImGui::SliderFloat3("scale", cubeScale, 0, 10);
        ImGui::SliderFloat3("euler rotation", cubeEulerRotation, -PI, PI);

        cube.m_info.position.x = cubePosition[0];
        cube.m_info.position.y = cubePosition[1];
        cube.m_info.position.z = cubePosition[2];
        
        cube.m_info.scale.x = cubeScale[0];
        cube.m_info.scale.y = cubeScale[1];
        cube.m_info.scale.z = cubeScale[2];

        Vector3 EulerAngle(cubeEulerRotation[0], cubeEulerRotation[1], cubeEulerRotation[2]);

        Quaternion finalOrientation(EulerAngle);

        cube.m_info.rotation = finalOrientation;

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