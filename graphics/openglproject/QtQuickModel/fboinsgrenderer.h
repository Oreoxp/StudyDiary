#ifndef FBOINSGRENDERER_H
#define FBOINSGRENDERER_H

#include <GLFW/glfw3.h>
#include <QOpenGLFunctions_3_0>
#include <QtOpenGL/QOpenGLShaderProgram>
#include <QtOpenGL/QOpenGLTexture>
#include <QOpenGLShaderProgram>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QQuickFramebufferObject>
#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLVertexArrayObject>
#include <QElapsedTimer>
#include "rendererStruct.h"

class GLFWItem : public QQuickFramebufferObject {
  Q_OBJECT
  QML_NAMED_ELEMENT(Renderer)
public:
  enum CLICK_TYPE { 
    DOWN_LEFT,
    DOWN_RIGHT,
    DOWN_UP,
    DOWN_DOWN,
  };
    GLFWItem();
    ~GLFWItem();

    Q_ENUM(CLICK_TYPE);
    Q_INVOKABLE void changeTrianglePos();
    Q_INVOKABLE void changeKeyDown(GLFWItem::CLICK_TYPE);
    Renderer* createRenderer() const override;

signals:
    void trianglePosChanged();
 void keyDownChanged(GLFWItem::CLICK_TYPE);

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

    void mvp();
    void clearWindow();
    void loadTexture(QString path, GLuint* texture);
   public slots:
    void onTrianglePosChanged();
    void onKeyDownChanged(GLFWItem::CLICK_TYPE);

   public:
    GLFWItem* m_item;
    QQuickWindow* m_window = nullptr;
    QOpenGLFramebufferObject* m_fbo;
    GLuint m_vao;
    GLuint m__light_vao;
    GLuint m_light_cube_vao;
    GLuint m_vbo;
    GLuint m_program;
    GLuint texture1;
    GLuint texture2;
    GLuint m_diffuse_map;
    QElapsedTimer timer;
    QVector3D m_view = {0.,0.,-3.0f};

    QVector3D cameraPos = QVector3D(0.0f, 0.0f, 5.0f);
    QVector3D cameraFront = QVector3D(0.0f, 0.0f, -1.0f);
    QVector3D cameraUp = QVector3D(0.0f, 1.0f, 0.0f);
    float cameraSpeed = 0.05f;

    QOpenGLShaderProgram m_light_shader;
    QOpenGLShaderProgram m_light_cube_shader;

    Model* m_model;
};
#endif
