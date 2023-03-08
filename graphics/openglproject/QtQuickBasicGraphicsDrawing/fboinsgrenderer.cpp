#include "fboinsgrenderer.h"
#include "logorenderer.h"

#include <QtGui/QOpenGLFramebufferObject>

#include <QtQuick/QQuickWindow>
#include <qsgsimpletexturenode.h>

#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

class LogoInFboRenderer : public QQuickFramebufferObject::Renderer
{
public:
    LogoInFboRenderer()
    {
        logo.initialize();
    }

    void render() {
        logo.render();
        update();
    }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) {
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        format.setSamples(4);
        return new QOpenGLFramebufferObject(size, format);
    }

    LogoRenderer logo;
};

QQuickFramebufferObject::Renderer *FboInSGRenderer::createRenderer() const
{
    return new LogoInFboRenderer();
}



GLFWItem::GLFWItem() {}

GLFWItem::~GLFWItem() {}

QQuickFramebufferObject::Renderer* GLFWItem::createRenderer() const {
    return new GLFWRenderer;
}



GLFWRenderer::GLFWRenderer()
    : m_fbo(nullptr),
      m_vao(0),
      m_vbo(0), m_program(0) {
    initializeOpenGLFunctions();
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
        // Create shader program
        const char* vertexShaderSource =
            "#version 330 core\n"
            "layout (location = 0) in vec3 aPos;\n"// 位置变量的属性位置值为 0
            "layout (location = 1) in vec3 aColor;\n"// 颜色变量的属性位置值为 1
            "layout(location = 2) in vec2 aTexCoord; \n"// 纹理变量的属性位置值为 1
            "out vec4 vertexColor;\n"
            "out vec4 ourPosition;\n"
            "out vec2 TexCoord;\n" 
            "uniform vec2 dev = vec2(0.5, 0);\n"
            "void main()\n"
            "{\n"
            "   vertexColor = vec4(aColor, 1.0);\n"
            "   gl_Position = vec4(aPos, 1.0);\n"
            "   ourPosition = gl_Position;\n"
            "   TexCoord = vec2(aTexCoord.x, aTexCoord.y); \n" 
            "}\0";
        const char* fragmentShaderSource =
            "#version 330 core\n"
            "out vec4 FragColor;\n"
            "in vec4 vertexColor;\n"
            "in vec4 ourPosition;\n"
            "in vec2 TexCoord;\n"
            "void main()\n"
            "{\n"
            "   FragColor = vertexColor;\n"
            "}\n\0";

        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
        glCompileShader(vertexShader);

        int success;
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);

        glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
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
        // Create VAO and VBO
        float vertices[] = {
            //     ---- 位置 ----       ---- 颜色 ----     - 纹理坐标 -
            0.5f,  0.5f,  0.0f,        1.0f, 0.0f, 0.0f,     1.0f, 1.0f,  // 右上
            0.5f,  -0.5f, 0.0f,        0.0f, 1.0f, 0.0f,     1.0f, 0.0f,  // 右下
           -0.5f, -0.5f, 0.0f,         0.0f, 0.0f, 1.0f,     0.0f, 0.0f,  // 左下
           -0.5f, 0.5f,  0.0f,         1.0f, 1.0f, 0.0f,     0.0f, 1.0f   // 左上
        };
        unsigned int indices[] = {
            0, 1, 3,  // first triangle
            1, 2, 3   // second triangle
        };
        unsigned int  EBO;
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
        glGenBuffers(1, &EBO);


        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices,
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
    }

    
    // Render to FBO
    m_fbo->bind();
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_MULTISAMPLE);

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(m_program);
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // Release FBO
    m_fbo->release();

    // Blit FBO to default framebuffer
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo->handle());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, m_fbo->width(), m_fbo->height(), 0, 0,
                      m_fbo->width(), m_fbo->height(), GL_COLOR_BUFFER_BIT,
                      GL_NEAREST);
}