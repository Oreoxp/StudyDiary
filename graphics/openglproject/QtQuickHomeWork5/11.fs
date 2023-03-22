#version 330 core
out vec4 FragColor;

struct Material {
    int type_; // 0 = ordinary, 1 = transparent
    vec3 texture_;
    vec3 normal_;
    mat4 model_;
    mat4 view_;
    mat4 projection_;
};

flat in Material outMaterial;

void main()
{
    if (outMaterial.type_ == 0) {
        FragColor = vec4(outMaterial.normal_, 1.0);
    } else {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
