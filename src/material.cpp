#include "material.h"

#include <numeric>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "buffer.h"

// --------------------------------------
// TEXTURE POOL
// --------------------------------------

uint32_t TexturePool::add_texture(std::string tex)
{
	push_texture(tex);
	push_image_info(m_textures.back().image.m_imageView);
	should_reiterate_active();

	return m_textures.size() - 1;
}
uint32_t TexturePool::find(std::string tex)
{
	auto it = std::find(m_loadedTextures.begin(), m_loadedTextures.end(), tex);
	if (it == m_loadedTextures.end())
		return add_texture(tex);
	return static_cast<uint32_t>(it - m_loadedTextures.begin());
}
void TexturePool::push_texture(std::string texFile)
{
	m_loadedTextures.push_back(texFile);
	m_textures.push_back(Texture());
	auto& tex = m_textures.back();
	tex.active = true;

	int x, y, n;
	unsigned char* data = stbi_load(texFile.c_str(), &x, &y, &n, 4);
	VkDeviceSize imageSize = x * y * 4;

	tex.image.create(
		m_device,
		m_physicalDevice,
		static_cast<uint32_t>(x),
		static_cast<uint32_t>(y),
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_TILING_LINEAR,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT
	);

	Buffer<unsigned char> stagingBuffer;
	BufferConfig bufferConfig = {};
	bufferConfig.device = m_device;
	bufferConfig.physicalDevice = m_physicalDevice;
	bufferConfig.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferConfig.memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	bufferConfig.useStagedBuffer = false;
	stagingBuffer.initialize(bufferConfig);
	stagingBuffer.cpy_raw(imageSize, data);

	stbi_image_free(data);

	// transfer buffer data to image

	VkCommandBuffer cmdBuf = begin_single_time_cmd_buffer(m_commandPool, m_device);

	VkImageSubresourceRange range;
	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	range.baseMipLevel = 0;
	range.levelCount = 1;
	range.baseArrayLayer = 0;
	range.layerCount = 1;

	VkImageMemoryBarrier imageBarrier_toTransfer = {};
	imageBarrier_toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

	imageBarrier_toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageBarrier_toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageBarrier_toTransfer.image = tex.image.m_image;
	imageBarrier_toTransfer.subresourceRange = range;

	imageBarrier_toTransfer.srcAccessMask = 0;
	imageBarrier_toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

	//barrier the image into the transfer-receive layout
	vkCmdPipelineBarrier(
		cmdBuf,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		0, 0, nullptr, 0, nullptr, 1,
		&imageBarrier_toTransfer
	);

	VkBufferImageCopy copyRegion = {};
	copyRegion.bufferOffset = 0;
	copyRegion.bufferRowLength = 0;
	copyRegion.bufferImageHeight = 0;

	copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegion.imageSubresource.mipLevel = 0;
	copyRegion.imageSubresource.baseArrayLayer = 0;
	copyRegion.imageSubresource.layerCount = 1;
	VkExtent3D extent; extent.width = x; extent.height = y; extent.depth = 1;
	copyRegion.imageExtent = extent;

	//copy the buffer into the image
	vkCmdCopyBufferToImage(
		cmdBuf,
		stagingBuffer.m_buffer,
		tex.image.m_image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &copyRegion
	);

	VkImageMemoryBarrier imageBarrier_toReadable = imageBarrier_toTransfer;

	imageBarrier_toReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageBarrier_toReadable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	imageBarrier_toReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imageBarrier_toReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	//barrier the image into the shader readable layout
	vkCmdPipelineBarrier(
		cmdBuf,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0, 0, nullptr, 0, nullptr, 1,
		&imageBarrier_toReadable
	);

	end_single_time_cmd_buffer(cmdBuf, m_commandPool, m_device, m_queue);

	stagingBuffer.destroy();
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

	write.descriptorCount = TEXTURE_POOL_MAX_TEXTURES;
	write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	write.pImageInfo = m_activeImageInfos.data();
	write.pBufferInfo = nullptr;
	write.pTexelBufferView = nullptr;

	return write;
}
VkWriteDescriptorSet TexturePool::get_sampler_descriptor_set_write(VkDescriptorSet descriptorSet, uint32_t binding)
{
	VkWriteDescriptorSet write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.pNext = nullptr;
	write.dstSet = descriptorSet;
	write.dstBinding = binding;
	write.dstArrayElement = 0;
	write.descriptorCount = 1;
	write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
	write.pImageInfo = &m_samplerInfo;
	write.pBufferInfo = nullptr;
	write.pTexelBufferView = nullptr;

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
		if (m_textures[texIndex].active)
			m_activeImageInfos.push_back(m_imageInfos[texIndex]);

	for (size_t i = m_activeImageInfos.size(); i < TEXTURE_POOL_MAX_TEXTURES; i++)
	{
		VkDescriptorImageInfo imageInfo = {};
		imageInfo.sampler = nullptr;
		imageInfo.imageView = m_emptyImages[i].m_imageView;
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		m_activeImageInfos.push_back(imageInfo);
	}
}
void TexturePool::push_image_info(VkImageView view)
{
	VkDescriptorImageInfo imageInfo = {};
	imageInfo.sampler = nullptr;
	imageInfo.imageView = view;
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	m_imageInfos.push_back(imageInfo);
}
void TexturePool::create_sampler()
{
	VkSamplerCreateInfo samplerCI = {};
	samplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCI.pNext = nullptr;
	samplerCI.flags = 0;
	samplerCI.magFilter = VK_FILTER_LINEAR;
	samplerCI.minFilter = VK_FILTER_LINEAR;
	samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCI.mipLodBias = 0.0f;
	samplerCI.anisotropyEnable = VK_FALSE;
	samplerCI.maxAnisotropy = 1;
	samplerCI.compareEnable = VK_FALSE;
	samplerCI.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerCI.minLod = 0.0f;
	samplerCI.maxLod = 0.0f;
	samplerCI.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerCI.unnormalizedCoordinates = VK_FALSE;

	vkCreateSampler(m_device, &samplerCI, nullptr, &m_sampler);
}
void TexturePool::init(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue queue)
{
	m_device = device;
	m_physicalDevice = physicalDevice;
	m_commandPool = commandPool;
	m_queue = queue;
	create_sampler();

	m_samplerInfo = {};
	m_samplerInfo.sampler = m_sampler;

	create_empty_images();
}

void TexturePool::create_empty_images()
{
	m_emptyImages.resize(TEXTURE_POOL_MAX_TEXTURES);

	for (auto& img : m_emptyImages)
	{
		img.create(
			m_device,
			m_physicalDevice,
			2, 2,
			VK_FORMAT_R8G8B8_SRGB,
			VK_IMAGE_TILING_LINEAR,
			VK_IMAGE_USAGE_SAMPLED_BIT
		);
	}

	VkCommandBuffer cmdBuf = begin_single_time_cmd_buffer(m_commandPool, m_device);

	VkImageSubresourceRange range;
	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	range.baseMipLevel = 0;
	range.levelCount = 1;
	range.baseArrayLayer = 0;
	range.layerCount = 1;

	VkImageMemoryBarrier imageBarrier = {};
	imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

	imageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageBarrier.subresourceRange = range;

	imageBarrier.srcAccessMask = 0;
	imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

	std::vector<VkImageMemoryBarrier> barriers;
	barriers.reserve(TEXTURE_POOL_MAX_TEXTURES);

	for (auto& img : m_emptyImages)
	{
		imageBarrier.image = img.m_image;
		barriers.push_back(imageBarrier);
	}

	vkCmdPipelineBarrier(
		cmdBuf,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		0, 0, nullptr, 0, nullptr,
		TEXTURE_POOL_MAX_TEXTURES,
		barriers.data()
	);

	end_single_time_cmd_buffer(cmdBuf, m_commandPool, m_device, m_queue);
}
void TexturePool::cleanup()
{
	for (auto& tex : m_textures)
		tex.image.destroy();

	for (auto& img : m_emptyImages)
		img.destroy();

	vkDestroySampler(m_device, m_sampler, nullptr);
}

// --------------------------------------
// SHADER
// --------------------------------------

void Shader::load_shader(std::string file)
{
	m_file = file;
	m_module = create_shader_module(file, s_device);
}
void Shader::destroy()
{
	vkDestroyShaderModule(s_device, m_module, nullptr);
}

bool Shader::operator==(const Shader& other) const
{
	return m_file == other.m_file;
}

VkDevice Shader::s_device;

void GraphicsShader::set_default_shader()
{
	fragment.load_shader("static_geometry/default_unlit.frag.spv");
	vertex.load_shader("static_geometry/default_unlit.vert.spv");
}
bool GraphicsShader::operator==(const GraphicsShader& other) const
{
	return fragment == other.fragment && vertex == other.vertex;
}

// --------------------------------------
// MATERIAL
// --------------------------------------

bool Material::operator==(const Material& other)
{
	return 
		m_ambientTex == other.m_ambientTex &&
		m_diffuseTex == other.m_diffuseTex &&
		m_specularTex == other.m_specularTex &&
		m_bumpTex == other.m_bumpTex &&
		m_shader == other.m_shader &&
		m_ambient == other.m_ambient &&
		m_diffuse == other.m_diffuse &&
		m_specular == other.m_specular &&
		m_transmittance == other.m_transmittance &&
		m_emission == other.m_emission &&
		m_specularHighlight == other.m_specularHighlight &&
		m_refraction == other.m_refraction &&
		m_dissolve == other.m_dissolve;
}
Vector3 to_vec(const float col[3])
{
	return Vector3{ col[0], col[1], col[2] };
}
Material& Material::operator= (const tinyobj::material_t& mat)
{
	m_ambient = to_vec(mat.ambient);
	m_diffuse = to_vec(mat.diffuse);
	m_specular = to_vec(mat.specular);
	m_transmittance = to_vec(mat.transmittance);
	m_emission = to_vec(mat.emission);
	m_specularHighlight = mat.shininess;
	m_refraction = mat.ior;
	m_dissolve = mat.dissolve;

	if (!mat.ambient_texname.empty())	m_ambientTex  =	m_texBaseDir + mat.ambient_texname;
	if (!mat.diffuse_texname.empty())	m_diffuseTex  =	m_texBaseDir + mat.diffuse_texname;
	if (!mat.specular_texname.empty())	m_specularTex = m_texBaseDir + mat.specular_texname;
	if (!mat.bump_texname.empty())		m_bumpTex	  = m_texBaseDir + mat.bump_texname;

	return *this;
}
MaterialSSBO& MaterialSSBO::operator= (const Material& mat)
{
	m_ambient = mat.m_ambient;
	m_diffuse = mat.m_diffuse;
	m_specular = mat.m_specular;
	m_transmittance = mat.m_transmittance;
	m_emission = mat.m_emission;
	m_specularHighlight = mat.m_specularHighlight;
	m_refraction = mat.m_refraction;
	m_dissolve = mat.m_dissolve;

	return *this;
}

Material Material::default_unlit()
{
	Material material;

	

	return material;
}

Material::Material() :
	m_dissolve{ 1 }, m_shader{ nullptr }
{
	
}