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
#if (VTK_MAJOR_VERSION == 8 && VTK_MINOR_VERSION >= 2) || VTK_MAJOR_VERSION > 8
    renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
#else
    renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
#endif
    renderer = vtkSmartPointer<vtkRenderer>::New();
    renderWindow->AddRenderer(renderer);

#if VTK_MAJOR_VERSION >= 9
    ui->qvtkWidget->setRenderWindow(renderWindow);
#else
    ui->qvtkWidget->SetRenderWindow(renderWindow);
#endif

    renderer->SetBackground(bckgValue, bckgValue, 1.0);

    vtkSmartPointer<vtkCamera> camera = vtkSmartPointer<vtkCamera>::New();
    renderer->SetActiveCamera(camera);

    connect(ui->actionShow_sphere, SIGNAL(triggered()), this, SLOT(showSphere()));
    connect(ui->actionToggle_full_screen, SIGNAL(triggered()), this, SLOT(toggleFullScreen()));
    connect(ui->actionChange_background, SIGNAL(triggered()), this, SLOT(changeBackground()));
    connect(ui->actionQuit, SIGNAL(triggered()), this, SLOT(close()));
    connect(ui->actionShowConf, SIGNAL(triggered()), this, SLOT(showConf()));
    connect(ui->actionLoadConf, SIGNAL(triggered()), this, SLOT(loadConf()));
    connect(ui->actionSaveConf, SIGNAL(triggered()), this, SLOT(saveConf()));
    connect(ui->actionResetCamera, SIGNAL(triggered()), this, SLOT(resetCamera()));
    connect(ui->configTextWindow, SIGNAL(textChanged()),this,SLOT(on_configTextWindow_textChanged()));
}

Rofiapp_MainWindow::~Rofiapp_MainWindow()
{
    delete ui;
}

bool Rofiapp_MainWindow::check_cfg(bool update_current_cfg)
{
    bool correct = false;
    Configuration *aux_cfg;
    aux_cfg = new Configuration();

    QString lineContents="";
    lineContents = ui->configTextWindow->toPlainText();
    std::istringstream iStream(lineContents.toStdString());
    try { IO::readConfiguration(iStream, *aux_cfg); }
    catch (...) {
        qDebug("Error on parsing configuration.");
    }
    if (aux_cfg -> isValid())
    {
        correct = true;
        if (update_current_cfg)
        {
            delete current_cfg;
            current_cfg = aux_cfg;
            renderer->RemoveAllViewProps();
            VtkSupp::buildScene(current_cfg, renderer);
        } else {
            delete aux_cfg;
        }
    } else {
        delete aux_cfg;
    }
    return correct;
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
    renderWindow->Render();
    ui->qvtkWidget->update();
    ui->actionResetCamera->setEnabled(false);
}

void Rofiapp_MainWindow::changeBackground()
{
    bckgValue -= 0.1;
    if (bckgValue < 0.5) { bckgValue = 1.0; }
    renderer->SetBackground(bckgValue, bckgValue, 1.0);
    renderWindow->Render();
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

void Rofiapp_MainWindow::loadConf()
{
    QString fileName =  QFileDialog::getOpenFileName(
              this,
              "Open Configuration File",
              QDir::currentPath(),
              "All files (*.*)");
    
    if( !fileName.isNull() )
    {
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
            if (this -> check_cfg(true))
            {
                ui->actionResetCamera->setEnabled(true);
                this->resetCamera();
                this->showConf();
            }
        }
    }
 }


void Rofiapp_MainWindow::showConf()
{
    this -> check_cfg(true);
    renderWindow->Render();
    ui->qvtkWidget->update();
}

void Rofiapp_MainWindow::resetCamera()
{
    camera = renderer -> GetActiveCamera();
    Vector massCenter = current_cfg->massCenter();
    camera->SetFocalPoint(massCenter(0), massCenter(1), massCenter(2));
    camera->SetPosition(massCenter(0), massCenter(1) - 6, massCenter(2));
    camera->SetViewUp(0,0,1);

    renderer -> ResetCamera();
    renderWindow->Render();
    ui->qvtkWidget->update();
}

void Rofiapp_MainWindow::on_configTextWindow_textChanged()
{

    if (check_cfg(false))
    {
        ui->actionShowConf->setEnabled(true);
        ui->actionSaveConf->setEnabled(true);
    } else {
        ui->actionShowConf->setEnabled(false);
        ui->actionSaveConf->setEnabled(false);
    }
}

void Rofiapp_MainWindow::saveConf()
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
