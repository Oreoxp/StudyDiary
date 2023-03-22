#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

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
    gl_Position = vec4(aPos, 1.0f);
}