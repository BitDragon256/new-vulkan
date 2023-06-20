#include <stdio.h>

#include <sstream>

#include <imgui.h>

#include "render.h"

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
    float cubeRotation[4] { 1, 0, 0, 0 };
    float cubeEulerRotation[3] { 0, 0, 0 };

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

        ImGui::Begin("Cube Transform");

        ImGui::SliderFloat3("position", cubePosition, -1, 1);
        //ImGui::SliderFloat4("quat rotation", cubeRotation, -1, 1);
        ImGui::SliderFloat3("euler rotation", cubeEulerRotation, -PI, PI);

        cube.m_info.position.x = cubePosition[0];
        cube.m_info.position.y = cubePosition[1];
        cube.m_info.position.z = cubePosition[2];

        Vector3 EulerAngle(cubeEulerRotation[0], cubeEulerRotation[1], cubeEulerRotation[2]);

        Quaternion finalOrientation(EulerAngle);

        cube.m_info.rotation = finalOrientation;

        /*cube.m_info.rotation.x = cubeRotation[0];
        cube.m_info.rotation.y = cubeRotation[1];
        cube.m_info.rotation.z = cubeRotation[2];
        cube.m_info.rotation.w = cubeRotation[3];*/

        std::stringstream quatDisplay;
        quatDisplay << finalOrientation.x << " " << finalOrientation.y << " " << finalOrientation.z << " " << finalOrientation.w;
        ImGui::Text(quatDisplay.str().c_str());

        std::stringstream cubeVerts;
        std::vector<Vertex> verts(8);
        cube.write_vertíces_to(verts.begin());
        for (Vertex v : verts)
        {
            v.pos = Quaternion::rotate(v.pos, finalOrientation);
            cubeVerts << v.pos.x << " " << v.pos.y << " " << v.pos.z << "\n";
        }
        ImGui::Text(cubeVerts.str().c_str());

        ImGui::End();

        NVE_RESULT res = renderer.render();
        if (res == NVE_RENDER_EXIT_SUCCESS)
            running = false;
    }

    return 0;
}