#version 460

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 outPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outUV;

layout( push_constant ) uniform constants
{
    mat4 projView;
    vec3 camPos;
    vec3 lightPos;
} CPC;

void main()
{;
    gl_Position = CPC.projView * vec4(inPosition, 1.0);

    outPos = inPosition;
    outNormal = inNormal.xzy;
    outUV = uv;
}