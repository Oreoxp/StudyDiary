def write_square_obj(filename):
    vertices = [
        (-0.5, -0.5, 0),
        (0.5, -0.5, 0),
        (0.5, 0.5, 0),
        (-0.5, 0.5, 0)
    ]

    faces = [
        (1, 2, 3),
        (1, 3, 4)
    ]

    normals = [
        (0, 0, 1)
    ]

    with open(filename, 'w') as f:
        # Write vertices
        for vertex in vertices:
            f.write(f'v {vertex[0]} {vertex[1]} {vertex[2]}\n')

        # Write normals
        for normal in normals:
            f.write(f'vn {normal[0]} {normal[1]} {normal[2]}\n')

        # Write faces with normal indices
        for face in faces:
            f.write(f'f {face[0]}//1 {face[1]}//1 {face[2]}//1\n')


if __name__ == '__main__':
    write_square_obj('square.obj')
