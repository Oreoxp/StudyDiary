#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec2 TexCoord;

struct Material {
    int type_; // 0 = ordinary, 1 = transparent
    vec3 texture_;
    vec3 normal_;
    mat4 model_;
    mat4 view_;
    mat4 projection_;
};


uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform Material material;
flat out Material outMaterial;

void main()
{
    gl_Position =material.projection_ * material.view_ * material.model_ * vec4(aPos, 1.0f);
    outMaterial.type_ = material.type_;
    outMaterial.texture_ = material.texture_;
    outMaterial.normal_ = aNormal;
    outMaterial.model_ = material.model_;
    outMaterial.view_ = material.view_;
    outMaterial.projection_ = material.projection_;
}