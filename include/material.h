#pragma once

#include <string>

#include <vulkan/vulkan.h>
#include <tiny_obj_loader.h>

#include "nve_types.h"
#include "image.h"

#define TEXTURE_POOL_MAX_TEXTURES 1024

typedef struct Texture
{
	VImage image;
	bool active;
} Texture;

class TexturePool
{
public:
	uint32_t add_texture(std::string tex);
	uint32_t find(std::string tex);
	VkWriteDescriptorSet get_descriptor_set_write(VkDescriptorSet descriptorSet, uint32_t binding);
	VkWriteDescriptorSet get_sampler_descriptor_set_write(VkDescriptorSet descriptorSet, uint32_t binding);
	void should_reiterate_active();
	void init(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue queue);
	void cleanup();

private:
	std::vector<Texture> m_textures;
	std::vector<std::string> m_loadedTextures;
	std::vector<VkDescriptorImageInfo> m_imageInfos;
	void push_image_info(VkImageView view);
	std::vector<VkDescriptorImageInfo> m_activeImageInfos;

	std::vector<VImage> m_emptyImages;
	void create_empty_images();

	bool m_reiterateActive;
	void reiterate_active();

	void create_sampler();
	void push_texture(std::string texFile);

	VkSampler m_sampler;
	VkDescriptorImageInfo m_samplerInfo;

	VkDevice m_device;
	VkPhysicalDevice m_physicalDevice;
	VkCommandPool m_commandPool;
	VkQueue m_queue;
};

class Shader
{
public:
	void load_shader(std::string file);
	void destroy();
	VkShaderModule m_module;

	bool operator==(const Shader& other) const;

	static VkDevice s_device;
private:
	std::string m_file;
};

struct GraphicsShader
{
	Shader fragment;
	Shader vertex;

	void set_default_shader();
	bool operator==(const GraphicsShader& other) const;
};

class Material
{
public:
	std::string m_ambientTex;
	std::string m_diffuseTex;
	std::string m_specularTex;
	std::string m_bumpTex;

	std::string m_texBaseDir;

	GraphicsShader* m_shader;

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

	bool operator== (const Material& mat);
	Material& operator= (const tinyobj::material_t& other);

	static Material default_unlit();
	Material();
};

struct MaterialSSBO
{
	alignas(16) Color m_ambient;
	alignas(16) Color m_diffuse;
	alignas(16) Color m_specular;
	alignas(16) Color m_transmittance;
	alignas(16) Color m_emission;
	float m_specularHighlight;
	float m_refraction;
	float m_dissolve;
	uint32_t m_textureIndex;

	MaterialSSBO& operator= (const Material& mat);
};