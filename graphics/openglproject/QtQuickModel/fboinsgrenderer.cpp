#include "fboinsgrenderer.h"

#include <QtOpenGL/QOpenGLFramebufferObject>

#include <QtQuick/QQuickWindow>
#include <qsgsimpletexturenode.h>
#include <QTransform>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>


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
  connect(this, SIGNAL(sliderValueChanged(int)), render_,
          SLOT(onSliderValueChanged(int)), Qt::QueuedConnection);
  return render_;
}

void GLFWItem::changeTrianglePos() {
  emit trianglePosChanged();
}

void GLFWItem::changeKeyDown(GLFWItem::CLICK_TYPE type) {
  emit keyDownChanged(type);
}

void GLFWItem::changeSliderValue(int value) {
  emit sliderValueChanged(value);
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
    m_light_cube_shader.addCacheableShaderFromSourceFile(
        QOpenGLShader::ShaderTypeBit::Geometry, "./12.geom");
    m_light_cube_shader.link();

    m_model = new Model("../resouce/nanosuit/2.obj");
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

  m_light_cube_shader.bind();
  m_light_cube_shader.setUniformValue("view", view);
  m_light_cube_shader.setUniformValue("projection", projection);

  // render the loaded model
  QMatrix4x4 model;
  model.translate(
      QVector3D(0.0f, 0.0f,
                0.0f));  // translate it down so it's at the center of the scene
  model.scale(2.0f);
  model.rotate(qRadiansToDegrees(45.0f), 0.0, 0., 1.);
  model.rotate(qRadiansToDegrees(90.0f), 1.0, 1., 0.);
  m_light_cube_shader.setUniformValue("model", model);
  m_light_cube_shader.setUniformValue("u_subdivisionLevel", subdivisionLevel);
  m_model->Draw(&m_light_cube_shader);
  m_light_cube_shader.release();
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

void GLFWRenderer::onSliderValueChanged(int value) {
  qDebug() << "onSliderValueChanged" << value;
  subdivisionLevel = value / 50.0;
}