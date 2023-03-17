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
    vec3 ks = vec3(0.7937, 0.7937, 0.7937);

    vec3 Ls = ks *  (lightPos - Normal)/dot(lightPos - Normal,lightPos - Normal) * pow(max(0.0f, dot(Normal, normalize(lightPos + viewPos))),128);
    
    // combine results
    vec3 result = (La + Ld + Ls) * objectColor;
    FragColor = vec4(result, 1.0);
}