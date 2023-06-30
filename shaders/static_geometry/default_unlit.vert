#version 460

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 uv;
layout(location = 4) in uint inMaterial;

struct Material
{
    vec3 m_ambient;
	vec3 m_diffuse;
	vec3 m_specular;
	vec3 m_transmittance;
	vec3 m_emission;
	float m_specularHighlight;
	float m_refraction;
	float m_dissolve;
};

layout(location = 0) out vec3 outPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out Material outMat;

layout(std140,set = 0, binding = 0) readonly buffer MaterialBuffer
{
    Material mats[];
} MaterialBufferObjects;

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
    outMat = MaterialBufferObjects.mats[inMaterial];

    outPos = gl_Position.xyz;
    outNormal = inNormal;
}