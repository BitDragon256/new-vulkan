#pragma once
#ifndef SHADER_COMPILER_H
#define SHADER_COMPILER_H

#include <string>
#include <vector>

enum class ShaderStage;

// load the shader source code with resolved includes
std::string load_shader_source(std::string path);
// compile glsl shader code to spirv
std::vector<uint32_t> glsl_to_spirv(std::string& glslCode, ShaderStage stage);

#endif //SHADER_COMPILER_H
