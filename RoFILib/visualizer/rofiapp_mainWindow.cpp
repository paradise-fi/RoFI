#include "rofiapp_mainWindow.h"
#include "ui_rofiapp_mainWindow.h"

Rofiapp_MainWindow::Rofiapp_MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Rofiapp_MainWindow)
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

Rofiapp_MainWindow::~Rofiapp_MainWindow()
{
    delete ui;
}

void Rofiapp_MainWindow::on_pushButton_2_clicked()
{
    ui->qvtkWidget->hide();
    renderer->AddActor( sphereActor );
    ui->qvtkWidget->show();
}

void Rofiapp_MainWindow::on_pushButton_4_clicked()
{
    bckgValue -= 0.1;
    if (bckgValue < 0.5) { bckgValue = 1.0; }
    renderer->SetBackground(bckgValue, bckgValue, 1.0);
    ui->qvtkWidget->update();
}

void Rofiapp_MainWindow::on_pushButton_3_clicked()
{
    if (not fullScreen) {
        this->showFullScreen();
    } else {
        this->showNormal();
    }
    fullScreen = not fullScreen;
}

