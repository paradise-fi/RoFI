#include "rofiapp_mainWindow.h"
#include "ui_rofiapp_mainWindow.h"

Rofiapp_MainWindow::Rofiapp_MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Rofiapp_MainWindow)
{
    ui->setupUi(this);
    bckgValue = 0.9;
    fullScreen = false;

    current_cfg = new Configuration();


    /* Fresh Renderer */
    renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
    renderer = vtkSmartPointer<vtkRenderer>::New();
    renderWindow->AddRenderer(renderer);

    ui->qvtkWidget->SetRenderWindow(renderWindow);

    renderer->SetBackground(bckgValue, bckgValue, 1.0);

    vtkSmartPointer<vtkCamera> camera = vtkSmartPointer<vtkCamera>::New();
    renderer->SetActiveCamera(camera);

    connect(ui->actionShow_sphere, SIGNAL(triggered()), this, SLOT(showSphere()));
    connect(ui->actionToggle_full_screen, SIGNAL(triggered()), this, SLOT(toggleFullScreen()));
    connect(ui->actionChange_background, SIGNAL(triggered()), this, SLOT(changeBackground()));
    connect(ui->actionQuit, SIGNAL(triggered()), this, SLOT(close()));
    connect(ui->configTextWindow, SIGNAL(textChanged()),this,SLOT(on_configTextWindow_textChanged()));
}

Rofiapp_MainWindow::~Rofiapp_MainWindow()
{
    delete ui;
}

void Rofiapp_MainWindow::showSphere()
{
    /* Sphere */
    vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
    vtkSmartPointer<vtkPolyDataMapper> sphereMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    vtkSmartPointer<vtkActor> sphereActor = vtkSmartPointer<vtkActor>::New();

    sphereMapper->SetInputConnection( sphereSource->GetOutputPort() );
    sphereActor->GetProperty()->SetFrontfaceCulling(true);
    sphereActor->SetMapper( sphereMapper );

    renderer->RemoveAllViewProps();

    renderer->AddActor( sphereActor );
    renderer->ResetCamera();
    ui->qvtkWidget->update();
    ui->resetCamera->setEnabled(false);
}

void Rofiapp_MainWindow::changeBackground()
{
    bckgValue -= 0.1;
    if (bckgValue < 0.5) { bckgValue = 1.0; }
    renderer->SetBackground(bckgValue, bckgValue, 1.0);
    ui->qvtkWidget->update();
}

void Rofiapp_MainWindow::toggleFullScreen()
{
    if (not fullScreen) {
        this->showFullScreen();
    } else {
        this->showNormal();
    }
    fullScreen = not fullScreen;
}

void Rofiapp_MainWindow::on_loadConf_clicked()
{
    QString fileName =  QFileDialog::getOpenFileName(
              this,
              "Open Configuration File",
              QDir::currentPath(),
              "All files (*.*)");
    
    if( !fileName.isNull() ) {

        QString lineContents="";
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::information(this, tr("Unable to open file"), file.errorString());
        } else {
            QTextStream inputStream(&file);
            while( !inputStream.atEnd() )
               {
                  lineContents += inputStream.readLine()+"\n";
               }
            file.close();
            ui->configTextWindow->setPlainText(lineContents);
            this->on_showConf_clicked();
            ui->resetCamera->setEnabled(true);
        }
    }
 }


void Rofiapp_MainWindow::on_showConf_clicked()
{
    QString lineContents="";
    lineContents = ui->configTextWindow->toPlainText();

    delete current_cfg;
    current_cfg = new Configuration();

    std::istringstream iStream(lineContents.toStdString());
    try { IO::readConfiguration(iStream, *current_cfg); }
    catch (...) {
        qDebug("Error on parsing configuration.");
    }
    current_cfg -> computeMatrices();

    renderer->RemoveAllViewProps();
    VtkSupp::buildScene(current_cfg, renderer);

    ui->resetCamera->setEnabled(true);
    this -> on_resetCamera_clicked();
}

void Rofiapp_MainWindow::on_resetCamera_clicked()
{
    camera = renderer -> GetActiveCamera();
    Vector massCenter = current_cfg->massCenter();
    camera->SetFocalPoint(massCenter(0), massCenter(1), massCenter(2));
    camera->SetPosition(massCenter(0), massCenter(1) - 6, massCenter(2));
    camera->SetViewUp(0,0,1);

    ui->qvtkWidget->update();
}

void Rofiapp_MainWindow::on_configTextWindow_textChanged()
{

    aux_cfg = new Configuration;
    QString lineContents="";
    lineContents = ui->configTextWindow->toPlainText();
    std::istringstream iStream(lineContents.toStdString());
    try {
        IO::readConfiguration(iStream, *aux_cfg);
        ui->showConf->setEnabled(true);
        ui->saveConf->setEnabled(true);
    } catch (...) {
        ui->showConf->setEnabled(false);
        ui->saveConf->setEnabled(false);
    }
    delete aux_cfg;
}

void Rofiapp_MainWindow::on_saveConf_clicked()
{
    QString fileName =  QFileDialog::getSaveFileName(
              this,
              "Save Configuration File",
              QDir::currentPath(),
              "All files (*.*)");

    if( !fileName.isNull() ) {
        QString lineContents="";
        lineContents = ui->configTextWindow->toPlainText();
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly)) {
            QMessageBox::information(this, tr("Unable to open file"), file.errorString());
        } else {
            QTextStream out(&file);
            out << lineContents;
            file.close();
        }
    }
}
