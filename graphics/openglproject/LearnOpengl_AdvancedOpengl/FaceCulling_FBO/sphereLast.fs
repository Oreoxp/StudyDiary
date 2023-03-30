#version 330 core

in vec3 WorldPos;
in vec3 Normal;

uniform samplerCube environmentTexture;
uniform vec3 cameraPos;

out vec4 FragColor;

float fresnel(vec3 I, vec3 N, float ior)
{
    float cosi = max(-1.0, min(1.0, dot(I, N)));
    float etai = 1.0, etat = ior;
    if (cosi > 0) { 
        float temp = 0;
        temp = etai; etai = etat; etat = temp;
    }
    float sint = etai / etat * sqrt(max(0.0, 1.0 - cosi * cosi));
    if (sint >= 1) { return 1.0; }
    float cost = sqrt(max(0.0, 1.0 - sint * sint));
    cosi = abs(cosi);
    float Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost));
    float Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));
    return (Rs * Rs + Rp * Rp) / 2.0;
}

void main()
{
    vec3 I = normalize(WorldPos - cameraPos);
    vec3 N = normalize(Normal);
    float ior = 1.52;
    vec3 R = reflect(I, N);
    vec3 T = refract(I, N, 1.0 / ior);
    float F = fresnel(I, N, ior);

    vec3 reflectedColor = texture(environmentTexture, R).rgb;
    vec3 refractedColor = texture(environmentTexture, T).rgb;

    vec3 finalColor = mix(refractedColor, reflectedColor, F);

    // ... combine the final color with other shading components as needed ...

    FragColor = vec4(finalColor, 1.0);
}