#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 lightPos; 
uniform vec3 viewPos; 
uniform vec3 lightColor;
uniform vec3 objectColor;

void main()
{
    // ambient
    float kd = 0.001;
    vec3 La = kd * lightColor;
    // diffuse
    vec3 Ld = lightColor * max(dot(Normal, normalize(lightPos - FragPos)), 0.0);
    // specular
    vec3 Ls = vec3(0.01); // specular color (white)
    
    // combine results
    vec3 result = (La + Ld + Ls) * objectColor;
    FragColor = vec4(result, 1.0);
}