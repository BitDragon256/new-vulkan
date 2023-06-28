#include "material.h"

#include <numeric>

// --------------------------------------
// TEXTURE POOL
// --------------------------------------

uint32_t TexturePool::add_texture(Texture* tex)
{
	m_textures.emplace_back(tex);
	push_image_info(tex->view);
	should_reiterate_active();

	return m_textures.size() - 1;
}

VkWriteDescriptorSet TexturePool::get_descriptor_set_write(VkDescriptorSet descriptorSet, uint32_t binding)
{
	VkWriteDescriptorSet write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.pNext = nullptr;
	write.dstSet = descriptorSet;
	write.dstBinding = binding;
	write.dstArrayElement = 0;

	if (m_reiterateActive)
		reiterate_active();

	write.descriptorCount = -1;
	write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	write.pImageInfo = nullptr;

	return write;
}
void TexturePool::should_reiterate_active()
{
	m_reiterateActive = true;
}
void TexturePool::reiterate_active()
{
	m_activeImageInfos.clear();
	for (size_t texIndex = 0; texIndex < m_textures.size(); texIndex++)
		if (m_textures[texIndex]->active)
			m_activeImageInfos.push_back(m_imageInfos[texIndex]);
}
void TexturePool::push_image_info(VkImageView view)
{
	VkDescriptorImageInfo imageInfo = {};
	imageInfo.sampler = nullptr;
	imageInfo.imageView = view;
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	m_imageInfos.push_back(imageInfo);
}

// --------------------------------------
// SHADER
// --------------------------------------

void Shader::load_shader(std::string file)
{
	m_module = create_shader_module(file, s_device);
}

bool Shader::operator==(const Shader& other)
{
	return m_module == other.m_module;
}

VkDevice Shader::s_device;

// --------------------------------------
// MATERIAL
// --------------------------------------

bool Material::operator==(const Material& other)
{
	return m_fragmentShader == other.m_fragmentShader && m_vertexShader == other.m_vertexShader;
}

Material Material::default_unlit()
{
	Material material;

	

	return material;
}