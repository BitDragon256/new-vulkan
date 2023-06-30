#pragma once

#include <string>

#include <vulkan/vulkan.h>
#include <tiny_obj_loader.h>

#include "nve_types.h"

typedef struct Texture
{
	VkImage image;
	VkImageView view;
	bool active;
} Texture;

#define TEXTURE_POOL_MAX_TEXTURES 1024

class TexturePool
{
public:
	uint32_t add_texture(Texture* tex);
	VkWriteDescriptorSet get_descriptor_set_write(VkDescriptorSet descriptorSet, uint32_t binding);
	void should_reiterate_active();

private:
	std::vector<Texture*> m_textures;
	std::vector<VkDescriptorImageInfo> m_imageInfos;
	void push_image_info(VkImageView view);
	std::vector<VkDescriptorImageInfo> m_activeImageInfos;

	bool m_reiterateActive;
	void reiterate_active();
};

class Shader
{
public:
	void load_shader(std::string file);
	void destroy();
	VkShaderModule m_module;

	bool operator==(const Shader& other);

	static VkDevice s_device;
private:
	std::string m_file;
};

struct GraphicsShader
{
	Shader fragment;
	Shader vertex;

	bool operator==(const GraphicsShader& other);
};

class Material
{
public:
	Texture* m_pDiffuseTexture;
	Texture* m_pNormalTexture;

	GraphicsShader m_shader;

	Color m_ambient;
	Color m_diffuse;
	Color m_specular;
	Color m_transmittance;
	Color m_emission;
	float m_specularHighlight;
	float m_refraction;
	float m_dissolve;

	void load_diff_tex(std::string file);
	void load_normal_tex(std::string file);

	bool operator== (const Material& other);
	Material& operator= (const tinyobj::material_t& other);

	static Material default_unlit();
	void set_default_shader();
	Material();
};

struct MaterialSSBO
{

};