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
vec3 quatMul(const vec4 q, const vec3 p) {
    // p’ = (v*v.dot(p) + v.cross(p)*(w))*2 + p*(w*w – v.dot(v))
    // vec3 v = q.xyz;
    // float w = q.w;
    // vec3 p_new = (v * dot(v, p) + cross(v, p) * w) * 2 + p * (w * w - dot(v, v));
    // return p_new;

    vec3 t = 2 * cross(q.xyz, p);
    vec3 p_new = p + q.w * 2 * cross(q.xyz, p) + cross(q.xyz, t);
    return p_new;

    //vec4 r = quatDot(q, quatDot(vec4(v, 0), quatInv(q)));
    //return r.xyz;
}

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
    normalize(q);
    vec4 t = vec4(v, 0);
    return mul(mul(q, t), conj(q)).xyz;
}

void main() {
    Transform transform = ObjectInfoBuffer.objects[gl_BaseInstance];
    mat4 cameraMatrix = CameraPushConstant.proj * CameraPushConstant.view;
    
    vec3 vertexPos = rotate(inPosition, transform.rotation) + transform.position;

    gl_Position = cameraMatrix * vec4(vertexPos, 1.0);
    fragColor = inColor;
}