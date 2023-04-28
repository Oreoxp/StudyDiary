#include "rendererStruct.h"
#include <QOpenGLFunctions_3_0>

Mesh::Mesh(QVector<Vertex> vertices, QVector<GLuint> indices, QVector<Texture> textures) {
    this->vertices = vertices;
    this->indices = indices;
    this->textures = textures;
    this->setupMesh();
}

Mesh::Mesh(const Mesh& mesh){
    this->vertices = mesh.vertices;
    this->indices = mesh.indices;
    this->textures = mesh.textures;
    this->setupMesh();
}

Mesh& Mesh::operator=(const Mesh& mesh){
    this->vertices = mesh.vertices;
    this->indices = mesh.indices;
    this->textures = mesh.textures;
    this->setupMesh();
    return *this;
}

void Mesh::Draw(QOpenGLShaderProgram* shader) {
  initializeOpenGLFunctions();
    // bind appropriate textures
    GLuint diffuseNr = 1;
    GLuint specularNr = 1;
    for (GLuint i = 0; i < this->textures.size(); i++) {
        glActiveTexture(GL_TEXTURE0 + i); // active proper texture unit before binding
        // retrieve texture number (the N in diffuse_textureN)
        QString number;
        QString name = this->textures[i].type;
        if (name == "texture_diffuse")
            number = QString::number(diffuseNr++);
        else if (name == "texture_specular")
            number = QString::number(specularNr++);

        shader->setUniformValue((name + number).toStdString().c_str(), i);
        glBindTexture(GL_TEXTURE_2D, this->textures[i].id);
    }

    // draw mesh
    glBindVertexArray(this->VAO);
    glDrawElements(GL_TRIANGLES, this->indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // always good practice to set everything back to defaults once configured.
    glActiveTexture(GL_TEXTURE0);
}

void Mesh::setupMesh() {
  initializeOpenGLFunctions();
    // create buffers/arrays
    glGenVertexArrays(1, &this->VAO);
    glGenBuffers(1, &this->VBO);
    glGenBuffers(1, &this->EBO);

    glBindVertexArray(this->VAO);
    // load data into vertex buffers
    glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
    // A great thing about structs is that their memory layout is sequential for all its items.
    // The effect is that we can simply pass a pointer to the struct and it translates perfectly to a glm::vec3/2 array which
    // again translates to 3/2 floats which translates to a byte array.
    glBufferData(GL_ARRAY_BUFFER, this->vertices.size() * sizeof(Vertex), &this->vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->indices.size() * sizeof(GLuint), &this->indices[0], GL_STATIC_DRAW);

    // set the vertex attribute pointers
    // vertex Positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)0);
    // vertex normals
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, normal));
    // vertex texture coords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, texCoord));

    glBindVertexArray(0);
}

Model::Model(QString path) {
  initializeOpenGLFunctions();
  this->loadModel(path);
}

void Model::Draw(QOpenGLShaderProgram* shader) {
    for (GLuint i = 0; i < this->meshes.size(); i++)
        this->meshes[i].Draw(shader);
}

void Model::loadModel(QString path){
    Assimp::Importer import;
    const aiScene* scene = import.ReadFile(path.toStdString(), aiProcess_Triangulate | aiProcess_FlipUVs);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        qDebug() << "ERROR::ASSIMP::" << import.GetErrorString();
        return;
    }
    this->directory = path.left(path.lastIndexOf('/'));
    this->processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode* node, const aiScene* scene) {
    // process all the node's meshes (if any)
    for (GLuint i = 0; i < node->mNumMeshes; i++) {
        // the node object only contains indices to index the actual objects in the scene.
        // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        this->meshes.push_back(this->processMesh(mesh, scene));
    }
    // then do the same for each of its children
    for (GLuint i = 0; i < node->mNumChildren; i++)
        this->processNode(node->mChildren[i], scene);
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene) {
    // data to fill
    QVector<Vertex> vertices;
    QVector<GLuint> indices;
    QVector<Texture> textures;

    // Walk through each of the mesh's vertices
    for (GLuint i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        // process vertex positions, normals and texture coordinates
        vertex.position = QVector3D(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        vertex.normal = QVector3D(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
            vertex.texCoord = QVector2D(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        else
            vertex.texCoord = QVector2D(0.0f, 0.0f);
        vertices.push_back(vertex);
    }
    // now walk through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
    for (GLuint i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        // retrieve all indices of the face and store them in the indices vector
        for (GLuint j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }
    // process materials
    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        // we assume a convention for sampler names in the shaders. Each diffuse texture should be named
        // as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER.
        // Same applies to other texture as the following list summarizes:
        // diffuse: texture_diffuseN
        // specular: texture_specularN
        // normal: texture_normalN

        // 1. diffuse maps
        QVector<Texture> diffuseMaps = this->loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.append(diffuseMaps);
        // 2. specular maps
        QVector<Texture> specularMaps = this->loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
        textures.append(specularMaps);
        // 3. normal maps
        QVector<Texture> normalMaps = this->loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
        textures.append(normalMaps);
        // 4. height maps
        QVector<Texture> heightMaps = this->loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
        textures.append(heightMaps);
    }
    return Mesh(vertices, indices, textures);
}

QVector<Texture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, QString typeName) {
    QVector<Texture> textures;
    for (GLuint i = 0; i < mat->GetTextureCount(type); i++) {
        aiString str;
        mat->GetTexture(type, i, &str);
        // check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
        GLboolean skip = false;
        for (GLuint j = 0; j < this->textures_loaded.size(); j++) {
            if (this->textures_loaded[j].path == QString(str.C_Str())) {
                textures.push_back(this->textures_loaded[j]);
                skip = true;
                break;
            }
        }
         if (!skip) {  // if texture hasn't been loaded already, load it
            Texture texture;
            texture.id = TextureFromFile(str.C_Str(), this->directory);
            texture.type = typeName;
            texture.path = str.C_Str();
            textures.push_back(texture);
            this->textures_loaded.push_back(texture);  // store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
        }
    }
    return textures;
}

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif  // !STB_IMAGE_IMPLEMENTATION

GLuint Model::TextureFromFile(const char* path, QString directory) {
    QString filename = directory + '/' + path;

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(filename.toStdString().c_str(), &width,
                                    &height, &nrComponents, 0);
    if (data) {
      GLenum format = 0;
      if (nrComponents == 1)
        format = GL_RED;
      else if (nrComponents == 3)
        format = GL_RGB;
      else if (nrComponents == 4)
        format = GL_RGBA;

      glBindTexture(GL_TEXTURE_2D, textureID);
      glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
                   GL_UNSIGNED_BYTE, data);
      glGenerateMipmap(GL_TEXTURE_2D);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                      GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      stbi_image_free(data);
    } else {
      qDebug() << "Texture failed to load at path: " << path ;
      stbi_image_free(data);
    }

    return textureID;
}


void Model::getVertexDataTexture(OtherObject& obj) {
    auto vertexsData = meshes[0].vertices;
    if (vertexData.empty()) {
      for (auto item : vertexsData) {
        vertexData.push_back(item.position.x());
        vertexData.push_back(item.position.y());
        vertexData.push_back(item.position.z());
        nuomalData.push_back(item.normal.x());
        nuomalData.push_back(item.normal.y());
        nuomalData.push_back(item.normal.z());
      }
    }
    GLuint vertexDataTexture;
    glGenTextures(1, &vertexDataTexture);
    glBindTexture(GL_TEXTURE_2D, vertexDataTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F,  vertexData.size(), 1, 0, GL_RGB, GL_FLOAT, vertexData.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Create and upload normal data texture
    GLuint normalDataTexture;
    glGenTextures(1, &normalDataTexture);
    glBindTexture(GL_TEXTURE_2D, normalDataTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, nuomalData.size(), 1, 0, GL_RGB, GL_FLOAT, nuomalData.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    obj.vertices_id = vertexDataTexture;
    obj.normals_id = normalDataTexture;
    obj.vertices = vertexData.data();
    obj.normals = nuomalData.data();
}

int Model::getNumTriangles() {
    return meshes[0].indices.size()/3;
}