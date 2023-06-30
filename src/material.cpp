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
	m_file = file;
	m_module = create_shader_module(file, s_device);
}
void Shader::destroy()
{
	vkDestroyShaderModule(s_device, m_module, nullptr);
}

bool Shader::operator==(const Shader& other)
{
	return m_file == other.m_file;
}

VkDevice Shader::s_device;

bool GraphicsShader::operator==(const GraphicsShader& other)
{
	return fragment == other.fragment && vertex == other.vertex;
}

// --------------------------------------
// MATERIAL
// --------------------------------------

bool Material::operator==(const Material& other)
{
	return 
		m_pDiffuseTexture == other.m_pDiffuseTexture &&
		m_pNormalTexture == other.m_pNormalTexture &&
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
void Material::set_default_shader()
{
	m_shader.fragment.load_shader("static_geometry/default_unlit.frag.spv");
	m_shader.vertex.load_shader("static_geometry/default_unlit.vert.spv");
}

Material::Material() :
	m_dissolve{1}
{
	set_default_shader();
}