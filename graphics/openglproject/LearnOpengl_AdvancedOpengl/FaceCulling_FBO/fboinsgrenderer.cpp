#include "fboinsgrenderer.h"

#include <QtOpenGL/QOpenGLFramebufferObject>

#include <qsgsimpletexturenode.h>
#include <QTransform>
#include <QtQuick/QQuickWindow>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

GLFWItem::GLFWItem() {}

GLFWItem::~GLFWItem() {}

GLFWRenderer* render_;

QQuickFramebufferObject::Renderer* GLFWItem::createRenderer() const {
  render_ = new GLFWRenderer();
  connect(this, SIGNAL(trianglePosChanged()), render_,
          SLOT(onTrianglePosChanged()), Qt::QueuedConnection);
  connect(this, SIGNAL(keyDownChanged(GLFWItem::CLICK_TYPE)), render_,
          SLOT(onKeyDownChanged(GLFWItem::CLICK_TYPE)), Qt::QueuedConnection);
  return render_;
}

void GLFWItem::changeTrianglePos() {
  emit trianglePosChanged();
}

void GLFWItem::changeKeyDown(GLFWItem::CLICK_TYPE type) {
  emit keyDownChanged(type);
}

// Create VAO and VBO
float vertices[] = {
    // positions          // texture Coords
    -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0.5f,  -0.5f, -0.5f, 1.0f, 0.0f,
    0.5f,  0.5f,  -0.5f, 1.0f, 1.0f, 0.5f,  0.5f,  -0.5f, 1.0f, 1.0f,
    -0.5f, 0.5f,  -0.5f, 0.0f, 1.0f, -0.5f, -0.5f, -0.5f, 0.0f, 0.0f,

    -0.5f, -0.5f, 0.5f,  0.0f, 0.0f, 0.5f,  -0.5f, 0.5f,  1.0f, 0.0f,
    0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
    -0.5f, 0.5f,  0.5f,  0.0f, 1.0f, -0.5f, -0.5f, 0.5f,  0.0f, 0.0f,

    -0.5f, 0.5f,  0.5f,  1.0f, 0.0f, -0.5f, 0.5f,  -0.5f, 1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
    -0.5f, -0.5f, 0.5f,  0.0f, 0.0f, -0.5f, 0.5f,  0.5f,  1.0f, 0.0f,

    0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.5f,  0.5f,  -0.5f, 1.0f, 1.0f,
    0.5f,  -0.5f, -0.5f, 0.0f, 1.0f, 0.5f,  -0.5f, -0.5f, 0.0f, 1.0f,
    0.5f,  -0.5f, 0.5f,  0.0f, 0.0f, 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.5f,  -0.5f, -0.5f, 1.0f, 1.0f,
    0.5f,  -0.5f, 0.5f,  1.0f, 0.0f, 0.5f,  -0.5f, 0.5f,  1.0f, 0.0f,
    -0.5f, -0.5f, 0.5f,  0.0f, 0.0f, -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,

    -0.5f, 0.5f,  -0.5f, 0.0f, 1.0f, 0.5f,  0.5f,  -0.5f, 1.0f, 1.0f,
    0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f, 0.5f,  0.5f,  0.0f, 0.0f, -0.5f, 0.5f,  -0.5f, 0.0f, 1.0f};

GLuint initCubemapTexture() {
  GLuint cubeMapTexture;
  glGenTextures(1, &cubeMapTexture);
  glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);

  for (unsigned int i = 0; i < 6; ++i) {
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, 800, 800, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, nullptr);
  }

  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  return cubeMapTexture;
}

GLFWRenderer::GLFWRenderer()
    : m_fbo(nullptr), m_vao(0), m_vbo(0), m_program(0) {
  initializeOpenGLFunctions();
  m_shader = new QOpenGLShaderProgram();
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
    // open files
    std::ifstream vShaderFile;
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    vShaderFile.open("./11.vs");
    std::stringstream vShaderStream;
    vShaderStream << vShaderFile.rdbuf();
    vShaderFile.close();

    std::string vertexShaderSource = vShaderStream.str();
    const char* vShaderCode = vertexShaderSource.c_str();

    // open files
    std::ifstream fShaderFile;
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.open("./11.fs");
    std::stringstream fShaderStream;
    fShaderStream << fShaderFile.rdbuf();
    fShaderFile.close();

    std::string fShaderSource = fShaderStream.str();
    const char* fShaderCode = fShaderSource.c_str();

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vertexShader, 1, &vShaderCode, nullptr);
    glCompileShader(vertexShader);

    int success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);

    glShaderSource(fragmentShader, 1, &fShaderCode, nullptr);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);

    m_program = glCreateProgram();
    glAttachShader(m_program, vertexShader);
    glAttachShader(m_program, fragmentShader);
    glLinkProgram(m_program);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    m_shader->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex,
                                               "./sphere.vs");
    m_shader->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment,
                                               "./sphere.fs");
    m_shader->link();
  }

  if (!m_vao) {
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          (void*)(3 * sizeof(float)));

    cubeMapTexture = initCubemapTexture();
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
  back_fbo = new QOpenGLFramebufferObject(size, format);
  return m_fbo;
}

void GLFWRenderer::render() {
  // Render to first FBO
  {
    back_fbo->bind();
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glUseProgram(m_program);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_MULTISAMPLE);

    QMatrix4x4 view{};
    QMatrix4x4 projection{};

    view.lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    view.rotate(qRadiansToDegrees(20.0f), 1, 1, 0);
    projection.perspective(qRadiansToDegrees(45.0f), (float)800 / (float)800,
                           0.1f, 100.0f);

    glUniformMatrix4fv(glGetUniformLocation(m_program, "view"), 1, GL_FALSE,
                       view.data());
    glUniformMatrix4fv(glGetUniformLocation(m_program, "projection"), 1,
                       GL_FALSE, projection.data());

    glBindVertexArray(m_vao);

    QMatrix4x4 model;
    model.translate(QVector3D(0, 0, 0));
    glUniformMatrix4fv(glGetUniformLocation(m_program, "model"), 1, GL_FALSE,
                       model.data());

    glDrawArrays(GL_TRIANGLES, 0, 36);


    // 将 FBO 纹理转换为立方体纹理
    glBindTexture(GL_TEXTURE_2D, back_fbo->texture());
    for (unsigned int i = 0; i < 6; ++i) {
      glCopyTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, 0, 0, 0, 0,
                          800, 800);
    }
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      qDebug() << "error";
    }
    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);
    back_fbo->release();
  }

  m_fbo->bind();
  glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  glUseProgram(m_program);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glEnable(GL_MULTISAMPLE);

  QMatrix4x4 view{};
  QMatrix4x4 projection{};
  QMatrix4x4 model;

  view.lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
  view.rotate(qRadiansToDegrees(20.0f), 1, 1, 0);
  projection.perspective(qRadiansToDegrees(45.0f), (float)800 / (float)800,
                         0.1f, 100.0f);
  model.translate(QVector3D(0, 0, 0));

  glBindVertexArray(m_vao);

  m_shader->bind();
  m_shader->setUniformValue("view", view);
  m_shader->setUniformValue("projection", projection);
  m_shader->setUniformValue("model", model);
  m_shader->setUniformValue("view_pos", view * QVector3D(0, 0, 0));
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);
  m_shader->setUniformValue("envMap", 0);
  m_shader->setUniformValue("CameraPos", cameraPos);
  m_shader->setUniformValue("LightPos", QVector3D(5.0f, 5.0f, 5.0f));
  m_shader->setUniformValue("LightColor", QVector3D(1.0f, 1.0f, 1.0f));

  glDrawArrays(GL_TRIANGLES, 0, 36);
  m_shader->release();
  /*
  // Render sphere with texture from first FBO
  m_sphere.bind();
  m_sphere.setUniformValue("view", view);
  m_sphere.setUniformValue("projection", projection);
  QMatrix4x4 model2;
  model2.scale(0.3f);
  model2.translate(QVector3D(-2.5, 3, 2));
  m_sphere.setUniformValue("model", model2);
  m_sphere.setUniformValue("view_pos", view * QVector3D(0, 0, 0));

  // glActiveTexture(GL_TEXTURE0);
  // glBindTexture(GL_TEXTURE_2D, back_fbo->texture());
  // m_sphere.setUniformValue("back_FragColor", 0);

  // 绑定立方体纹理（请根据您的情况修改）
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);
  m_sphere.setUniformValue("envMap", 0);

  // 将相机位置传递给球体片段着色器
  m_sphere.setUniformValue("CameraPos", cameraPos);
  m_sphere.setUniformValue("LightPos", QVector3D(5.0f, 5.0f, 5.0f));
  m_sphere.setUniformValue("LightColor", QVector3D(1.0f, 1.0f, 1.0f));

  // glEnable(GL_BLEND);
  // 0glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  m_sphere.Draw();

  glBindVertexArray(0);
  // Release FBO
  m_fbo->release();
  */
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glBindVertexArray(0);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
}

void GLFWRenderer::mvp() {}

void GLFWRenderer::onTrianglePosChanged() {
  clearWindow();

  float newvertices[] = {0.6f,  0.5f,  0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f,
                         0.6f,  -0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f,
                         -0.6f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                         -0.6f, 0.5f,  0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f};

  glGenVertexArrays(1, &m_vao);
  glGenBuffers(1, &m_vbo);

  glBindVertexArray(m_vao);
  glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(newvertices), newvertices,
               GL_STATIC_DRAW);

  // position attribute
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  // texture coord attribute
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
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
      // m_view.setY(m_view.y() + 0.1f);
      cameraPos += cameraSpeed * cameraFront;
      break;
    case GLFWItem::CLICK_TYPE::DOWN_DOWN:
      cameraPos -= cameraSpeed * cameraFront;
      // m_view.setY(m_view.y() - 0.1f);
      break;
    case GLFWItem::CLICK_TYPE::DOWN_LEFT:
      cameraPos -= QVector3D::crossProduct(cameraFront, cameraUp).normalized() *
                   cameraSpeed;
      // m_view.setX(m_view.x() - 0.1f);
      break;
    case GLFWItem::CLICK_TYPE::DOWN_RIGHT:
      cameraPos += QVector3D::crossProduct(cameraFront, cameraUp).normalized() *
                   cameraSpeed;
      // m_view.setX(m_view.x() + 0.1f);
      break;
    default:
      break;
  }
}