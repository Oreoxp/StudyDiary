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
    vec3 color;
};

in vec3 FragPos;  
in vec3 Normal;  
in vec2 TexCoords;
  
uniform vec3 viewPos;
uniform Material material;
uniform Light light;
uniform sampler2D depth_map;

vec3 defuseColor = vec3(1.0, 1.0, 1.0);

vec3 CalcLight(Light light_model, vec3 normal, vec3 fragPos, vec3 view_Pos);

void main()
{
    vec3 result = vec3(0.0);
    //result += CalcLight(light, Normal, FragPos, viewPos);

    float depth = texture(depth_map, TexCoord).r;
    FragColor = vec4(vec3(depth), 1.0);
    //FragColor = vec4(result, 1.0);
} 

vec3 CalcLight(Light light_model, vec3 normal, vec3 fragPos, vec3 view_Pos)
{
    vec3 lightDir = normalize(light_model.position - fragPos);
    float intensity = 0.0;
    if(light_model.cutOff == 0){
        intensity = 1.0;
    }else{
        float theta = degrees(acos(dot(lightDir, normalize(-light_model.direction))));
        float theta_cutOff = theta - light_model.cutOff;
        float epsilon   = -5.0;
        intensity = clamp(theta_cutOff / epsilon, 0.0, 1.0);
    }
    // ambient
    vec3 ambient = light_model.ambient * defuseColor;
  	
    // diffuse 
    vec3 norm = normalize(normal);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light_model.diffuse * diff * defuseColor;  

    // specular
    vec3 viewDir = normalize(view_Pos - fragPos);
    //vec3 reflectDir = reflect(-lightDir, norm);  
   // float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    //vec3 specular = light_model.specular * spec * (material.specular);

    //Blinn-Phong model specular
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = max(dot(normal, halfwayDir), 0.0);
    vec3 specular = light_model.specular * spec * (material.specular);


    vec3 result = ambient*intensity + diffuse*intensity + specular;
    //vec3 result = specular;
    return result;
}
