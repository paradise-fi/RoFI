#include "rofiapp_mainWindow.h"
#include <QApplication>
#include <QSurfaceFormat>

int main(int argc, char *argv[])
{


// VTK 8.2 and newer
//    QSurfaceFormat::setDefaultFormat(QVTKOpenGLWidget::defaultFormat());  // VTK 8.2 and newer

// VTK 7.1 and older
    auto format = QSurfaceFormat::defaultFormat();
    format.setStencilBufferSize(8);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication a(argc, argv);
    Rofiapp_MainWindow w;


    w.show();

    return a.exec();
}
