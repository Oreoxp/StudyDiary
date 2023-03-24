#version 330 core

in vec3 WorldPos;
in vec3 Normal;

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D envMap;
uniform vec3 CameraPos;
uniform vec3 LightPos;
uniform vec3 LightColor;

void main()
{
    FragColor = texture(envMap, TexCoords);
}