#version 330 core

layout (triangles) in;
layout (line_strip, max_vertices = 30) out;

//细分参数
uniform float u_subdivisionLevel = 1.0;

out vec3 gs_barycentricCoords;
out vec3 gs_position;
out vec3 gs_color;

vec3 find_sphere_center(vec3 p0, vec3 p1, vec3 p2) {
    vec3 a = p1 - p0;
    vec3 b = p2 - p0;
    vec3 c = cross(a, b);
    vec3 d = cross(c, a);
    float e = dot(d, b);
    float f = dot(d, d);
    float g = 0.5 * e / f;
    return p0 + g * d;
}

vec4 move_point_to_arc(vec3 A, vec3 B, vec3 C, vec3 D) {
    vec3 O = find_sphere_center(A, B, C);
    float R = length(O - A);

    float d = length(O - D);
    float s = R / d;

    vec3 OD = D - O;
    vec3 OD_scaled = OD * s;

    vec3 D_new = O + OD_scaled;

    return vec4(D_new,1.0);
}

void main()
{
    for (int i = 0; i < 3; ++i)
    {
        gl_Position = gl_in[i].gl_Position;
        gs_color = vec3(1.0, 0.0, 0.0);
        EmitVertex();
    }
    gl_Position = gl_in[0].gl_Position;
    gs_color = vec3(1.0, 0.0, 0.0);
    EmitVertex();
    EndPrimitive();
    
    //曲面细分
    for(int i = 0; i < 3; ++i) {
        vec4 p0 = gl_in[i].gl_Position;
        vec4 p1 = gl_in[(i + 1) % 3].gl_Position;
        vec4 p2 = gl_in[(i + 2) % 3].gl_Position;
        gl_Position = (p0 + p1) / u_subdivisionLevel;
        gs_color = vec3(0.0, 1.0, 0.0);
        EmitVertex();
    }
    vec4 p0 = gl_in[0].gl_Position;
    vec4 p1 = gl_in[1].gl_Position;
    vec4 p2 = gl_in[2].gl_Position;
    gl_Position =(p0 + p1) / u_subdivisionLevel;
    gs_color = vec3(0.0, 1.0, 0.0);
    EmitVertex();
    EndPrimitive();
}
