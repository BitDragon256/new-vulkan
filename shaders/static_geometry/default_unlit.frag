#version 450

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

layout(location = 0) in Material inMat;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(inMat.m_diffuse, 1.0);
}