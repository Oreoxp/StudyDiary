#include <QOpenGLFunctions_3_0>
#include <QtOpenGL/QOpenGLShaderProgram>
#include <QtOpenGL/QOpenGLTexture>
#include <QOpenGLShaderProgram>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QQuickFramebufferObject>
#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLVertexArrayObject>
#include <QElapsedTimer>

#include <../assimp/Importer.hpp>
#include <../assimp/scene.h>
#include <../assimp/postprocess.h>


struct Vertex {
    QVector3D position;
    QVector3D normal;
    QVector2D texCoord;
};

struct Texture {
    GLuint id;
    QString type;
    QString path;
};

class Mesh : protected QOpenGLFunctions_3_0 {
 public:
    Mesh(QVector<Vertex> vertices, QVector<GLuint> indices, QVector<Texture> textures);
    void Draw(QOpenGLShaderProgram* shader);
    Mesh(const Mesh& mesh);
    Mesh& operator=(const Mesh& mesh);

    GLuint VAO, VBO, EBO;
    QVector<Vertex> vertices;
    QVector<GLuint> indices;
    QVector<Texture> textures;
    void setupMesh();
};

class Model : protected QOpenGLFunctions_3_0 {
 public:
    Model(QString path);
    void Draw(QOpenGLShaderProgram* shader);
    std::vector<float> getModelData();
private:
    QVector<Mesh> meshes;
    QString directory;
    QVector<Texture> textures_loaded;
    void loadModel(QString path);
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
    QVector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type,
        QString typeName);
    GLuint TextureFromFile(const char* path, QString directory);
};
