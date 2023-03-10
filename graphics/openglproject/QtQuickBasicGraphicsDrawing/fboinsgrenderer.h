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

class GLFWItem : public QQuickFramebufferObject {
    Q_OBJECT
   public:
    GLFWItem();
    ~GLFWItem();

    Q_INVOKABLE void changeTrianglePos();
    Renderer* createRenderer() const override;

signals:
    void trianglePosChanged();

private:
};

class GLFWRenderer : public QObject,public QQuickFramebufferObject::Renderer,
                     protected QOpenGLFunctions_3_0 {
 Q_OBJECT
public:
    GLFWRenderer();
    ~GLFWRenderer();

    QOpenGLFramebufferObject* createFramebufferObject(
        const QSize& size) override;
    void render() override;
    void synchronize(QQuickFramebufferObject* item) {
      m_item = static_cast<GLFWItem*>(item);
      if (m_item) {
        if (!m_window) {
          m_window = m_item->window();
        }
      }
    }

public slots:
    void onTrianglePosChanged();
public:
    GLFWItem* m_item;
    QQuickWindow* m_window = nullptr;
    QOpenGLFramebufferObject* m_fbo;
    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_program;
};
#endif
