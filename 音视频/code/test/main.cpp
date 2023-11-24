#include <QtCore/QCoreApplication>

extern "C"{
#include <libavutil/avutil.h>
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    printf("nihao,%s\n", av_version_info());
    return a.exec();
}
