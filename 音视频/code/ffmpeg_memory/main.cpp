#include <QtCore/QCoreApplication>
#include <iostream>
extern "C" {
#include <libavutil/avutil.h>
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    
    std::cout << "Hello World!" << av_version_info() << std::endl;
    
    return a.exec();
}
