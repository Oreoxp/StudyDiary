#include "fboinsgrenderer.h"

#include <QtGui/QOpenGLFramebufferObject>

#include <qsgsimpletexturenode.h>
#include <QTransform>
#include <QtQuick/QQuickWindow>
#include <QtMath>

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



float bottomVertices[] = {
  // positions           // normals
  -1.0f, -1.0f, 0.0f,    0.0f, 0.0f, -1.0f,
   1.0f,  1.0f, 0.0f,    0.0f, 0.0f, -1.0f,
   1.0f, -1.0f, 0.0f,    0.0f, 0.0f, -1.0f,

  -1.0f, -1.0f, 0.0f,    0.0f, 0.0f, -1.0f,
   1.0f,  1.0f, 0.0f,    0.0f, 0.0f, -1.0f,
  -1.0f,  1.0f, 0.0f,    0.0f, 0.0f, -1.0f
};


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
      camera(QVector3D(0.0f, 0.0f, 3.0f)),
      m_sphere_light("./resource/light_sphere.vs", "./resource/light_sphere.fs"),
      m_sphere_bottom("./resource/bottom.vs", "./resource/bottom.fs", "../../resouce/sphere/square.obj"),
      m_sphere_torusknot("./resource/sphereLast.vs", "./resource/sphereLast.fs", "../../resouce/sphere/rifle.obj") {
  initializeOpenGLFunctions();
  m_main_shader = new QOpenGLShaderProgram();
  m_shader = new QOpenGLShaderProgram();
  m_bottom_shader = new QOpenGLShaderProgram();
  m_shaderSolid = new QOpenGLShaderProgram();
  m_skybox_shader = new QOpenGLShaderProgram();

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

    m_bottom_shader->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex,
      "./resource/bottom.vs");
    m_bottom_shader->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment,
      "./resource/bottom.fs");
    m_bottom_shader->link();
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

    unsigned int bottom_vbo;
    glGenVertexArrays(1, &m_bottom_vao);
    glGenBuffers(1, &bottom_vbo);

    glBindVertexArray(m_bottom_vao);
    glBindBuffer(GL_ARRAY_BUFFER, bottom_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(bottomVertices), bottomVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
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

void GLFWRenderer::renderBackgroundFbo() {
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  back_fbo->bind();
  glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  QMatrix4x4 view{};
  QMatrix4x4 projection{};

  view = camera.GetViewMatrix();
  projection.perspective(camera.Zoom, (float)1000 / (float)1000, 0.1f, 100.0f);

  GLenum error = glGetError();
  if (error != GL_NO_ERROR)
    qDebug() << "OpenGL error:" << error;

  glDepthFunc(GL_LEQUAL);
  glBindVertexArray(m_skyboxVAO);
  m_skybox_shader->bind();
  m_skybox_shader->setUniformValue("view", view);
  m_skybox_shader->setUniformValue("projection", projection);
  m_skybox_shader->setUniformValue("cameraPos", camera.Position);
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


void GLFWRenderer::render() {
  obj.clear();
  obj.reserve(3);
  float currentFrame = timer.elapsed();
  deltaTime = (currentFrame - lastFrame) / 100.0;
  lastFrame = currentFrame;

  QMatrix4x4 view{};
  QMatrix4x4 projection{};
  QMatrix4x4 model{};
  view = camera.GetViewMatrix();
  projection.perspective(camera.Zoom, (float)1000 / (float)1000, 0.1f, 100.0f);

  m_fbo->bind();
  glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);

  QMatrix4x4 model_bottom{};
  model_bottom.scale(4.0f);
  model_bottom.rotate(60.0f,
    { 1.0,0.0,0 });
  model_bottom.translate(0, -0.8, -0.5);

  //light
  m_sphere_light.bind();
  glDepthFunc(GL_LESS);
  QMatrix4x4 model_light{};
  // Compute the center of model2
  QVector3D model2_center = model_bottom * QVector3D(0, 0, 0);
  // Set the initial position of the model_light
  model_light.translate(model2_center);
  model_light.translate(0, 0, 0); // Move the light away from the model2_center along the model2 normal
  model_light.translate(-model2_center);
  // Rotate the model_light around the model2's center normal
  model_light.translate(model2_center);
  model_light.rotate(30.0f * timer.elapsed() / 200, QVector3D(0.0, -1.0, 0.5)); // Rotate around model2's center normal
  model_light.translate(-model2_center);
  model_light.scale(0.1f);
  m_sphere_light.setUniformValue("view", view);
  m_sphere_light.setUniformValue("projection", projection);
  m_sphere_light.setUniformValue("model", model_light);
  m_sphere_light.Draw(m_fbo, QMatrix4x4(), QMatrix4x4());

  //sphere 1
  m_sphere.bind();
  glDepthFunc(GL_LESS);
  glBindVertexArray(m_vao);
  QMatrix4x4 model3{};
  model3.translate(model2_center);
  model3.scale(0.2f);
  model3.translate(3, -1.0, 0);
  m_sphere.setUniformValue("view", view);
  m_sphere.setUniformValue("projection", projection);
  m_sphere.setUniformValue("model", model3);
  OtherObject obj1;
  m_sphere.getVertexDataTexture(obj1);
  obj1.pos = model3 * QVector3D{ 0., 0., 0. };
  obj1.r = 2;
  obj.push_back(obj1);
  m_sphere.setUniformValue("depth_map", obj1.depth_id);
  glActiveTexture(GL_TEXTURE0 + obj1.depth_id);
  glBindTexture(GL_TEXTURE_2D, obj1.depth_id);
  m_sphere.Draw(m_fbo, model3, model_light, true);
  glBindVertexArray(0);

  //sphere 2
  m_sphere2.bind();
  glDepthFunc(GL_LESS);
  QMatrix4x4 model4{};
  model4.translate(model2_center);
  model4.scale(0.2f);
  model4.translate(-3.0, -1.0, 0);
  m_sphere2.setUniformValue("view", view);
  m_sphere2.setUniformValue("projection", projection);
  m_sphere2.setUniformValue("model", model4);
  m_sphere2.setUniformValue("viewPos", view * QVector3D{ 0., 0., 0. });
  OtherObject obj2;
  m_sphere2.getVertexDataTexture(obj2);
  obj2.pos = model3 * QVector3D{ 0., 0., 0. };
  obj2.r = 2;
  obj.push_back(obj2);
  m_sphere2.setUniformValue("depth_map", obj2.depth_id);
  glActiveTexture(GL_TEXTURE0 + obj2.depth_id);
  glBindTexture(GL_TEXTURE_2D, obj2.depth_id);
  m_sphere2.Draw(m_fbo, model4, model_light, true);

  //torusknot  
  m_sphere_torusknot.bind();
  glDepthFunc(GL_LESS);
  //glBindVertexArray(m_vao);
  QMatrix4x4 model_torusknot{};
  model_torusknot.translate(model2_center);
  model_torusknot.translate(0.0, -0.3, 0.0);
  model_torusknot.scale(0.01f);
  m_sphere_torusknot.setUniformValue("view", view);
  m_sphere_torusknot.setUniformValue("projection", projection);
  m_sphere_torusknot.setUniformValue("model", model_torusknot);
  m_sphere_torusknot.setUniformValue("viewPos", view * QVector3D{ 0., 0., 0. });
  OtherObject obj3;
  m_sphere_torusknot.getVertexDataTexture(obj3);
  obj3.pos = model_torusknot * QVector3D{ 0., 0., 0. };
  obj3.r = 2;
  obj.push_back(obj3);
  m_sphere.setUniformValue("depth_map", obj3.depth_id);
  glActiveTexture(GL_TEXTURE0 + obj3.depth_id);
  glBindTexture(GL_TEXTURE_2D, obj3.depth_id);
  m_sphere_torusknot.Draw(m_fbo, model_torusknot, model_light, true);

  //bottom
  m_sphere_bottom.bind();
  m_sphere_bottom.setUniformValue("view", view);
  m_sphere_bottom.setUniformValue("projection", projection);
  m_sphere_bottom.setUniformValue("model", model_bottom);
  m_sphere_bottom.setUniformValue("viewPos", view * QVector3D{ 0., 0., 0. });

  m_sphere_bottom.setUniformValue("otherObejcts[0].position", model3 *QVector3D{ 0., 0., 0. });
  m_sphere_bottom.setUniformValue("otherObejcts[0].radius", 2);
  m_sphere_bottom.setUniformValue("otherObejcts[0].vertex_data_texture", obj[0].vertices_id);
  m_sphere_bottom.setUniformValue("otherObejcts[0].normal_data_texture", obj[0].normals_id);
  m_sphere_bottom.setUniformValue("otherObejcts[0].num_triangles", obj[0].num_triangles);
  m_sphere_bottom.setUniformValue("otherObejcts[0].model", model3);
  glActiveTexture(GL_TEXTURE0 + obj[0].vertices_id);
  glBindTexture(GL_TEXTURE_2D, obj[0].vertices_id);
  glActiveTexture(GL_TEXTURE0 + obj[0].normals_id);
  glBindTexture(GL_TEXTURE_2D, obj[0].normals_id);

  m_sphere_bottom.setUniformValue("otherObejcts[1].position", model4* QVector3D{ 0., 0., 0. });
  m_sphere_bottom.setUniformValue("otherObejcts[1].radius", 2);
  m_sphere_bottom.setUniformValue("otherObejcts[1].vertex_data_texture", obj[1].vertices_id);
  m_sphere_bottom.setUniformValue("otherObejcts[1].normal_data_texture", obj[1].normals_id);
  m_sphere_bottom.setUniformValue("otherObejcts[1].num_triangles", obj[1].num_triangles);
  m_sphere_bottom.setUniformValue("otherObejcts[1].model", model4);
  glActiveTexture(GL_TEXTURE0 + obj[1].vertices_id);
  glBindTexture(GL_TEXTURE_2D, obj[1].vertices_id);
  glActiveTexture(GL_TEXTURE0 + obj[1].normals_id);
  glBindTexture(GL_TEXTURE_2D, obj[1].normals_id);

  m_sphere_bottom.setUniformValue("otherObejcts[2].position", model_torusknot* QVector3D{ 0., 0., 0. });
  m_sphere_bottom.setUniformValue("otherObejcts[2].radius", 2);
  m_sphere_bottom.setUniformValue("otherObejcts[2].vertex_data_texture", obj[2].vertices_id);
  m_sphere_bottom.setUniformValue("otherObejcts[2].normal_data_texture", obj[2].normals_id);
  m_sphere_bottom.setUniformValue("otherObejcts[2].num_triangles", obj[2].num_triangles);
  m_sphere_bottom.setUniformValue("otherObejcts[2].model", model_torusknot);
  glActiveTexture(GL_TEXTURE0 + obj[2].vertices_id);
  glBindTexture(GL_TEXTURE_2D, obj[2].vertices_id);
  glActiveTexture(GL_TEXTURE0 + obj[2].normals_id);
  glBindTexture(GL_TEXTURE_2D, obj[2].normals_id);

  m_sphere_bottom.Draw(m_fbo, model_bottom, model_light, true);
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
  float xpos = static_cast<float>(x);
  float ypos = static_cast<float>(y);
  if (firstMouse) {
    lastX = xpos;
    lastY = ypos;
    firstMouse = false;
  }

  float xoffset = xpos - lastX;
  float yoffset =
      lastY - ypos;  // reversed since y-coordinates go from bottom to top

  lastX = xpos;
  lastY = ypos;

  camera.ProcessMouseMovement(xoffset, yoffset);
}