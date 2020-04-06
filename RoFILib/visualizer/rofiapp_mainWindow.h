#ifndef ROFIAPP_MAINWINDOW_H
#define ROFIAPP_MAINWINDOW_H

#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkProperty.h>

#include <vtkRenderWindowInteractor.h>
#include <vtkProperty.h>
#include <vtkSTLReader.h>
#include <vtkMatrix4x4.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkCamera.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkAxesActor.h>
#include <vtkWindowToImageFilter.h>
#include <vtkPNGWriter.h>

#include <locale>  //for locale settings, vtkObjReader si locale sensitive
#include <vtkOBJReader.h>
#include <vtkRenderLargeImage.h>

#include <QMainWindow>
#include <QErrorMessage>
#include <QFileDialog>
#include <QDebug>

#include <string.h>
#include <Configuration.h>
#include <IO.h>
#include <sstream>

#include <resources.hpp>

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

private:
    Ui::Rofiapp_MainWindow *ui;
    float bckgValue;
    bool fullScreen;

    Configuration *current_cfg;

    /* Sphere */
    vtkSmartPointer<vtkSphereSource> sphereSource;
    vtkSmartPointer<vtkPolyDataMapper> sphereMapper;
    vtkSmartPointer<vtkActor> sphereActor;

    vtkSmartPointer<vtkRenderWindow> renderWindow;

    vtkSmartPointer<vtkRenderer> renderer;
    vtkSmartPointer<vtkCamera> camera;
    QList<vtkSmartPointer<vtkActor>> *addedActorsList;


    std::filesystem::path getModel( const std::string& model );
    void addActor(const std::string &model, const Matrix &matrix, int color);

    const int colors[10][3] = { {255, 255, 255},
                                {0, 255, 0},
                                {0, 0, 255},
                                {191, 218, 112},
                                {242, 202, 121},
                                {218, 152, 207},
                                {142, 202, 222},
                                {104, 135, 205},
                                {250, 176, 162},
                                {234, 110, 111}};

};

#endif // ROFIAPP_MAINWINDOW_H
