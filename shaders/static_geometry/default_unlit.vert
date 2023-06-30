#version 460

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 fragColor;

layout( push_constant ) uniform constants
{
    mat4 view;
    mat4 proj;
} CameraPushConstant;

// vec2 positions[3] = vec2[](
//     vec2(0.0, -0.5),
//     vec2(0.5, 0.5),
//     vec2(-0.5, 0.5)
// );

void main()
{
    mat4 cameraMatrix = CameraPushConstant.proj * CameraPushConstant.view;
    gl_Position = cameraMatrix * vec4(inPosition, 1.0);
    //gl_Position = vec4(positions[gl_VertexIndex], 0, 0);
    fragColor = inColor;
}