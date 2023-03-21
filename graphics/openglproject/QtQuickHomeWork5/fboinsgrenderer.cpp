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
    }

    if (!m_vao) {
      glGenVertexArrays(1, &m_vao);
      glGenBuffers(1, &m_vbo);
      
      glBindVertexArray(m_vao);
      glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
      glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

          // position attribute
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                            (void*)0);
      glEnableVertexAttribArray(0);
      // texture coord attribute
      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                            (void*)(3 * sizeof(float)));
      glEnableVertexAttribArray(1);
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
    {
        back_fbo->bind();

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                GL_STENCIL_BUFFER_BIT);

        glUseProgram(m_program);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glEnable(GL_MULTISAMPLE);

        QMatrix4x4 view{};
        QMatrix4x4 projection{};

        view.lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        view.rotate(qRadiansToDegrees(20.0f), 1, 1, 0);
        // view.translate(m_view);
        projection.perspective(qRadiansToDegrees(45.0f),
                               (float)800 / (float)800, 0.1f, 100.0f);

        unsigned int viewLoc = glGetUniformLocation(m_program, "view");
        unsigned int projectionLoc =
            glGetUniformLocation(m_program, "projection");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, view.data());
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, projection.data());

        glBindVertexArray(m_vao);

        QMatrix4x4 model;
        model.translate(QVector3D(0, 0, 0));
        // model.rotate(qRadiansToDegrees(20.0f), 1, 1, 0);
        unsigned int modelLoc = glGetUniformLocation(m_program, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model.data());

        glDrawArrays(GL_TRIANGLES, 0, 36);
        back_fbo->release();
    }

  m_fbo->bind();

  // Render to FBO

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
  // view.translate(m_view);
  projection.perspective(qRadiansToDegrees(45.0f), (float)800 / (float)800,
                         0.1f, 100.0f);

  unsigned int viewLoc = glGetUniformLocation(m_program, "view");
  unsigned int projectionLoc = glGetUniformLocation(m_program, "projection");
  glUniformMatrix4fv(viewLoc, 1, GL_FALSE, view.data());
  glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, projection.data());

  glBindVertexArray(m_vao);

  QMatrix4x4 model;
  model.translate(QVector3D(0, 0, 0));
  // model.rotate(qRadiansToDegrees(20.0f), 1, 1, 0);
  unsigned int modelLoc = glGetUniformLocation(m_program, "model");
  glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model.data());

  glDrawArrays(GL_TRIANGLES, 0, 36);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, 0);
  m_sphere.bind();
  m_sphere.setUniformValue("view", view);
  m_sphere.setUniformValue("projection", projection);
  QMatrix4x4 model2;
  model2.scale(0.3f);
  model2.translate(QVector3D(-2.5,3, 2));
  m_sphere.setUniformValue("model", model2);
  m_sphere.setUniformValue("view_pos", view * QVector3D(0, 0, 0));

  GLuint back_tex = back_fbo->texture();
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, back_tex);
  m_sphere.setUniformValue("back_FragColor", 0);
  //glEnable(GL_BLEND);
  //0glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  m_sphere.Draw();

  glBindVertexArray(0);
  // Release FBO
  m_fbo->release();
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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          (void*)0);
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
