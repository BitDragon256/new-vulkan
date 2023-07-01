#version 450

struct Material
{
    vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	vec3 transmittance;
	vec3 emission;
	float specularHighlight;
	float refraction;
	float dissolve;
};

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in Material inMat;

layout(location = 0) out vec4 outColor;

layout( push_constant ) uniform constants
{
    mat4 projView; // not updated // vertex stage
    vec3 camPos;
} CameraPushConstant;

vec3 lightPos = vec3(3, 2, 5);

void main()
{
    vec3 N = normalize(inNormal);
    vec3 L = normalize(lightPos - inPos);
    // Lambert's cosine law
    float lambertian = max(dot(N, L), 0.0);
    float specular = 0.0;
    if(lambertian > 0.0)
    {
        vec3 R = reflect(-L, N); // Reflected light vector
        vec3 V = normalize(CameraPushConstant.camPos - inPos); // Vector to viewer
        // Compute the specular term
        float specAngle = max(dot(R, V), 0.0);
        specular = pow(specAngle, inMat.specularHighlight);
    }
    outColor = vec4(inMat.ambient + inMat.diffuse * lambertian + inMat.specular * specular, 1.0);
}