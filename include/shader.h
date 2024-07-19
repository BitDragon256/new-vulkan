#pragma once
#ifndef SHADER_H
#define SHADER_H

#include <string>
#include <vector>

enum class RenderType;

typedef struct ShaderType
{
    enum class Lighting
    {
        Custom,
        Unlit,
        Lambertian
    } lighting;
    enum class Material
    {
        WithMaterial,
        WithoutMaterial
    } material;
    enum class Geometry
    {
        Static, // without transform
        Dynamic, // with transform
        TestTriangle
    } geometry;
} ShaderType;

enum class ShaderStage
{
    Tesselation,
    Geometry,
    Vertex,
    Fragment,

    Compute,

    RayGeneration,
    RayMiss,
    RayClosestHit,
    RayAnyHit,
    RayIntersection
};

// stores the shader for a single stage
class SingleShader
{
public:
    const std::string& source_code();
    const std::vector<uint32_t>& spirv_code();

    ShaderStage stage;
private:
    std::string source;
    std::vector<uint32_t> spirvCode;
};

// stores a complete shader consisting of multiple shader stages
class Shader
{
public:
    ShaderType type;

    // generate shaders based on shader type and the desired render
    void generate_shader_stages(RenderType renderType);
    [[nodiscard]] const SingleShader& get_shader_stage(ShaderStage stage) const;
private:
    void set_shader_stage(ShaderStage stage, const SingleShader& singleShader);
    std::vector<SingleShader> m_shaders;
};

#endif //SHADER_H
