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
    sampler2D vertex_data_texture;
    sampler2D normal_data_texture;
    int num_triangles;
};

in vec3 FragPos;  
in vec3 Normal;  
  
uniform vec3 viewPos;
uniform Material material;
uniform Light light;

#define NR_POINT_OBJ 3
uniform OtherObjects otherObejcts[NR_POINT_OBJ];

vec3 defuseColor = vec3(1.0, 1.0, 1.0);

vec3 CalcLight(Light light_model, vec3 normal, vec3 fragPos, vec3 view_Pos);
vec3 CalcShadow(OtherObjects oo,Light light_model, vec3 normal, vec3 fragPos, vec3 view_Pos, vec3 old_color);
bool isRayIntersectingSphere(vec3 ray_origin, vec3 ray_direction, vec3 sphere_center, float sphere_radius);
vec3 get_tex_data(sampler2D tex, int index);
bool isRayIntersectingTriangle(vec3 ray_origin, vec3 ray_dir, vec3 v0, vec3 v1, vec3 v2);

void main()
{
    vec3 result = vec3(0.0);
    result += CalcLight(light, Normal, FragPos, viewPos);
    vec3 result2 = CalcShadow(otherObejcts[0], light, Normal, FragPos, viewPos, result);
    vec3 result3 = CalcShadow(otherObejcts[1], light, Normal, FragPos, viewPos, result);
    //vec3 result4 = CalcShadow(otherObejcts[2], light, Normal, FragPos, viewPos, result);
    if(result2 == vec3(0, 0, 0) || result3 == vec3(0, 0, 0)){
        result = vec3(0, 0, 0);
    }
    //result = FragPos.xyz;
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
    bool isShadow = false;

    if(isRayIntersectingSphere(fragPos, normalize(light_dir), oo.position, 0.3)) { 
        for (int i = 0; i < oo.num_triangles * 3; i += 3) {
            vec3 v0 = get_tex_data(oo.vertex_data_texture, i);
            vec3 v1 = get_tex_data(oo.vertex_data_texture, i + 1);
            vec3 v2 = get_tex_data(oo.vertex_data_texture, i + 2);

            if (isRayIntersectingTriangle(fragPos, normalize(light_dir), v0, v1, v2)) {
                isShadow = true;
                break;
            }
        }
    }

    if (isShadow) {
        result = vec3(0, 0, 0);
    } else {
        result = old_color;
    }

    return result;
}

//判断光线是否与三角形相交
bool isRayIntersectingTriangle(vec3 ray_origin, vec3 ray_dir, vec3 v0, vec3 v1, vec3 v2) {
    vec3 edge1 = v1 - v0;
    vec3 edge2 = v2 - v0;
    vec3 h = cross(ray_dir, edge2);
    float a = dot(edge1, h);

    if (abs(a) < 1e-5) {
        return false;
    }

    float f = 1.0 / a;
    vec3 s = ray_origin - v0;
    float u = f * dot(s, h);

    if (u < 0.0 || u > 1.0) {
        return false;
    }

    vec3 q = cross(s, edge1);
    float v = f * dot(ray_dir, q);

    if (v < 0.0 || u + v > 1.0) {
        return false;
    }

    float t = f * dot(edge2, q);

    return t > 1e-5;
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

vec3 get_tex_data(sampler2D tex, int index) {
    return texelFetch(tex, ivec2(index, 0), 0).rgb;
}