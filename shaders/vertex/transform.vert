#version 460

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec3 fragColor;

struct Transform
{
    vec3 position;
    vec3 scale;
    vec4 rotation;
};

layout(std140,set = 0, binding = 0) readonly buffer ObjectBuffer
{
    Transform objects[];
} ObjectInfoBuffer;

layout( push_constant ) uniform constants
{
    mat4 view;
    mat4 proj;
} CameraPushConstant;

#define PI 3.14159265

vec4 mul(vec4 q1, vec4 q2)
{
    vec4 ret;
    ret.x = (q1.w*q2.x + q1.x*q2.w + q1.y*q2.z - q1.z*q2.y);
    ret.y = (q1.w*q2.y - q1.x*q2.z + q1.y*q2.w + q1.z*q2.x);
    ret.z = (q1.w*q2.z + q1.x*q2.y - q1.y*q2.x + q1.z*q2.w);
    ret.w = (q1.w*q2.w - q1.x*q2.x - q1.y*q2.y - q1.z*q2.z);
    return ret;
}

vec4 conj(vec4 q)
{
    return vec4(-q.xyz, q.w);
}

vec3 rotate(vec3 v, vec4 q)
{
    vec4 t = vec4(v, 0);
    return
    mul(
        mul(q, t),
        conj(q)
    ).xyz;
}

vec3 fast_rot(vec3 v, vec4 q)
{
    vec3 t = 2 * cross(q.xyz, v);
    return v + q.w * t + cross(q.xyz, t);
}

vec3 even_faster_rot(vec3 v, vec4 q)
{
    vec3 p = q.xyz;
    float w = q.w;
    return (p * dot(p, v) + cross(p, v) * w) * 2 + v * (w * w - dot(p, p));
}

void main() {
    Transform transform = ObjectInfoBuffer.objects[gl_BaseInstance];
    mat4 cameraMatrix = CameraPushConstant.proj * CameraPushConstant.view;
    
    vec3 vertexPos = even_faster_rot(inPosition * transform.scale, transform.rotation) + transform.position;

    gl_Position = cameraMatrix * vec4(vertexPos, 1.0);
    fragColor = inColor;
}