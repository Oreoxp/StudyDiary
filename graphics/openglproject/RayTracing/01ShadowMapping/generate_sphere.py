import math

def generate_sphere_obj_file(radius, latitudes, longitudes, output_file):
    with open(output_file, 'w') as obj_file:
        # Write vertices, normals, and texture coordinates
        for i in range(latitudes + 1):
            theta = i * math.pi / latitudes
            y = radius * math.cos(theta)
            v = 1 - i / latitudes
            for j in range(longitudes + 1):
                phi = j * 2 * math.pi / longitudes
                x = radius * math.sin(theta) * math.cos(phi)
                z = radius * math.sin(theta) * math.sin(phi)
                u = j / longitudes
                obj_file.write(f"v {x:.6f} {y:.6f} {z:.6f}\n")
                obj_file.write(f"vn {math.sin(theta) * math.cos(phi):.6f} {math.cos(theta):.6f} {math.sin(theta) * math.sin(phi):.6f}\n")
                obj_file.write(f"vt {u:.6f} {v:.6f}\n")

        # Write faces
        for i in range(latitudes):
            for j in range(longitudes):
                index = i * (longitudes + 1) + j + 1
                v1 = index
                v2 = index + 1
                v3 = index + longitudes + 1
                v4 = index + longitudes + 2

                obj_file.write(f"f {v1}/{v1}/{v1} {v2}/{v2}/{v2} {v3}/{v3}/{v3}\n")
                obj_file.write(f"f {v2}/{v2}/{v2} {v4}/{v4}/{v4} {v3}/{v3}/{v3}\n")

if __name__ == '__main__':
    radius = 1
    latitudes = 40
    longitudes = 40
    output_file = 'sphere_with_texcoords_6decimal.obj'
    generate_sphere_obj_file(radius, latitudes, longitudes, output_file)
