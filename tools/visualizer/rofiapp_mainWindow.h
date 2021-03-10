#ifndef ROFIAPP_MAINWINDOW_H
#define ROFIAPP_MAINWINDOW_H

#include "Vtk.h"

#include <QMainWindow>
#include <QMessageBox>
#include <QErrorMessage>
#include <QFileDialog>
#include <QDebug>

// #include <QVTKOpenGLWidget.h>   // VTK 8.2 and newer
#include <QVTKWidget.h>         // VTK 7.1 and older


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

    void on_loadConf_clicked();
    void on_showConf_clicked();
    void on_resetCamera_clicked();
    void on_configTextWindow_textChanged();

    void on_saveConf_clicked();

private:
    Ui::Rofiapp_MainWindow *ui;
    float bckgValue;
    bool fullScreen;

    Configuration *current_cfg;
    Configuration *aux_cfg;

//    vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindow;  // VTK 8.2 and newer
    vtkSmartPointer<vtkRenderWindow> renderWindow;  // VTK 7.1 and older

    vtkSmartPointer<vtkRenderer> renderer;
    vtkSmartPointer<vtkCamera> camera;
};

#endif // ROFIAPP_MAINWINDOW_H
