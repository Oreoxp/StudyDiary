#include <iostream>
#include <vector>
#include <cmath>
#include <unordered_set>
#include <fstream>

struct Vec3 {
    float x, y, z;

    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vec3 normalized() const {
        float length = std::sqrt(x * x + y * y + z * z);
        return Vec3(x / length, y / length, z / length);
    }
};

struct Triangle {
    Vec3 v0, v1, v2;

    Triangle(const Vec3& v0, const Vec3& v1, const Vec3& v2) : v0(v0), v1(v1), v2(v2) {}
};

void subdivide(const Triangle& t, std::vector<Triangle>& triangles, int depth) {
    if (depth == 0) {
        triangles.push_back(t);
        return;
    }

    Vec3 mid_v0_v1 = t.v0.normalized() + t.v1.normalized();
    Vec3 mid_v1_v2 = t.v1.normalized() + t.v2.normalized();
    Vec3 mid_v2_v0 = t.v2.normalized() + t.v0.normalized();

    mid_v0_v1 = mid_v0_v1.normalized();
    mid_v1_v2 = mid_v1_v2.normalized();
    mid_v2_v0 = mid_v2_v0.normalized();

    subdivide(Triangle(t.v0, mid_v0_v1, mid_v2_v0), triangles, depth - 1);
    subdivide(Triangle(t.v1, mid_v1_v2, mid_v0_v1), triangles, depth - 1);
    subdivide(Triangle(t.v2, mid_v2_v0, mid_v1_v2), triangles, depth - 1);
    subdivide(Triangle(mid_v0_v1, mid_v1_v2, mid_v2_v0), triangles, depth - 1);
}

std::vector<Triangle> createIcosphere(int subdivisions) {
    float t = (1.0f + std::sqrt(5.0f)) / 2.0f;

    std::vector<Vec3> vertices = {
        {-1, t, 0}, {1, t, 0}, {-1, -t, 0}, {1, -t, 0},
        {0, -1, t}, {0, 1, t}, {0, -1, -t}, {0, 1, -t},
        {t, 0, -1}, {t, 0, 1}, {-t, 0, -1}, {-t, 0, 1}
    };

    std::vector<Triangle> baseTriangles = {
        {vertices[0], vertices[11], vertices[5]},
        {vertices[0], vertices[5], vertices[1]},
        {vertices[0], vertices[1], vertices[7]},
        {vertices[0], vertices[7], vertices[10]},
        {vertices[0], vertices[10], vertices[11]},
        {vertices[1], vertices[5], vertices[9]},
        {vertices[5], vertices[11], vertices[4]},
        {vertices[11], vertices[10], vertices[2]},
        {vertices[10], vertices[7], vertices[6]},
        {vertices[7], vertices[1], vertices[8]},
        {vertices[3], vertices[9], vertices[4]},
        {vertices[3], vertices[4], vertices[2]},
        {vertices[3], vertices[2], vertices[6]},
        {vertices[3], vertices[6], vertices[8]},
        {vertices[3], vertices[8], vertices[9]},
        {vertices[4], vertices[9], vertices[5]},
        {vertices[2], vertices[4], vertices[11]},
        {vertices[6], vertices[2], vertices[10]},
        {vertices[8], vertices[6], vertices[7]},
        {vertices[9], vertices[8], vertices[1]}
    };

    std::vector<Triangle> triangles;
    for (const auto& baseTriangle : baseTriangles) {
        subdivide(baseTriangle, triangles, subdivisions);
    }

    return triangles;
}


void writeObjFile(const std::vector<Triangle>& triangles, const std::string& filename) {
    std::ofstream obj_file(filename);

    for (const auto& triangle : triangles) {
        obj_file << "v " << triangle.v0.x << " " << triangle.v0.y << " " << triangle.v0.z << std::endl;
        obj_file << "v " << triangle.v1.x << " " << triangle.v1.y << " " << triangle.v1.z << std::endl;
        obj_file << "v " << triangle.v2.x << " " << triangle.v2.y << " " << triangle.v2.z << std::endl;
    }

    size_t vertices_count = triangles.size() * 3;
    for (size_t i = 1; i <= vertices_count; i += 3) {
        obj_file << "f " << i << " " << (i + 1) << " " << (i + 2) << std::endl;
    }

    obj_file.close();
}

int main() {
    int subdivisions = 3;
    std::vector<Triangle> triangles = createIcosphere(subdivisions);
    writeObjFile(triangles, "sphere.obj");
    std::cout << "Sphere obj file created." << std::endl;

    return 0;
}
