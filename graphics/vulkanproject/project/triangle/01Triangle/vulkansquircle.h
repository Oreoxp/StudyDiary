// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef VULKANSQUIRCLE_H
#define VULKANSQUIRCLE_H

#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <vulkan/vulkan.h>

class SquircleRenderer;

//VulkanSquircle 类：
//    VulkanSquircle 类主要负责与Qt Quick集成，将SquircleRenderer的渲染结果显示在Qt
//Quick场景中。它继承自QQuickItem，可以作为一个自定义的Qt Quick项添加到场景中。
//主要功能如下：
//      管理SquircleRenderer的实例，以处理实际的渲染操作； 
//      将 squircle 形状的属性（如颜色、线宽等）暴露给 QML，使得可以在 QML 中直接操作这些属性； 
//      与Qt Quick的渲染循环同步，确保 Vulkan 渲染与 Qt Quick 渲染在同一帧中进行。
class VulkanSquircle : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(qreal t READ t WRITE setT NOTIFY tChanged)
    QML_ELEMENT

public:
    VulkanSquircle();

    qreal t() const { return m_t; }
    void setT(qreal t);

signals:
    void tChanged();

public slots:
    void sync();
    void cleanup();

private slots:
    void handleWindowChanged(QQuickWindow *win);

private:
    void releaseResources() override;

    qreal m_t = 0;
    SquircleRenderer *m_renderer = nullptr;
};


class VulkanTriangle : public QQuickItem {
    Q_OBJECT
    QML_ELEMENT
public:
struct QueueFamilyIndices {
  int graphicsFamily = -1;

  bool isComplete() {
    return graphicsFamily >= 0;
  }
};

public:
  void run();
  void initVulkan();
  void mainLoop();
  void cleanup();

private:
  bool CheckValidationLayerSupport();
  std::vector<const char*> getRequiredExtensions();
  void createInstance();

  void pickPhysicalDevice();
  bool isDeviceSuitable(VkPhysicalDevice device);
  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

  void createLogicalDevice();

  void setupDebugMessenger();
  VkResult CreateDebugUtilsMessengerEXT(
      VkInstance instance,
      const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
      const VkAllocationCallbacks* pAllocator,
      VkDebugUtilsMessengerEXT* pDebugMessenger);
  void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                     VkDebugUtilsMessengerEXT debugMessenger,
                                     const VkAllocationCallbacks* pAllocator);


 private:
  VkDebugUtilsMessengerEXT callback;
  VkInstance vkinstance;
  QQuickWindow* m_window;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkQueue graphicsQueue;
};
#endif
