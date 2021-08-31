#include "rofiapp_mainWindow.h"
#include <QApplication>
#include <QSurfaceFormat>

int main(int argc, char *argv[])
{


#if (VTK_MAJOR_VERSION == 8 && VTK_MINOR_VERSION >= 2) || VTK_MAJOR_VERSION > 8
    QSurfaceFormat::setDefaultFormat(QVTKOpenGLWidget::defaultFormat());
#else
    auto format = QSurfaceFormat::defaultFormat();
    format.setStencilBufferSize(8);
    QSurfaceFormat::setDefaultFormat(format);
#endif

    QApplication a(argc, argv);
    Rofiapp_MainWindow w;


    w.show();

    return a.exec();
}
