#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D ourTexture1;
uniform sampler2D ourTexture2;
uniform sampler2D ourTexture3;


void main()
{
    //计算光线折射
    vec3 refractColor = texture(ourTexture1, TexCoord).rgb;
    //计算光线反射
    vec3 reflectColor = texture(ourTexture2, TexCoord).rgb;
    //计算反射率
    float reflectRatio = texture(ourTexture3, TexCoord).r;
    //计算折射率
    float refractRatio = 1.0 - reflectRatio;
    //计算最终颜色
    FragColor = vec4(refractColor * refractRatio + reflectColor * reflectRatio, 1.0);
}