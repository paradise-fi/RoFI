#include "main/rofirun.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    RofiRun w;
    w.show();
    return a.exec();
}
