#version 330 core

layout (triangles) in;
layout (line_strip, max_vertices = 146) out;

//细分参数
uniform int u_subdivisionLevel = 1;

out vec3 gs_barycentricCoords;
out vec3 gs_position;
out vec3 gs_color;

struct Triangle {
    vec4 p0, p1, p2;
};

void main()
{
    Triangle inputTriangle;
    inputTriangle.p0 = gl_in[0].gl_Position;
    inputTriangle.p1 = gl_in[1].gl_Position;
    inputTriangle.p2 = gl_in[2].gl_Position;

    Triangle triangles[146];
    int triangleCount = 0;
    triangles[triangleCount++] = inputTriangle;

    for (int level = 0; level < u_subdivisionLevel; ++level) {
        int newTriangleCount = 0;
        Triangle newTriangles[146];

        for (int i = 0; i < triangleCount; ++i) {
            Triangle currTriangle = triangles[i];

            vec4 p01 = (currTriangle.p0 + currTriangle.p1) / 2.0;
            vec4 p12 = (currTriangle.p1 + currTriangle.p2) / 2.0;
            vec4 p20 = (currTriangle.p2 + currTriangle.p0) / 2.0;

            newTriangles[newTriangleCount++] = Triangle(currTriangle.p0, p01, p20);
            newTriangles[newTriangleCount++] = Triangle(currTriangle.p1, p12, p01);
            newTriangles[newTriangleCount++] = Triangle(currTriangle.p2, p20, p12);
            newTriangles[newTriangleCount++] = Triangle(p01, p12, p20);
        }

        triangleCount = newTriangleCount;
        for (int i = 0; i < triangleCount; ++i) {
            triangles[i] = newTriangles[i];
        }
    }

    for (int i = 0; i < triangleCount; ++i) {
        Triangle currTriangle = triangles[i];
        gs_color = vec3(1.0, 0.0, 0.0);
        gl_Position = currTriangle.p0;
        EmitVertex();
        gs_color = vec3(1.0, 0.0, 0.0);
        gl_Position = currTriangle.p1;
        EmitVertex();
        gs_color = vec3(1.0, 0.0, 0.0);
        gl_Position = currTriangle.p2;
        EmitVertex();
        EndPrimitive();
    }
}
