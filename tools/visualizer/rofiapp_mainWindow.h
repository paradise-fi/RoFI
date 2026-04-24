#ifndef ROFIAPP_MAINWINDOW_H
#define ROFIAPP_MAINWINDOW_H

#include "Vtk.h"

#include <QMainWindow>
#include <QMessageBox>
#include <QErrorMessage>
#include <QFileDialog>
#include <QDebug>

#include <QVTKOpenGLNativeWidget.h>
#include <vtkGenericOpenGLRenderWindow.h>

#include <string.h>
#include <legacy/configuration/Configuration.h>
#include <legacy/configuration/IO.h>
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
    double bckgValue;
    bool fullScreen;

    Configuration *current_cfg;
    bool check_cfg(bool update_current_cfg);

    vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindow;

    vtkSmartPointer<vtkRenderer> renderer;
    vtkSmartPointer<vtkCamera> camera;
};

#endif // ROFIAPP_MAINWINDOW_H
