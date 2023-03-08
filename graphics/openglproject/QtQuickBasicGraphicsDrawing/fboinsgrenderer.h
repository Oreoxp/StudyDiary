#ifndef FBOINSGRENDERER_H
#define FBOINSGRENDERER_H

#include <GLFW/glfw3.h>
#include <QOpenGLFunctions_3_0>
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOpenGLTexture>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QQuickFramebufferObject>
#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLVertexArrayObject>

class LogoRenderer;

class FboInSGRenderer : public QQuickFramebufferObject
{
    Q_OBJECT
public:
    Renderer *createRenderer() const;
};

class GLFWItem : public QQuickFramebufferObject {
    Q_OBJECT
   public:
    GLFWItem();
    ~GLFWItem();

    Renderer* createRenderer() const override;
};

class GLFWRenderer : public QQuickFramebufferObject::Renderer,
                     protected QOpenGLFunctions_3_0 {
   public:
    GLFWRenderer();
    ~GLFWRenderer();

    QOpenGLFramebufferObject* createFramebufferObject(
        const QSize& size) override;
    void render() override;

    QOpenGLFramebufferObject* m_fbo;
    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_program;
};
#endif
