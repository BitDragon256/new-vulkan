#include "shader.h"

void test()
{
    Shader shader;
    shader.type.lighting = ShaderType::Lighting::Lambertian;
    shader.type.material = ShaderType::Material::WithMaterial;
}