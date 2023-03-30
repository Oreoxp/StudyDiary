#include "fboinsgrenderer.h"

#include <QtOpenGL/QOpenGLFramebufferObject>

#include <qsgsimpletexturenode.h>
#include <QTransform>
#include <QtQuick/QQuickWindow>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "stb_image.h"

GLFWItem::GLFWItem() {}

GLFWItem::~GLFWItem() {}

GLFWRenderer* render_;

QQuickFramebufferObject::Renderer* GLFWItem::createRenderer() const {
  render_ = new GLFWRenderer();
  connect(this, SIGNAL(trianglePosChanged()), render_,
          SLOT(onTrianglePosChanged()), Qt::QueuedConnection);
  connect(this, SIGNAL(keyDownChanged(GLFWItem::CLICK_TYPE)), render_,
          SLOT(onKeyDownChanged(GLFWItem::CLICK_TYPE)), Qt::QueuedConnection);
  connect(this, SIGNAL(mouseChangeChangedd(qreal, qreal)), render_,
          SLOT(onMouseChangeChanged(qreal, qreal)), Qt::QueuedConnection);
  return render_;
}

void GLFWItem::changeTrianglePos() {
  emit trianglePosChanged();
}

void GLFWItem::changeKeyDown(GLFWItem::CLICK_TYPE type) {
  emit keyDownChanged(type);
}

void GLFWItem::mouseChange(qreal x, qreal y) {
  emit mouseChangeChangedd(x, y);
}

// Create VAO and VBO
float vertices[] = {
    // positions          // normals
    -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f, 0.5f,  -0.5f, -0.5f,
    0.0f,  0.0f,  -1.0f, 0.5f,  0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f,
    0.5f,  0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f, -0.5f, 0.5f,  -0.5f,
    0.0f,  0.0f,  -1.0f, -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f,

    -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,  0.5f,  -0.5f, 0.5f,
    0.0f,  0.0f,  1.0f,  0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
    0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  -0.5f, 0.5f,  0.5f,
    0.0f,  0.0f,  1.0f,  -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,

    -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f,  -0.5f, 0.5f,  -0.5f,
    -1.0f, 0.0f,  0.0f,  -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f,  -0.5f, -0.5f, 0.5f,
    -1.0f, 0.0f,  0.0f,  -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f,

    0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.5f,  0.5f,  -0.5f,
    1.0f,  0.0f,  0.0f,  0.5f,  -0.5f, -0.5f, 1.0f,  0.0f,  0.0f,
    0.5f,  -0.5f, -0.5f, 1.0f,  0.0f,  0.0f,  0.5f,  -0.5f, 0.5f,
    1.0f,  0.0f,  0.0f,  0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

    -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,  0.5f,  -0.5f, -0.5f,
    0.0f,  -1.0f, 0.0f,  0.5f,  -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,
    0.5f,  -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,  -0.5f, -0.5f, 0.5f,
    0.0f,  -1.0f, 0.0f,  -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,

    -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,  0.5f,  0.5f,  -0.5f,
    0.0f,  1.0f,  0.0f,  0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  -0.5f, 0.5f,  0.5f,
    0.0f,  1.0f,  0.0f,  -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f};


   float quadVertices[] = {  // vertex attributes for a quad that fills the entire
                          // screen in Normalized Device Coordinates.
    // positions   // texCoords
    -1.0f, 1.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f,

    -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  1.0f, 1.0f};

float skyboxVertices[] = {
    // positions
    -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f,
    1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,

    -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f,
    -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,

    1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,
    1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,

    -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
    1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,

    -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,
    1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,

    -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f,
    1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f};

unsigned int loadCubemap(std::vector<std::string> faces) {
  unsigned int textureID;
  glGenTextures(1, &textureID);
  glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

  int width, height, nrChannels;
  for (unsigned int i = 0; i < faces.size(); i++) {
    unsigned char* data =
        stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
    if (data) {
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height,
                   0, GL_RGB, GL_UNSIGNED_BYTE, data);
      stbi_image_free(data);
    } else {
      std::cout << "Cubemap texture failed to load at path: " << faces[i]
                << std::endl;
      stbi_image_free(data);
    }
  }
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  return textureID;
}

GLFWRenderer::GLFWRenderer()
    : m_fbo(nullptr),
      m_vao(0),
      m_vbo(0),
      m_program(0),
      camera(QVector3D(0.0, 0.0, 0.8f)) {
  initializeOpenGLFunctions();
  m_main_shader = new QOpenGLShaderProgram();
  m_shader = new QOpenGLShaderProgram();
  m_shader2 = new QOpenGLShaderProgram();
  m_skybox_shader = new QOpenGLShaderProgram();
  m_cube_map = new QOpenGLTexture(QOpenGLTexture::TargetCubeMap);
  m_cube_map->create();
  m_cube_map->setSize(1000, 1000);
  m_cube_map->setFormat(QOpenGLTexture::RGBA8_UNorm);
  m_cube_map->allocateStorage();

  for (int i = 0; i < 6; ++i) {
    m_fbo_cube[i] = new QOpenGLFramebufferObject(1000, 1000);
    m_fbo_cube[i]->addColorAttachment(1000, 1000);
    m_fbo_cube[i]->bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_cube_map->textureId(), 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    qDebug() << "Framebuffer is not complete!";
    }
    m_fbo_cube[i]->release();
  }

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

    m_main_shader->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex,
                                                   "./11.vs");
    m_main_shader->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment,
                                               "./11.fs");
    m_main_shader->link();

    m_shader->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex,
                                               "./sphere.vs");
    m_shader->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment,
                                               "./sphere.fs");
    m_shader->link();

    m_shader2->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex,
                                               "./sphereLast.vs");
    m_shader2->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment,
                                               "./sphereLast.fs");
    m_shader2->link();

    m_skybox_shader->addCacheableShaderFromSourceFile(
        QOpenGLShader::Vertex, "./skybox.vs");
    m_skybox_shader->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment,
                                               "./skybox.fs");
    m_skybox_shader->link();
  }

  if (!m_vao) {
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    glGenVertexArrays(1, &m_vao2);
    glGenBuffers(1, &m_vbo2);

    glBindVertexArray(m_vao2);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo2);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices,
                 GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void*)(2 * sizeof(float)));

    unsigned int quadVBO;
    glGenVertexArrays(1, &m_vao_quad);
    glGenBuffers(1, &quadVBO);

    glBindVertexArray(m_vao_quad);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices,
                 GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void*)(2 * sizeof(float)));

    // skybox VAO
    unsigned int skyboxVBO;
    glGenVertexArrays(1, &m_skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(m_skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices,
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                          (void*)0);

    
    std::vector<std::string> faces{("./resource/right.jpg"),
                                   ("./resource/left.jpg"),
                                   ("./resource/top.jpg"),
                                   ("./resource/bottom.jpg"),
                                   ("./resource/front.jpg"),
                                   ("./resource/back.jpg")};
    cubemapTexture = loadCubemap(faces);
  }
  timer.start();
  lastFrame = timer.elapsed();
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
  QOpenGLFramebufferObjectFormat fboFormat;
  fboFormat.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
  fboFormat.setTextureTarget(GL_TEXTURE_2D);
  fboFormat.setInternalTextureFormat(GL_RGBA8);
  back_fbo = new QOpenGLFramebufferObject(size, fboFormat);
  return m_fbo;
}


void GLFWRenderer::render() {
  float currentFrame = timer.elapsed();
  deltaTime = (currentFrame - lastFrame) / 100.0;
  lastFrame = currentFrame;
  {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    back_fbo->bind();
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    QMatrix4x4 view{};
    QMatrix4x4 projection{};
    QMatrix4x4 model{};

    view = camera.GetViewMatrix();
    projection.perspective(camera.Zoom, (float)1000 / (float)1000, 0.1f,
                           100.0f);
    
    QMatrix4x4 model2{};
    /*glBindVertexArray(m_vao);
    model2.scale(0.2f);
    // model2.rotate(30, 0, 1, 0);
    // model2.translate(0.5, -1, 0);
     m_shader->bind();
    m_shader->setUniformValue("view", view);
    m_shader->setUniformValue("projection", projection);
    m_shader->setUniformValue("model", model2);
    m_shader->setUniformValue("cameraPos", camera.Position);
    m_shader->setUniformValue("skybox", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    m_shader->release();
    glBindVertexArray(0);*/


    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
      qDebug() << "OpenGL error:" << error;

    glDepthFunc(GL_LEQUAL);
    m_skybox_shader->bind();
    glBindVertexArray(m_skyboxVAO);

    projection.perspective(camera.Zoom, (float)1000 / (float)1000, 0.1f,
                           100.0f);
    model.scale(2.0);
    // model.rotate(-90.0f, 0, 0, 1);
    m_skybox_shader->setUniformValue("view", view);
    m_skybox_shader->setUniformValue("projection", projection);
    m_skybox_shader->setUniformValue("model", model);
    m_skybox_shader->setUniformValue("skybox", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    m_skybox_shader->release();
    glBindVertexArray(0);
    glDepthFunc(GL_LESS);
    glViewport(0, 0, 1000, 1000);

    back_fbo->release();
  }

  
  QMatrix4x4 view{};
  QMatrix4x4 projection{};
  QMatrix4x4 model{};
  view = camera.GetViewMatrix();
  projection.perspective(camera.Zoom, (float)1000 / (float)1000, 0.1f, 100.0f);

  view.QMatrix4x4::lookAt(QVector3D(0, 0, 0), QVector3D(1, 0, 0),
                          QVector3D(0, -1, 0));
  auto view1 = view;
  view.QMatrix4x4::lookAt(QVector3D(0, 0, 0), QVector3D(-1, 0, 0),
                          QVector3D(0, -1, 0));
  auto view2 = view;
  view.QMatrix4x4::lookAt(QVector3D(0, 0, 0), QVector3D(0, 1, 0),
                          QVector3D(0, 0, 1));
  auto view3 = view;
  view.QMatrix4x4::lookAt(QVector3D(0, 0, 0), QVector3D(0, -1, 0),
                          QVector3D(0, 0, -1));
  auto view4 = view;
  view.QMatrix4x4::lookAt(QVector3D(0, 0, 0), QVector3D(0, 0, 1),
                          QVector3D(0, -1, 0));
  auto view5 = view;
  view.QMatrix4x4::lookAt(QVector3D(0, 0, 0), QVector3D(0, 0, -1),
                          QVector3D(0, -1, 0));
  auto view6 = view;

  QMatrix4x4 cube_views[6] = {view1, view2, view3, view4, view5, view6};

  for (int i = 0; i < 6; ++i) {
    m_fbo_cube[i]->bind();
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    // Bind and set up the shader for rendering the scene (e.g., m_main_shader).
    m_main_shader->bind();
    glDepthFunc(GL_LESS);
    glBindVertexArray(m_vao2);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, back_fbo->texture());
    m_main_shader->setUniformValue("screenTexture", 0);
    m_main_shader->setUniformValue("view", cube_views[i]);
    m_main_shader->setUniformValue("projection", projection);

    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    m_main_shader->release();
    m_fbo_cube[i]->release();
  }


  m_fbo->bind();
  glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);

  m_main_shader->bind();
  glDepthFunc(GL_LESS);
  glBindVertexArray(m_vao2);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, back_fbo->texture());
  m_main_shader->setUniformValue("screenTexture", 0);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glBindVertexArray(0);
  m_main_shader->release();

  /* m_shader2->bind();
  glDepthFunc(GL_LESS);
  glBindVertexArray(m_vao);
  QMatrix4x4 model3{};
  model3.scale(0.2f);
  model3.rotate(30, 0, 1, 0);
  model3.translate(-0.7, 0, 0);
  m_shader2->setUniformValue("view", view);
  m_shader2->setUniformValue("projection", projection);
  m_shader2->setUniformValue("model", model3);
  m_shader2->setUniformValue("cameraPos", camera.Position);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_CUBE_MAP, m_cube_map->textureId());
  m_shader2->setUniformValue("environmentTexture", 0);
  glDrawArrays(GL_TRIANGLES, 0, 36);
  m_shader2->release();*/

  glBindVertexArray(0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  m_fbo->release();
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
      camera.ProcessKeyboard(FORWARD, deltaTime);
      break;
    case GLFWItem::CLICK_TYPE::DOWN_DOWN:
      camera.ProcessKeyboard(BACKWARD, deltaTime);
      // m_view.setY(m_view.y() - 0.1f);
      break;
    case GLFWItem::CLICK_TYPE::DOWN_LEFT:
      camera.ProcessKeyboard(LEFT, deltaTime);
      // m_view.setX(m_view.x() - 0.1f);
      break;
    case GLFWItem::CLICK_TYPE::DOWN_RIGHT:
      camera.ProcessKeyboard(RIGHT, deltaTime);
      // m_view.setX(m_view.x() + 0.1f);
      break;
    default:
      break;
  }
}


void GLFWRenderer::onMouseChangeChanged(qreal x, qreal y) {
  camera.ProcessMouseMovement(x, y);
}