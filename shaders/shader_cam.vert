#version 460

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

struct Transform
{
    vec3 position;
    vec3 scale;
    vec4 rotation;
};

// Quaternion Inverse
vec4 quatInv(const vec4 q) {
 // assume it's a unit quaternion, so just Conjugate
 return vec4( -q.xyz, q.w );
}
// Quaternion multiplication
vec4 quatDot(const vec4 q1, const vec4 q2) {
 float scalar = q1.w * q2.w - dot(q1.xyz, q2.xyz);
 vec3 v = cross(q1.xyz, q2.xyz) + q1.w * q2.xyz + q2.w * q1.xyz;
 return vec4(v, scalar);
}
// Apply unit quaternion to vector (rotate vector)
vec3 quatMul(const vec4 q, const vec3 v) {
 vec4 r = quatDot(q, quatDot(vec4(v, 0), quatInv(q)));
 return r.xyz;
}

vec3 mul(const Transform t, const vec3 v)
{
    return t.position.xyz + quatMul(t.rotation, v * t.scale.xyz);
}

layout(std140,set = 0, binding = 0) readonly buffer ObjectBuffer
{
    Transform objects[];
} ObjectInfoBuffer;

layout( push_constant ) uniform constants
{
    mat4 view;
    mat4 proj;
} CameraPushConstant;

void main() {
    Transform transform = ObjectInfoBuffer.objects[gl_BaseInstance];
    mat4 cameraMatrix = CameraPushConstant.proj * CameraPushConstant.view;
    gl_Position = cameraMatrix * vec4(mul(transform, inPosition), 1.0);
    fragColor = inColor;
}