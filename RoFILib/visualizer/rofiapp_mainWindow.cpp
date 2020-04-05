#include "rofiapp_mainWindow.h"
#include "ui_rofiapp_mainWindow.h"

Matrix shoeMatrix()
{
    return rotate(M_PI/2, X);
}

Matrix bodyMatrix(double alpha)
{
    double diff = alpha * M_PI/180.0;
    return rotate(M_PI/2 + diff, X);
}

Matrix dockMatrix(ConnectorId dock, bool on, double onCoeff = -1)
{
    double d;
    if (onCoeff < 0){
        d = on ? 0.05 : 0;
    } else  {
        d = onCoeff * 0.05;
    }
    Matrix docks[3] = {
            translate(Vector{d,0,0}) * rotate(M_PI, Z), // XPlus
            translate(Vector{-d,0,0}) * identity, // XMinus
            translate(Vector{0,0,-d}) * rotate(-M_PI/2, Y) // ZMinus
    };
    return docks[dock];
}


Rofiapp_MainWindow::Rofiapp_MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Rofiapp_MainWindow)
{
    ui->setupUi(this);
    bckgValue = 0.9;
    fullScreen = false;

    sphereSource = vtkSmartPointer<vtkSphereSource>::New();
    sphereMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    sphereMapper->SetInputConnection( sphereSource->GetOutputPort() );

    sphereActor = vtkSmartPointer<vtkActor>::New();
    sphereActor->GetProperty()->SetFrontfaceCulling(true);
    sphereActor->SetMapper( sphereMapper );

    /* Fresh Renderer */
    renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
    renderer = vtkSmartPointer<vtkRenderer>::New();
    renderWindow->AddRenderer(renderer);
    ui->qvtkWidget->SetRenderWindow(renderWindow);

    renderer->SetBackground(bckgValue, bckgValue, 1.0);

    vtkSmartPointer<vtkCamera> camera = vtkSmartPointer<vtkCamera>::New();
    renderer->SetActiveCamera(camera);

    addedActorsList = new QList<vtkSmartPointer<vtkActor>>;

    connect(ui->actionShow_sphere, SIGNAL(triggered()), this, SLOT(showSphere()));
    connect(ui->actionToggle_full_screen, SIGNAL(triggered()), this, SLOT(toggleFullScreen()));
    connect(ui->actionChange_background, SIGNAL(triggered()), this, SLOT(changeBackground()));
    connect(ui->actionQuit, SIGNAL(triggered()), this, SLOT(close()));
}

Rofiapp_MainWindow::~Rofiapp_MainWindow()
{
    delete ui;
}

void Rofiapp_MainWindow::showSphere()
{
    for (QList<vtkSmartPointer<vtkActor>>::iterator i = addedActorsList->begin(); i != addedActorsList->end(); i++) {
        renderer->RemoveActor( *i );
    }
    addedActorsList->clear();
    renderer->AddActor( sphereActor );
    addedActorsList->append( sphereActor );
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
        QFile* inputFile_f = new QFile(fileName);
        inputFile_f->open(QIODevice::ReadOnly);
        QTextStream* inputStream = new QTextStream(inputFile_f);
        while( !inputStream->atEnd() )
           {
              lineContents += inputStream->readLine()+"\n";
           }
        ui->configTextWindow->setPlainText(lineContents);
        inputFile_f->close();
        delete inputFile_f;
        delete inputStream;

        this->on_showConf_clicked();
        ui->resetCamera->setEnabled(true);
    }
 }


inline vtkSmartPointer<vtkMatrix4x4> convertMatrix( const Matrix& m )
{
    vtkSmartPointer<vtkMatrix4x4> mat = vtkSmartPointer<vtkMatrix4x4>::New();
    mat->SetElement(0,0, m(0,0));
    mat->SetElement(0,1, m(0,1));
    mat->SetElement(0,2, m(0,2));
    mat->SetElement(0,3, m(0,3));
    mat->SetElement(1,0, m(1,0));
    mat->SetElement(1,1, m(1,1));
    mat->SetElement(1,2, m(1,2));
    mat->SetElement(1,3, m(1,3));
    mat->SetElement(2,0, m(2,0));
    mat->SetElement(2,1, m(2,1));
    mat->SetElement(2,2, m(2,2));
    mat->SetElement(2,3, m(2,3));
    mat->SetElement(3,0, m(3,0));
    mat->SetElement(3,1, m(3,1));
    mat->SetElement(3,2, m(3,2));
    mat->SetElement(3,3, m(3,3));
    return mat;
}

std::filesystem::path Rofiapp_MainWindow::getModel( const std::string& model ) {
    static ResourceFile body = LOAD_RESOURCE_FILE( visualizer_model_body_obj );
    static ResourceFile shoe = LOAD_RESOURCE_FILE( visualizer_model_shoe_obj );
    static ResourceFile connector = LOAD_RESOURCE_FILE( visualizer_model_connector_obj );

    if ( model == "body" )
        return body.name();
    if ( model == "shoe" )
        return shoe.name();
    if ( model == "connector" )
        return connector.name();
    throw std::runtime_error( "Invalid model '" + model + "' requested" );
}


void Rofiapp_MainWindow::addActor(const std::string &model, const Matrix &matrix, int color)
{

    // vtkOBJReader is locale sensitive!
    std::locale currentLocale;
    currentLocale = std::locale::global(std::locale::classic());
    vtkSmartPointer<vtkOBJReader> reader = vtkSmartPointer<vtkOBJReader>::New();
    reader->SetFileName( getModel(model).c_str() );
    reader->Update();
    std::locale::global(currentLocale);

    vtkSmartPointer<vtkTransform> rotation = vtkSmartPointer<vtkTransform>::New();
    rotation->SetMatrix( convertMatrix(matrix) );

    vtkSmartPointer<vtkTransformPolyDataFilter> filter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    filter->SetTransform( rotation );
    filter->SetInputConnection( reader->GetOutputPort() );

    vtkSmartPointer<vtkPolyDataMapper> frameMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    frameMapper->SetInputConnection(filter->GetOutputPort());

    vtkSmartPointer<vtkActor> frameActor = vtkSmartPointer<vtkActor>::New();
    frameActor->GetProperty()->SetColor(colors[color][0]/256.0, colors[color][1]/256.0 , colors[color][2]/256.0);
    frameActor->GetProperty()->SetFrontfaceCulling(true);
    frameActor->SetMapper(frameMapper);
    frameActor->SetPosition( matrix(0,3), matrix(1,3), matrix(2,3) );
    frameActor->SetScale( 1 / 95.0 );

    renderer->AddActor( frameActor );
    addedActorsList->append( frameActor );
}

void Rofiapp_MainWindow::on_showConf_clicked()
{
    QString lineContents="";
    lineContents = ui->configTextWindow->toPlainText();

    std::istringstream iStream(lineContents.toStdString());
    if (!IO::readConfiguration(iStream, current_cfg)) {
        return;
    }


    for (QList<vtkSmartPointer<vtkActor>>::iterator i = addedActorsList->begin(); i != addedActorsList->end(); i++) {
        renderer->RemoveActor( *i );
    }
    addedActorsList->clear();

    //qDebug("Test1");

    current_cfg.computeMatrices();

    //qDebug("Test2");

    for ( const auto& [id, matrices] : current_cfg.getMatrices())
    {
        int color =  id % 7 + 3;
        const Module& mod = current_cfg.getModules().at(id);
        EdgeList edges = current_cfg.getEdges().at(id);
        for (ShoeId s : {A, B})
        {
            Joint j = s == A ? Alpha : Beta;
            this->addActor("shoe", matrices[s] * shoeMatrix(), color);
            this->addActor("body", matrices[s] * bodyMatrix(mod.getJoint(j)), color);

            for (ConnectorId dock : {XPlus, XMinus, ZMinus})
            {
                bool on = edges[s * 3 + dock].has_value();
                double onCoeff = on ? edges[s * 3 + dock].value().onCoeff() : 0;
                this->addActor("connector", matrices[s] * dockMatrix(dock, on, onCoeff), color);
            }
        }
    }

    camera = renderer -> GetActiveCamera();
    Vector massCenter = current_cfg.massCenter();
    camera->SetFocalPoint(massCenter(0), massCenter(1), massCenter(2));
    camera->SetPosition(massCenter(0), massCenter(1) - 6, massCenter(2));
    camera->SetViewUp(0,0,1);


    //qDebug("Test3");

    ui->qvtkWidget->GetRenderWindow()->Render();
    ui->qvtkWidget->update();

    //qDebug("Test4");

}

void Rofiapp_MainWindow::on_resetCamera_clicked()
{
    camera = renderer -> GetActiveCamera();
    Vector massCenter = current_cfg.massCenter();
    camera->SetFocalPoint(massCenter(0), massCenter(1), massCenter(2));
    camera->SetPosition(massCenter(0), massCenter(1) - 6, massCenter(2));
    camera->SetViewUp(0,0,1);
    ui->qvtkWidget->update();

}
