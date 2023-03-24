#include "Sphere.h"

Sphere::Sphere(){
    m_model = new Model("../../resouce/nanosuit/2.obj");
    m_shader.addShaderFromSourceFile(QOpenGLShader::Vertex, "./sphere.vs");
    m_shader.addShaderFromSourceFile(QOpenGLShader::Fragment, "./sphere.fs");
    m_shader.link();
}

void Sphere::bind(){
    m_shader.bind();
}

void Sphere::setUniformValue(const char* name, QMatrix4x4 value) {
    m_shader.setUniformValue(name, value);
}

void Sphere::setUniformValue(const char* name, int value){
    m_shader.setUniformValue(name, value);
}

void Sphere::setUniformValue(const char* name, QVector3D value){
    m_shader.setUniformValue(name, value);
}

void Sphere::Draw() {
  m_model->Draw(&m_shader);
  m_shader.release();
}