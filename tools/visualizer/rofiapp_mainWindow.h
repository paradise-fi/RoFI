#ifndef ROFIAPP_MAINWINDOW_H
#define ROFIAPP_MAINWINDOW_H

#include "Vtk.h"

#include <QMainWindow>
#include <QMessageBox>
#include <QErrorMessage>
#include <QFileDialog>
#include <QDebug>

#include <CompatQVTKWidget.h>

#include <string.h>
#include <Configuration.h>
#include <IO.h>
#include <sstream>

#include <atoms/resources.hpp>

namespace Ui {
class Rofiapp_MainWindow;
}

class Rofiapp_MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit Rofiapp_MainWindow(QWidget *parent = nullptr);
    ~Rofiapp_MainWindow();

private slots:
    void showSphere();
    void changeBackground();
    void toggleFullScreen();

    void loadConf();
    void showConf();
    void resetCamera();
    void on_configTextWindow_textChanged();

    void saveConf();

private:
    Ui::Rofiapp_MainWindow *ui;
    float bckgValue;
    bool fullScreen;

    Configuration *current_cfg;
    bool check_cfg(bool update_current_cfg);

#if (VTK_MAJOR_VERSION == 8 && VTK_MINOR_VERSION >= 2) || VTK_MAJOR_VERSION > 8
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindow;
#else
    vtkSmartPointer<vtkRenderWindow> renderWindow;
#endif

    vtkSmartPointer<vtkRenderer> renderer;
    vtkSmartPointer<vtkCamera> camera;
};

#endif // ROFIAPP_MAINWINDOW_H
