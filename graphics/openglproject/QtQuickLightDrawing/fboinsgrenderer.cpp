#include "fboinsgrenderer.h"

#include <QtOpenGL/QOpenGLFramebufferObject>

#include <QtQuick/QQuickWindow>
#include <qsgsimpletexturenode.h>
#include <QTransform>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


GLFWItem::GLFWItem() {}

GLFWItem::~GLFWItem() {}

GLFWRenderer* render_;

QQuickFramebufferObject::Renderer* GLFWItem::createRenderer() const {
  render_ = new GLFWRenderer();
  connect(this, SIGNAL(trianglePosChanged()), render_,
          SLOT(onTrianglePosChanged()), Qt::QueuedConnection);
  connect(this, SIGNAL(keyDownChanged(GLFWItem::CLICK_TYPE)), render_,
          SLOT(onKeyDownChanged(GLFWItem::CLICK_TYPE)),
          Qt::QueuedConnection);
  return render_;
}

void GLFWItem::changeTrianglePos() {
  emit trianglePosChanged();
}

void GLFWItem::changeKeyDown(GLFWItem::CLICK_TYPE type) {
  emit keyDownChanged(type);
}


    float vertices[] = {
    // positions            // normals              // texture coords
    -0.5f, -0.5f, -0.5f,     0.0f,  0.0f,  -1.0f,    0.0f,  0.0f,  
    0.5f,  -0.5f, -0.5f,     0.0f,  0.0f,  -1.0f,    1.0f,  0.0f,   
    0.5f,  0.5f,  -0.5f,     0.0f,  0.0f,  -1.0f,    1.0f,  1.0f,  
    0.5f,  0.5f,  -0.5f,     0.0f,  0.0f,  -1.0f,    1.0f,  1.0f,  
    -0.5f, 0.5f,  -0.5f,     0.0f,  0.0f,  -1.0f,    0.0f,  1.0f,
    -0.5f, -0.5f, -0.5f,     0.0f,  0.0f,  -1.0f,    0.0f,  0.0f,

    -0.5f, -0.5f, 0.5f,      0.0f,  0.0f,  1.0f,     0.0f,  0.0f,  
    0.5f,  -0.5f, 0.5f,      0.0f,  0.0f,  1.0f,     1.0f,  0.0f,   
    0.5f,  0.5f,  0.5f,      0.0f,  0.0f,  1.0f,     1.0f,  1.0f,   
    0.5f,  0.5f,  0.5f,      0.0f,  0.0f,  1.0f,     1.0f,  1.0f,  
    -0.5f, 0.5f,  0.5f,      0.0f,  0.0f,  1.0f,     0.0f,  1.0f, 
    -0.5f, -0.5f, 0.5f,      0.0f,  0.0f,  1.0f,     0.0f,  0.0f,
    
    -0.5f, 0.5f,  0.5f,      -1.0f, 0.0f,  0.0f,   1.0f,  0.0f,  -0.5f, 0.5f,
    -0.5f, -1.0f, 0.0f,      0.0f,  1.0f,  1.0f,   -0.5f, -0.5f, -0.5f, -1.0f,
    0.0f,  0.0f,  0.0f,      1.0f,  -0.5f, -0.5f,  -0.5f, -1.0f, 0.0f,  0.0f,
    0.0f,  1.0f,  -0.5f,     -0.5f, 0.5f,  -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
    -0.5f, 0.5f,  0.5f,      -1.0f, 0.0f,  0.0f,   1.0f,  0.0f,

    0.5f,  0.5f,  0.5f,      1.0f,  0.0f,  0.0f,   1.0f,  0.0f,  0.5f,  0.5f,
    -0.5f, 1.0f,  0.0f,      0.0f,  1.0f,  1.0f,   0.5f,  -0.5f, -0.5f, 1.0f,
    0.0f,  0.0f,  0.0f,      1.0f,  0.5f,  -0.5f,  -0.5f, 1.0f,  0.0f,  0.0f,
    0.0f,  1.0f,  0.5f,      -0.5f, 0.5f,  1.0f,   0.0f,  0.0f,  0.0f,  0.0f,
    0.5f,  0.5f,  0.5f,      1.0f,  0.0f,  0.0f,   1.0f,  0.0f,

    -0.5f, -0.5f, -0.5f,     0.0f,  -1.0f, 0.0f,   0.0f,  1.0f,  0.5f,  -0.5f,
    -0.5f, 0.0f,  -1.0f,     0.0f,  1.0f,  1.0f,   0.5f,  -0.5f, 0.5f,  0.0f,
    -1.0f, 0.0f,  1.0f,      0.0f,  0.5f,  -0.5f,  0.5f,  0.0f,  -1.0f, 0.0f,
    1.0f,  0.0f,  -0.5f,     -0.5f, 0.5f,  0.0f,   -1.0f, 0.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f,     0.0f,  -1.0f, 0.0f,   0.0f,  1.0f,

    -0.5f, 0.5f,  -0.5f,     0.0f,  1.0f,  0.0f,   0.0f,  1.0f,  0.5f,  0.5f,
    -0.5f, 0.0f,  1.0f,      0.0f,  1.0f,  1.0f,   0.5f,  0.5f,  0.5f,  0.0f,
    1.0f,  0.0f,  1.0f,      0.0f,  0.5f,  0.5f,   0.5f,  0.0f,  1.0f,  0.0f,
    1.0f,  0.0f,  -0.5f,     0.5f,  0.5f,  0.0f,   1.0f,  0.0f,  0.0f,  0.0f,
    -0.5f, 0.5f,  -0.5f,     0.0f,  1.0f,  0.0f,   0.0f,  1.0f};

QVector3D cubePositions[] = {
    QVector3D(0.0f, 0.0f, 0.0f),    QVector3D(2.0f, 5.0f, -15.0f),
    QVector3D(-1.5f, -2.2f, -2.5f), QVector3D(-3.8f, -2.0f, -12.3f),
    QVector3D(2.4f, -0.4f, -3.5f),  QVector3D(-1.7f, 3.0f, -7.5f),
    QVector3D(1.3f, -2.0f, -2.5f),  QVector3D(1.5f, 2.0f, -2.5f),
    QVector3D(1.5f, 0.2f, -1.5f),   QVector3D(-1.3f, 1.0f, -1.5f)
};

GLFWRenderer::GLFWRenderer()
    : m_fbo(nullptr),
      m_vao(0), m_vbo(0), m_program(0) {
    initializeOpenGLFunctions();
  if (!m_program) {
    // Make sure a valid OpenGL context is current
    QOpenGLContext* ctx = QOpenGLContext::currentContext();
    if (!ctx) {
      qWarning() << "No current OpenGL context";
      return;
    }
    if (!ctx->isValid()) {
      qWarning() << "Invalid OpenGL context";
      return;
    }

    m_light_shader.addCacheableShaderFromSourceFile(
        QOpenGLShader::ShaderTypeBit::Vertex, "./11.vert");
    m_light_shader.addCacheableShaderFromSourceFile(
        QOpenGLShader::ShaderTypeBit::Fragment, "./11.fs");

    m_light_cube_shader.addCacheableShaderFromSourceFile(
            QOpenGLShader::ShaderTypeBit::Vertex, "./12.vert");
    m_light_cube_shader.addCacheableShaderFromSourceFile(
        QOpenGLShader::ShaderTypeBit::Fragment, "./12.fs");

    if (!m_light_shader.link())
      qWarning() << m_light_shader.log();

    if (!m_light_cube_shader.link())
      qWarning() << m_light_cube_shader.log();

    //load texture
    loadTexture("./container2.png", &m_diffuse_map);

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindVertexArray(m_vao);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // second, configure the light's VAO (VBO stays the same; the vertices are
    // the same for the light object which is also a 3D cube)
    glGenVertexArrays(1, &m_light_cube_vao);
    glBindVertexArray(m_light_cube_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void*)0);
    glEnableVertexAttribArray(0);
  }
    timer.start();
}

GLFWRenderer::~GLFWRenderer() {
    if (m_program) {
        glDeleteProgram(m_program);
    }
    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
    }
    if (m_vbo) {
        glDeleteBuffers(1, &m_vbo);
    }
}

QOpenGLFramebufferObject* GLFWRenderer::createFramebufferObject(
    const QSize& size) {
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(4);
    m_fbo = new QOpenGLFramebufferObject(size, format);
    return m_fbo;
}


struct LIGHT{
  QMatrix4x4 model;
  bool is_point;
};

void GLFWRenderer::render() {
  // Blit FBO to default framebuffer
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo->handle());

  // Render to FBO
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glEnable(GL_MULTISAMPLE);

  glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  QVector3D lightPos(5.2f, 5.0f, 2.0f);
  // calculate the model matrix for each object and pass it to shader
  // before drawing
  QMatrix4x4 view{};
  QMatrix4x4 projection{};
  // model1.rotate(qRadiansToDegrees(10.0f) * timer.elapsed() * 0.0001, 0, 0,
  // 1);
  view.lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
  view.rotate(qRadiansToDegrees(45.0f), 1.0, 0., 0.);
  view.rotate(qRadiansToDegrees(-50.0f), 0.0, 0., 1.);
  // view.translate(m_view);
  projection.perspective(qRadiansToDegrees(45.0f), (float)800 / (float)800,
                         0.1f, 100.0f);

  std::vector<LIGHT> lights;
  QMatrix4x4 light_model_white_signed = QMatrix4x4();
  light_model_white_signed.scale(0.2f);
  light_model_white_signed.rotate(
      qRadiansToDegrees(10.0f * timer.elapsed() / 12000), 0, 0, 1);
  light_model_white_signed.translate({0, 0, 3});
  light_model_white_signed.translate(lightPos);

  m_light_cube_shader.bind();
  m_light_cube_shader.setUniformValue("view", view);
  m_light_cube_shader.setUniformValue("projection", projection);
  m_light_cube_shader.setUniformValue("model", light_model_white_signed);
  m_light_cube_shader.setUniformValue("lightColor", QVector3D(1.0, 1.0, 1.0));
  glBindVertexArray(m_light_cube_vao);
  glDrawArrays(GL_TRIANGLES, 0, 36);

  QMatrix4x4 light_model_green_all = QMatrix4x4();
  light_model_green_all.scale(0.2f);
  light_model_green_all.rotate(
      qRadiansToDegrees(10.0f * timer.elapsed() / 12000), 1, 0, 0);
  light_model_green_all.translate({-5, 0, 3});
  light_model_green_all.translate(lightPos);

  m_light_cube_shader.bind();
  m_light_cube_shader.setUniformValue("view", view);
  m_light_cube_shader.setUniformValue("projection", projection);
  m_light_cube_shader.setUniformValue("model", light_model_green_all);
  m_light_cube_shader.setUniformValue("lightColor", QVector3D(1.0, 1.0, 0.0));
  lights.push_back({light_model_green_all, false});
  lights.push_back({light_model_white_signed, true});
  glBindVertexArray(m_light_cube_vao);
  glDrawArrays(GL_TRIANGLES, 0, 36);

  m_light_cube_shader.release();

  m_light_shader.bind();
  m_fbo->bind();
  for (unsigned int i = 0; i < 10; i++) {
    m_light_shader.setUniformValue("objectColor", QVector3D{1.0f, 0.5f, 0.31f});
    m_light_shader.setUniformValue("view", view);
    m_light_shader.setUniformValue("projection", projection);
    m_light_shader.setUniformValue("viewPos", view * QVector3D{0., 0., 0.});

    // calculate the model matrix for each object and pass it to shader
    // before drawing
    QMatrix4x4 wood_box_model;
    wood_box_model.translate(cubePositions[i]);

    for (unsigned int j = 0; j < lights.size(); j++) {
      m_light_shader.setUniformValue(
          QString("Lights[%1].position").arg(j).toLocal8Bit().data(),
          lights[j].model * QVector3D{0., 0., 0.});
      m_light_shader.setUniformValue(
          QString("Lights[%1].direction").arg(j).toLocal8Bit().data(),
          wood_box_model * QVector3D{0., 0., 0.} -
              lights[j].model * QVector3D{0., 0., 0.});
      m_light_shader.setUniformValue(
          QString("Lights[%1].cutOff").arg(j).toLocal8Bit().data(),
          lights[j].is_point ? 20.0f : 0);
      m_light_shader.setUniformValue(
          QString("Lights[%1].ambient").arg(j).toLocal8Bit().data(),
          QVector3D{0.2f, 0.2f, 0.2f});
      m_light_shader.setUniformValue(
          QString("Lights[%1].diffuse").arg(j).toLocal8Bit().data(),
          QVector3D{1.5f, 1.5f, 1.5f});
      m_light_shader.setUniformValue(
          QString("Lights[%1].specular").arg(j).toLocal8Bit().data(),
          QVector3D{0.5f, 0.5f, 0.5f});
      m_light_shader.setUniformValue(
          QString("Lights[%1].color").arg(j).toLocal8Bit().data(),
          QVector3D{1.0f, 1.0f, 1.0f});
    }

    m_light_shader.setUniformValue("model", wood_box_model);
    m_light_shader.setUniformValue("material.diffuse",
                                   QVector3D{1.0f, 0.5f, 0.31f});
    m_light_shader.setUniformValue("material.specular",
                                   QVector3D{0.5f, 0.5f, 0.5f});
    m_light_shader.setUniformValue("material.shininess", 32.0f);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_diffuse_map);
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);
  }
  // render the cube

  glBindVertexArray(0);

  m_fbo->release();
  m_light_shader.release();

  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
}


void GLFWRenderer::mvp() {

}

void GLFWRenderer::onTrianglePosChanged() {
    clearWindow();
    
    float newvertices[] = {
        0.6f,  0.5f,  0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f,  
        0.6f,  -0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f,  
        -0.6f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,  
        -0.6f, 0.5f,  0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f   
    };

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(newvertices), newvertices,
                 GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void*)0);
    glEnableVertexAttribArray(0);
    // normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    update();
}

void GLFWRenderer::clearWindow() {
    m_fbo->release();
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo->handle());

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLFWRenderer::onKeyDownChanged(GLFWItem::CLICK_TYPE type) {
    switch (type) {
        case GLFWItem::CLICK_TYPE::DOWN_UP:
        //m_view.setY(m_view.y() + 0.1f);
        cameraPos += cameraSpeed * cameraFront;
        break;
        case GLFWItem::CLICK_TYPE::DOWN_DOWN:
        cameraPos -= cameraSpeed * cameraFront;
        //m_view.setY(m_view.y() - 0.1f);
        break;
        case GLFWItem::CLICK_TYPE::DOWN_LEFT:
        cameraPos -= QVector3D::crossProduct(cameraFront, cameraUp).normalized() *
                     cameraSpeed;
        //m_view.setX(m_view.x() - 0.1f);
        break;
        case GLFWItem::CLICK_TYPE::DOWN_RIGHT:
        cameraPos +=
            QVector3D::crossProduct(cameraFront, cameraUp).normalized() *
            cameraSpeed;
        //m_view.setX(m_view.x() + 0.1f);
        break;
        default:
        break;
    }
}

void GLFWRenderer::loadTexture(QString path, GLuint* texture){
  // load and create a texture
  // -------------------------
  glGenTextures(1, texture);
  glBindTexture(GL_TEXTURE_2D, *texture); // all upcoming GL_TEXTURE_2D operations now have effect on this texture object
  // set the texture wrapping parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  // set texture filtering parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  QImage image(path);
  image = image.convertToFormat(QImage::Format_RGBA8888);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width(), image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, image.bits());
  glGenerateMipmap(GL_TEXTURE_2D);
}