#version 330 core
out vec2 TexCoord;

struct Material {
    int type_; // 0 = ordinary, 1 = transparent
    vec3 pos_;
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

void main()
{
    gl_Position = material.projection_ * material.view_ * material.model_ * vec4(material.pos_, 1.0f);
}