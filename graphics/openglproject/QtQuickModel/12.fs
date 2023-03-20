#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

in vec3 fragNormal;
in vec3 fragPosition;
in vec3 gs_barycentricCoords;
in vec3 gs_position;
in vec3 gs_color;

uniform sampler2D texture_diffuse1;

uniform vec3 edgeColor = vec3(1.0, 0.0, 0.0); // Red color for edges
uniform float edgeThreshold = 0.1; // Threshold for detecting edges

void main()
{    
    //FragColor = texture(texture_diffuse1, TexCoords);


    // Check if the fragment is near an edge using barycentric coordinates
    // minBarycentricCoord = min(min(gs_barycentricCoords.x, gs_barycentricCoords.y), gs_barycentricCoords.z);
    //if (minBarycentricCoord > edgeThreshold) {
    //    FragColor = vec4(0.5,0.5,0.5, 1.0);
   // }else{
    //    FragColor = vec4(gs_position, 1.0);
    //}
    
    //if (gl_FrontFacing) { // Check if the fragment is part of a front-facing triangle
    //    FragColor = vec4(edgeColor, 1.0); // Output the vertex color for front-facing triangles
    //} else {
    //    FragColor = vec4(1.0, 1.0, 1.0, 1.0); // Output default color for back-facing triangles
    //}
     // 计算片段与顶点的距离
    //float distance = length(gs_position - gl_FragCoord.xyz);
    
    // 如果距离小于阈值，则显示红色
    //if (distance < edgeThreshold)
    //{
    //    FragColor = vec4(1.0, 0.0, 0.0, 1.0); // 红色
    //}
    
    FragColor = vec4(gs_color, 1.0);
}