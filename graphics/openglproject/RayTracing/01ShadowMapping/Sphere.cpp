#include "Sphere.h"

Sphere::Sphere(QString vs, QString fs, QString resource){
    m_model = new Model(resource);
    m_shader.addShaderFromSourceFile(QOpenGLShader::Vertex, vs);
    m_shader.addShaderFromSourceFile(QOpenGLShader::Fragment, fs);
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

void Sphere::Draw(QMatrix4x4 wood_box_model, QMatrix4x4 light_model, bool light) {
  if (!light) {
    m_model->Draw(&m_shader);
    m_shader.release();
    return;
  }

  m_shader.setUniformValue("objectColor", QVector3D{ 1.0f, 0.5f, 0.31f });

  m_shader.setUniformValue(
    "light.position",
    light_model * QVector3D{ 0., 0., 0. });
  m_shader.setUniformValue(
    "light.direction",
    wood_box_model * QVector3D{ 0., 0., 0. } -
    light_model * QVector3D{0., 0., 0.});
  m_shader.setUniformValue(
    "light.cutOff", false);
  m_shader.setUniformValue(
    "light.ambient",
    QVector3D{ 0.2f, 0.2f, 0.2f });
  m_shader.setUniformValue(
    "light.diffuse",
    QVector3D{ 1.5f, 1.5f, 1.5f });
  m_shader.setUniformValue(
    "light.specular",
    QVector3D{ 0.5f, 0.5f, 0.5f });
  m_shader.setUniformValue(
    "light.color",
    QVector3D{ 1.0f, 1.0f, 1.0f });

  m_shader.setUniformValue("model", wood_box_model);
  m_shader.setUniformValue("material.diffuse",
    QVector3D{ 1.0f, 0.5f, 0.31f });
  m_shader.setUniformValue("material.specular",
    QVector3D{ 0.5f, 0.5f, 0.5f });
  m_shader.setUniformValue("material.shininess", 32.0f);
  m_model->Draw(&m_shader);
  m_shader.release();
}