#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;

uniform vec3 view_pos;
uniform sampler2D back_FragColor;

void main()
{
    vec3 view_dir = FragPos - view_pos;
    vec3 refract_dir = refract(view_dir, Normal, 1.0/1.5);

    vec4 ss = texture(back_FragColor, vec2(0,0));
    FragColor = ss;
}