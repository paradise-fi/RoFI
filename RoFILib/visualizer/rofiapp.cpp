#include "rofiapp_mainWindow.h"
#include <QApplication>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Rofiapp_MainWindow w;

    w.show();

    return a.exec();
}
