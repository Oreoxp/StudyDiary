#include <QGuiApplication>

#include <QtQuick/QQuickView>
#include <QTCore\QTextCodec>

#include "fboinsgrenderer.h"

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

    qmlRegisterType<GLFWItem>("GLFWItem", 1, 0, "GLFWItem");
    //QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    QQuickView view;
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.setSource(QUrl("qrc:/main.qml"));
    view.show();

    return app.exec();
}
