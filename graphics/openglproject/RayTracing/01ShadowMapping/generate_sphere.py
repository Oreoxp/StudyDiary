import math

def generate_sphere_obj_file(radius, latitudes, longitudes, output_file):
    with open(output_file, 'w') as obj_file:
        # Write vertices and normals
        for i in range(latitudes + 1):
            theta = i * math.pi / latitudes
            y = radius * math.cos(theta)
            for j in range(longitudes + 1):
                phi = j * 2 * math.pi / longitudes
                x = radius * math.sin(theta) * math.cos(phi)
                z = radius * math.sin(theta) * math.sin(phi)
                obj_file.write(f"v {x} {y} {z}\n")
                obj_file.write(f"vn {math.sin(theta) * math.cos(phi)} {math.cos(theta)} {math.sin(theta) * math.sin(phi)}\n")

        # Write faces
        for i in range(latitudes):
            for j in range(longitudes):
                v1 = i * (longitudes + 1) + j + 1
                v2 = i * (longitudes + 1) + j + 2
                v3 = (i + 1) * (longitudes + 1) + j + 1
                v4 = (i + 1) * (longitudes + 1) + j + 2

                obj_file.write(f"f {v1}//{v1} {v2}//{v2} {v3}//{v3}\n")
                obj_file.write(f"f {v2}//{v2} {v4}//{v4} {v3}//{v3}\n")

if __name__ == '__main__':
    radius = 1
    latitudes = 40
    longitudes = 40
    output_file = 'sphere11.obj'
    generate_sphere_obj_file(radius, latitudes, longitudes, output_file)
