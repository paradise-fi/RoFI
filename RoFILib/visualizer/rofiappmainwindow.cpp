#include "rofiappmainwindow.h"
#include "ui_rofiappmainwindow.h"

RofiappMainWindow::RofiappMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::RofiappMainWindow)
{
    ui->setupUi(this);
    bckgValue = 1.0;
    fullScreen = false;

    sphereSource = vtkSmartPointer<vtkSphereSource>::New();
    sphereMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    sphereMapper->SetInputConnection( sphereSource->GetOutputPort() );

    sphereActor = vtkSmartPointer<vtkActor>::New();
    sphereActor->SetMapper( sphereMapper );

    /* Renderer */
    renderer = vtkSmartPointer<vtkRenderer>::New();
    renderer->SetBackground(bckgValue, bckgValue, 1.0);
    ui->qvtkWidget->GetRenderWindow()->AddRenderer( renderer );

}

RofiappMainWindow::~RofiappMainWindow()
{
    delete ui;
}

void RofiappMainWindow::on_pushButton_2_clicked()
{
    ui->qvtkWidget->hide();
    renderer->AddActor( sphereActor );
    ui->qvtkWidget->show();
}

void RofiappMainWindow::on_pushButton_4_clicked()
{
    bckgValue -= 0.1;
    if (bckgValue < 0.5) { bckgValue = 1.0; }
    renderer->SetBackground(bckgValue, bckgValue, 1.0);
    ui->qvtkWidget->update();
}

void RofiappMainWindow::on_pushButton_3_clicked()
{
    if (not fullScreen) {
        this->showFullScreen();
    } else {
        this->showNormal();
    }
    fullScreen = not fullScreen;
}

