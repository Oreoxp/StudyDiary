#version 330 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

out vec3 gs_barycentricCoords;
out vec3 gs_position;

void main()
{
    for (int i = 0; i < 3; ++i)
    {
        vec4 p0 = gl_in[i].gl_Position;
        vec4 p1 = gl_in[(i + 1) % 3].gl_Position;

        // Calculate the midpoint between the two vertices
        vec4 mid = (p0 + p1) / 2.0;

        // Emit the original vertices
        gl_Position = p0;
        gs_position = p0.xyz;
        EmitVertex();
    }
    EndPrimitive();
}
