#include <QGuiApplication>

#include <QtQuick/QQuickView>

#include "fboinsgrenderer.h"

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    qmlRegisterType<GLFWItem>("GLFWItem", 1, 0,
                                  "GLFWItem");

    QQuickView view;
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.setSource(QUrl("qrc:/main.qml"));
    view.show();

    return app.exec();
}
