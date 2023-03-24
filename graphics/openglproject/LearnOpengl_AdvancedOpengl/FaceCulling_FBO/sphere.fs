#version 330 core

in vec3 WorldPos;
in vec3 Normal;

out vec4 FragColor;

uniform vec3 CameraPos;
uniform vec3 LightPos;
uniform vec3 LightColor;
uniform vec3 ModelColor;

float schlick(float cosine, float refIdx) {
    float r0 = (1.0 - refIdx) / (1.0 + refIdx);
    r0 = r0 * r0;
    return r0 + (1.0 - r0) * pow(1.0 - cosine, 5);
}

void main()
{
    vec3 N = normalize(Normal);
    vec3 L = normalize(LightPos - WorldPos);
    vec3 V = normalize(CameraPos - WorldPos);
    vec3 H = normalize(L + V);
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, H), 0.0);

    float fresnel = schlick(NdotV, 1.33);
    vec3 specular = fresnel * LightColor;
    vec3 diffuse = (1.0 - fresnel) * LightColor * NdotL;

    // Multiply ModelColor with diffuse and specular components
    vec3 finalDiffuse = diffuse * ModelColor;
    vec3 finalSpecular = specular * ModelColor;

    // Replace the original color calculation with the new components
    vec3 color = finalDiffuse + finalSpecular;

    FragColor = vec4(color, 1.0);
}
