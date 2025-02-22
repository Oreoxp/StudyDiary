#pragma once

#include <QtGui/QOpenGLShaderProgram>
#include <QVector3D>
#include "rendererStruct.h"

class Sphere {
public:
  Sphere(QString vs = "./resource/sphereLast.vs", QString fs = "./resource/sphereLast.fs", QString resource = "../../resouce/sphere/sphere.obj");

    void Draw(QOpenGLFramebufferObject* draw_fbo, QMatrix4x4 wood_box_model, QMatrix4x4 light_model, bool light = false);
    void bind();
    void setUniformValue(const char* name, QMatrix4x4 value);
    void setUniformValue(const char* name, int value);
    void setUniformValue(const char* name, QVector3D value);

    void getVertexDataTexture(OtherObject&);
    int getNumTriangles();

private:
    QOpenGLShaderProgram m_shader;
    Model* m_model;
};
