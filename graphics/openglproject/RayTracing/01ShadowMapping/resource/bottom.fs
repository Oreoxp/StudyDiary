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

struct OtherObjects {
    vec3 position;
    float radius;
};

in vec3 FragPos;  
in vec3 Normal;  
  
uniform vec3 viewPos;
uniform Material material;
uniform Light light;

#define NR_POINT_OBJ 2
uniform OtherObjects otherObejcts[NR_POINT_OBJ];

vec3 defuseColor = vec3(1.0, 1.0, 1.0);

vec3 CalcLight(Light light_model, vec3 normal, vec3 fragPos, vec3 view_Pos);
vec3 CalcShadow(OtherObjects oo,Light light_model, vec3 normal, vec3 fragPos, vec3 view_Pos, vec3 old_color);
bool isRayIntersectingSphere(vec3 ray_origin, vec3 ray_direction, vec3 sphere_center, float sphere_radius);

void main()
{
    vec3 result = vec3(0.0);
    result += CalcLight(light, Normal, FragPos, viewPos);
    vec3 result2 = CalcShadow(otherObejcts[0], light, Normal, FragPos, viewPos, result);
    vec3 result3 = CalcShadow(otherObejcts[1], light, Normal, FragPos, viewPos, result);
    if(result2 == vec3(0, 0, 0) || result3 == vec3(0, 0, 0)){
        result = vec3(0, 0, 0);
    }
    FragColor = vec4(result, 1.0);
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

vec3 CalcShadow(OtherObjects oo, Light light_model, vec3 normal, vec3 fragPos, vec3 view_Pos, vec3 old_color) {
    vec3 result;
    vec3 view_dir = view_Pos - fragPos; // Fixed the direction
    vec3 light_dir = light_model.position - fragPos;

    if(isRayIntersectingSphere(fragPos, normalize(light_dir), oo.position, 0.2)) { //shadow
        result = vec3(0, 0, 0);
    } else {
        result = old_color;
    }
    return result;
}


// ray_origin: 射线的起点
// ray_direction: 射线的方向（需要归一化）
// sphere_center: 球体的中心
// sphere_radius: 球体的半径
bool isRayIntersectingSphere(vec3 ray_origin, vec3 ray_direction, vec3 sphere_center, float sphere_radius) {
    // 计算从射线起点到球体中心的向量
    vec3 oc = ray_origin - sphere_center;

    // 使用二次方程求解交点
    float a = dot(ray_direction, ray_direction);
    float b = 2.0 * dot(oc, ray_direction);
    float c = dot(oc, oc) - sphere_radius * sphere_radius;
    
    float discriminant = b * b - 4 * a * c;
    
    // 判断是否存在交点
    return discriminant > 0.0;
}
