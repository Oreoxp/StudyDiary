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


// Create VAO and VBO
float vertices[] = {
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
        QOpenGLShader::ShaderTypeBit::Vertex, "./11.vs");
    m_light_shader.addCacheableShaderFromSourceFile(
        QOpenGLShader::ShaderTypeBit::Fragment, "./11.fs");

    m_light_cube_shader.addCacheableShaderFromSourceFile(
            QOpenGLShader::ShaderTypeBit::Vertex, "./12.vs");
    m_light_cube_shader.addCacheableShaderFromSourceFile(
        QOpenGLShader::ShaderTypeBit::Fragment, "./12.fs");

    if (!m_light_shader.link())
      qWarning() << m_light_shader.log();

    if (!m_light_cube_shader.link())
      qWarning() << m_light_cube_shader.log();


    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindVertexArray(m_vao);

    
    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void*)0);
    glEnableVertexAttribArray(0);
    // normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);


    // second, configure the light's VAO (VBO stays the same; the vertices are
    // the same for the light object which is also a 3D cube)
    glGenVertexArrays(1, &m_light_cube_vao);
    glBindVertexArray(m_light_cube_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
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
    QMatrix4x4 model1 = QMatrix4x4();
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

    QMatrix4x4 model2 = QMatrix4x4();
    model2.scale(0.2f);
    model2.rotate(qRadiansToDegrees(10.0f * timer.elapsed() / 12000), 0, 0, 1);
    model2.translate({0,0,3});
    model2.translate(lightPos);

    m_light_cube_shader.bind();
    m_light_cube_shader.setUniformValue("view", view);
    m_light_cube_shader.setUniformValue("projection", projection);
    m_light_cube_shader.setUniformValue("model", model2);
    glBindVertexArray(m_light_cube_vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    m_light_cube_shader.release();


    m_light_shader.bind();
    m_fbo->bind();
    m_light_shader.setUniformValue("objectColor", QVector3D{1.0f, 0.5f, 0.31f});
    m_light_shader.setUniformValue("lightColor", QVector3D{1.0f, 1.0f, 1.0f});
    m_light_shader.setUniformValue("view", view);
    m_light_shader.setUniformValue("projection", projection);
    m_light_shader.setUniformValue("model", model1);
    m_light_shader.setUniformValue(
        "lightPos",
        model2 * QVector3D{0., 0., 0.} - model1 * QVector3D{0., 0., 0.});
    m_light_shader.setUniformValue("viewPos", view * QVector3D{0., 0., 0.});
    // render the cube
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);


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
