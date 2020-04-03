#ifndef ROFIAPPMAINWINDOW_H
#define ROFIAPPMAINWINDOW_H

#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>

#include <QMainWindow>

namespace Ui {
class RofiappMainWindow;
}

class RofiappMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit RofiappMainWindow(QWidget *parent = nullptr);
    ~RofiappMainWindow();

private slots:
    void on_pushButton_2_clicked();
    void on_pushButton_4_clicked();

    void on_pushButton_3_clicked();

private:
    Ui::RofiappMainWindow *ui;
    float bckgValue;
    bool fullScreen;

    /* Sphere */
    vtkSmartPointer<vtkSphereSource> sphereSource;
    vtkSmartPointer<vtkPolyDataMapper> sphereMapper;
    vtkSmartPointer<vtkActor> sphereActor;

    /* Renderer */
    vtkSmartPointer<vtkRenderer> renderer;

};

#endif // ROFIAPPMAINWINDOW_H
