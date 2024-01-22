#include "rofiapp_mainWindow.h"
#include <QApplication>
#include <QSurfaceFormat>

int main(int argc, char *argv[])
{


#if (VTK_MAJOR_VERSION == 8 && VTK_MINOR_VERSION >= 2) || VTK_MAJOR_VERSION > 8
    QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());
#else
    auto format = QSurfaceFormat::defaultFormat();
    format.setStencilBufferSize(8);
    QSurfaceFormat::setDefaultFormat(format);
#endif

    QApplication a(argc, argv);
    Rofiapp_MainWindow w;

    if (QApplication::arguments().size() == 2) {
        QString fileName = QApplication::arguments().at(1);
        QFile file(fileName);
        w.loadConfFile(file);
    } else if (QApplication::arguments().size() > 2) {
        qDebug("Too many arguments.");
        qDebug("usage: rofi-app <configuration.in>");
        std::exit(EXIT_FAILURE);
    }

    w.show();

    return a.exec();
}
