// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef VULKANSQUIRCLE_H
#define VULKANSQUIRCLE_H

#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>

class SquircleRenderer;

//VulkanSquircle �ࣺ
//    VulkanSquircle ����Ҫ������Qt Quick���ɣ���SquircleRenderer����Ⱦ�����ʾ��Qt
//Quick�����С����̳���QQuickItem��������Ϊһ���Զ����Qt Quick����ӵ������С�
//��Ҫ�������£�
//      ����SquircleRenderer��ʵ�����Դ���ʵ�ʵ���Ⱦ������ 
//      �� squircle ��״�����ԣ�����ɫ���߿�ȣ���¶�� QML��ʹ�ÿ����� QML ��ֱ�Ӳ�����Щ���ԣ� 
//      ��Qt Quick����Ⱦѭ��ͬ����ȷ�� Vulkan ��Ⱦ�� Qt Quick ��Ⱦ��ͬһ֡�н��С�
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

#endif
