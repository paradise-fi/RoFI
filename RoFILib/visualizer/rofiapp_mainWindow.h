#ifndef ROFIAPP_MAINWINDOW_H
#define ROFIAPP_MAINWINDOW_H

#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>

#include <QMainWindow>

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
    void on_pushButton_2_clicked();
    void on_pushButton_4_clicked();

    void on_pushButton_3_clicked();

private:
    Ui::Rofiapp_MainWindow *ui;
    float bckgValue;
    bool fullScreen;

    /* Sphere */
    vtkSmartPointer<vtkSphereSource> sphereSource;
    vtkSmartPointer<vtkPolyDataMapper> sphereMapper;
    vtkSmartPointer<vtkActor> sphereActor;

    /* Renderer */
    vtkSmartPointer<vtkRenderer> renderer;

};

#endif // ROFIAPP_MAINWINDOW_H
