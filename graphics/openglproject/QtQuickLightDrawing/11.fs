#version 330 core
out vec4 FragColor;

struct Material {
    sampler2D diffuse;
    vec3 specular;    
    float shininess;
}; 

struct Light {
    vec3 position;
    vec3 direction;
    float cutOff;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

in vec3 FragPos;  
in vec3 Normal;  
in vec2 TexCoords;
  
uniform vec3 viewPos;
uniform Material material;
uniform Light light;

void main()
{
    vec3 lightDir = normalize(light.position - FragPos);
    float theta = degrees(acos(dot(lightDir, normalize(-light.direction))));

    // ambient
    vec3 ambient = light.ambient * texture(material.diffuse, TexCoords).rgb;
  	
    // diffuse 
    vec3 norm = normalize(Normal);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * texture(material.diffuse, TexCoords).rgb;  

    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * texture(material.diffuse, TexCoords).rgb; // 修改此处，使用漫反射纹理 

    if (theta > light.cutOff) {    
        FragColor = vec4(light.ambient * texture(material.diffuse, TexCoords).rgb, 1.0);
    } else {
        vec3 result = ambient + diffuse + specular;
        FragColor = vec4(result, 1.0);
    }
} 