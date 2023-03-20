#pragma once

#include <QtOpenGL/QOpenGLShaderProgram>
#include "rendererStruct.h"

class Sphere {
public:
    Sphere();
    void Draw();
    void bind();
    void setUniformValue(const char* name, QMatrix4x4 value);


private:
    QOpenGLShaderProgram m_shader;
    Model* m_model;
};
