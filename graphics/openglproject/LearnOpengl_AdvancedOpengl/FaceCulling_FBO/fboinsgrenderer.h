#ifndef FBOINSGRENDERER_H
#define FBOINSGRENDERER_H

#include <GLFW/glfw3.h>
#include <QElapsedTimer>
#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLFunctions_3_0>
#include <QOpenGLVertexArrayObject>
#include <QtOpenGL/QOpenGLShaderProgram>
#include <QtOpenGL/QOpenGLTexture>
#include <QtQuick/QQuickFramebufferObject>
#include <QtQuick/QQuickWindow>
#include "sphere.h"
#include "camera.h"

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
  Q_INVOKABLE void mouseChange(qreal x, qreal y);
  Renderer* createRenderer() const override;

 signals:
  void mouseChangeChangedd(qreal x, qreal y);
  void trianglePosChanged();
  void keyDownChanged(GLFWItem::CLICK_TYPE);
};

class GLFWRenderer : public QObject,
                     public QQuickFramebufferObject::Renderer,
                     protected QOpenGLFunctions_3_0 {
  Q_OBJECT
 public:
  GLFWRenderer();
  ~GLFWRenderer();

  QOpenGLFramebufferObject* createFramebufferObject(const QSize& size) override;
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

 public slots:
  void onTrianglePosChanged();
  void onKeyDownChanged(GLFWItem::CLICK_TYPE);
  void onMouseChangeChanged(qreal x, qreal y);

 public:
  GLFWItem* m_item;
  QQuickWindow* m_window = nullptr;
  QOpenGLFramebufferObject* m_fbo;
  QOpenGLFramebufferObject* back_fbo;
  QOpenGLShaderProgram* m_main_shader; 
  QOpenGLShaderProgram* m_shader;
  QOpenGLShaderProgram* m_shaderSolid;
  QOpenGLShaderProgram* m_shader2;
  QOpenGLShaderProgram* m_skybox_shader; 
  QOpenGLFramebufferObject* m_fbo_cube[6];
  QOpenGLTexture* m_cube_map;
  GLuint m_vao;
  GLuint m_vao2;
  GLuint m_vao_quad;
  GLuint m_skyboxVAO;
  GLuint cubemapTexture;
  GLuint m_vbo;
  GLuint m_vbo2;
  GLuint framebuffer;
  GLuint textureColorbuffer;
  GLuint m_program;
  GLuint texture1;
  GLuint texture2;
  GLuint cubeMapTexture;
  QElapsedTimer timer;
  QVector3D m_view = {0., 0., -3.0f};

  QVector3D cameraPos = QVector3D(0.0f, 0.0f, 3.0f);
  QVector3D cameraFront = QVector3D(0.0f, 0.0f, -1.0f);
  QVector3D cameraUp = QVector3D(0.0f, 1.0f, 0.0f);
  float cameraSpeed = 0.05f;

  Camera camera;
  float deltaTime = 0;
  float lastFrame = 0;
  float lastX = 0;
  float lastY = 0;
  float firstMouse = false;
  /*Sphere m_sphere;*/
};
#endif