#pragma once

#include <string>
#include <vulkan/vulkan.h>

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

class Material
{
public:
	Texture* m_pDiffuseTexture;
	Texture* m_pNormalTexture;
	VkShaderModule* m_pShader;
	Vector4 m_diffuseColor;
	Vector3 m_ambientColor;
	float m_alphaTest;

	void load_shader(std::string file);
	void load_diff_tex(std::string file);
	void load_normal_tex(std::string file);

	bool operator==(const Material& other);
};