// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QGuiApplication>
#include <QtQuick/QQuickView>
#include "vulkansquircle.h"

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    qmlRegisterType<VulkanTriangle>("VulkanTriangle", 1, 0, "VulkanTriangle");
    qmlRegisterType<VulkanSquircle>("VulkanUnderQML", 1, 0, "VulkanSquircle");
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Vulkan);

    QQuickView view;
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.setSource(QUrl("./main.qml"));
    view.show();

    return app.exec();
}
