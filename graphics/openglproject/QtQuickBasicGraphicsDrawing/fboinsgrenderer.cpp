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
  return render_;
}

void GLFWItem::changeTrianglePos() {
  emit trianglePosChanged();
}

// Create VAO and VBO
float vertices[] = {
    //     ---- 位置 ----       ---- 颜色 ----     - 纹理坐标 -
    0.5f,  0.5f,  0.0f,        1.0f, 0.0f, 0.0f,    1.0f, 1.0f,  // 右上
    0.5f,  -0.5f, 0.0f,        0.0f, 1.0f, 0.0f,    1.0f, 0.0f,  // 右下
    -0.5f, -0.5f, 0.0f,        0.0f, 0.0f, 1.0f,    0.0f, 0.0f,  // 左下
    -0.5f, 0.5f,  0.0f,        1.0f, 1.0f, 0.0f,    0.0f, 1.0f   // 左上
};
unsigned int indices[] = {
    0, 1, 3,  // first triangle
    1, 2, 3   // second triangle
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
      unsigned int EBO;
      glGenVertexArrays(1, &m_vao);
      glGenBuffers(1, &m_vbo);
      glGenBuffers(1, &EBO);
      
      glBindVertexArray(m_vao);
      glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
      glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
                   GL_STATIC_DRAW);

      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                            (void*)0);
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                            (void*)(3 * sizeof(float)));
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                            (void*)(6 * sizeof(float)));
      glEnableVertexAttribArray(2);

      // load and create a texture
      // texture 1
      glGenTextures(1, &texture2);
      glBindTexture(GL_TEXTURE_2D, texture2);
      // set the texture wrapping parameters
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      int width, height, nrChannels;
      //stbi_set_flip_vertically_on_load(true);
      unsigned char* data =
          stbi_load("./awesomeface.png", &width, &height, &nrChannels, 0);
      if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
      } else {
        std::cout << "Failed to load texture" << std::endl;
      }
      stbi_image_free(data);

            // texture 2
      glGenTextures(1, &texture1);
      glBindTexture(GL_TEXTURE_2D, texture1);
      // set the texture wrapping parameters
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      unsigned char* data2 =
          stbi_load("./wall.jpg", &width, &height, &nrChannels, 0);
      if (data2) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                     GL_UNSIGNED_BYTE, data2);
        glGenerateMipmap(GL_TEXTURE_2D);
      } else {
        std::cout << "Failed to load texture" << std::endl;
      }
      stbi_image_free(data2);

      glBindBuffer(GL_ARRAY_BUFFER, 0);
      glBindVertexArray(0);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

      glUseProgram(m_program);
      glUniform1i(glGetUniformLocation(m_program, "texture1"), 0);
      glUniform1i(glGetUniformLocation(m_program, "texture2"), 1);
    }
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

    m_fbo->bind();

    // Render to FBO
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_MULTISAMPLE);

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture2);

    glUseProgram(m_program);

    QMatrix4x4 model{};
    QMatrix4x4 view{};
    QMatrix4x4 projection{};
    model.rotate(qRadiansToDegrees(-30.0f), 1.0f, 0.0f, 0.0f);
    view.translate(0.0f, 0.0f, -3.0f);
    projection.perspective(qRadiansToDegrees(45.0f), (float)800 / (float)600,
                           0.1f, 100.0f);

    
    unsigned int modelLoc = glGetUniformLocation(m_program, "model");
    unsigned int viewLoc = glGetUniformLocation(m_program, "view");
    unsigned int projectionLoc = glGetUniformLocation(m_program, "projection");
    // pass them to the shaders (3 different ways)
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model.data());
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, view.data());
    // note: currently we set the projection matrix each frame, but since the
    // projection matrix rarely changes it's often best practice to set it
    // outside the main loop only once.
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, projection.data());


    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

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

    unsigned int EBO;
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &EBO);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(newvertices), newvertices,
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
                 GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

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