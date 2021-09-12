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

    interactorStyle = vtkSmartPointer<InteractorStyle>::New();
    renderWindow->GetInteractor()->SetInteractorStyle(interactorStyle);

    vtkSmartPointer<vtkCallbackCommand> onPartSelected = vtkSmartPointer<vtkCallbackCommand>::New();
    onPartSelected->SetClientData(this);
    onPartSelected->SetCallback(Rofiapp_MainWindow::onPartSelected);
    interactorStyle->AddObserver(interactorStyle->PartSelectedEvent, onPartSelected);

    connect(ui->actionShow_sphere, SIGNAL(triggered()), this, SLOT(showSphere()));
    connect(ui->actionToggle_full_screen, SIGNAL(triggered()), this, SLOT(toggleFullScreen()));
    connect(ui->actionChange_background, SIGNAL(triggered()), this, SLOT(changeBackground()));
    connect(ui->actionQuit, SIGNAL(triggered()), this, SLOT(close()));
    connect(ui->actionShowConf, SIGNAL(triggered()), this, SLOT(showConf()));
    connect(ui->actionLoadConf, SIGNAL(triggered()), this, SLOT(loadConf()));
    connect(ui->actionSaveConf, SIGNAL(triggered()), this, SLOT(saveConf()));
    connect(ui->actionResetCamera, SIGNAL(triggered()), this, SLOT(resetCamera()));
    connect(ui->angleGammaDial, SIGNAL(valueChanged(int)), this, SLOT(angleGammaDial_changed(int)));
    connect(ui->angleGammaDial, SIGNAL(sliderReleased()), this, SLOT(angleDial_released()));
    connect(ui->angleAlphaDial, SIGNAL(valueChanged(int)), this, SLOT(angleAlphaBetaDial_changed(int)));
    connect(ui->angleAlphaDial, SIGNAL(sliderReleased()), this, SLOT(angleDial_released()));
    connect(ui->angleBetaDial, SIGNAL(valueChanged(int)), this, SLOT(angleAlphaBetaDial_changed(int)));
    connect(ui->angleBetaDial, SIGNAL(sliderReleased()), this, SLOT(angleDial_released()));
    connect(ui->connectedCheckBox, SIGNAL(toggled(bool)), this, SLOT(connectedCheckBox_toggled(bool)));
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
            interactorStyle->clear();
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

    interactorStyle->clear();
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
        QFile file(fileName);
        loadConfFile(file);
    }
 }

void Rofiapp_MainWindow::loadConfFile(QFile &file)
{
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::information(this, tr("Unable to open file"), file.errorString());
    } else {
        QString lineContents="";
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


void Rofiapp_MainWindow::showConf()
{
    this -> check_cfg(true);
    renderWindow->Render();
    ui->qvtkWidget->update();
    ui->actionResetCamera->setEnabled(true);
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

QTextCursor Rofiapp_MainWindow::findRegex(QRegularExpression regex) {
    activeCursor.setPosition(0);
    QTextCursor cursor = ui->configTextWindow->document()->find(regex, activeCursor);
    return cursor;
}

QTextCursor Rofiapp_MainWindow::findBodyInCode(ID moduleId) {
    const QRegularExpression findBody(QString("^M\\s+%1\\s+-?\\d+\\s+-?\\d+\\s+\\K-?\\d+").arg(moduleId), QRegularExpression::MultilineOption);
    return this->findRegex(findBody);
}

QTextCursor Rofiapp_MainWindow::findShoeInCode(ID moduleId, ShoeId shoe) {
    const QRegularExpression findShoe(QString("^M\\s+%1%2\\s+\\K-?\\d+").arg(moduleId).arg(shoe == ShoeId::A ? "" : "\\s+-?\\d+"), QRegularExpression::MultilineOption);
    return this->findRegex(findShoe);
}

QTextCursor Rofiapp_MainWindow::findConnectorInCode(ID moduleId, ShoeId shoe, ConnectorId dock) {
    const QRegularExpression findEdge(QString("^(E\\s+%1\\s+%2\\s+%3\\s+[NESW0123]\\s+([012]|\\+X|-X|-Z)\\s+[01AB]\\s+\\d+|E\\s+\\d+\\s+[01AB]\\s+([012]|\\+X|-X|-Z)\\s+[NESW0123]\\s+%3\\s+%2\\s+%1)$").arg(moduleId).arg(shoe).arg(dock), QRegularExpression::MultilineOption);
    return this->findRegex(findEdge);
}

void Rofiapp_MainWindow::setActiveCursor(QTextCursor cursor) {
    activeCursor = cursor;
    ui->configTextWindow->setTextCursor(cursor);
}

void Rofiapp_MainWindow::angleAlphaBetaDial_changed(int value) {
    if (!(partSelected && selectedPartType == ModelPartType::SHOE)) {
        return;
    }

    if (angleDialMoving) {
        activeCursor.joinPreviousEditBlock();
    } else {
        activeCursor.beginEditBlock();
        angleDialMoving = true;
    }
    findShoeInCode(selectedModuleId, selectedShoeId).insertText(QString("%1").arg(value));
    activeCursor.endEditBlock();
}

void Rofiapp_MainWindow::angleGammaDial_changed(int value) {
    if (!(partSelected && selectedPartType == ModelPartType::BODY)) {
        return;
    }

    if (angleDialMoving) {
        activeCursor.joinPreviousEditBlock();
    } else {
        activeCursor.beginEditBlock();
        angleDialMoving = true;
    }
    findBodyInCode(selectedModuleId).insertText(QString("%1").arg(value));
    activeCursor.endEditBlock();
}

void Rofiapp_MainWindow::angleDial_released() {
    angleDialMoving = false;
}

void setConnectorChecked(Ui::Rofiapp_MainWindow* ui, bool value) {
    for (QWidget* widget : std::initializer_list<QWidget*>{ui->connectedLabel, ui->connectedCheckBox,}) {
        widget->setEnabled(value);
        if (value) {
            widget->setToolTip(nullptr);
        } else {
            widget->setToolTip("Re-connecting not currently supported.");
        }
        widget->setVisible(true);
    }
}

void Rofiapp_MainWindow::connectedCheckBox_toggled(bool value) {
    if (!(partSelected && selectedPartType == ModelPartType::CONNECTOR)) {
        return;
    }

    if (!value) {
        findConnectorInCode(selectedModuleId, selectedShoeId, selectedConnectorId).removeSelectedText();
    } else {
        std::cerr << "Creating connections is not currently supported." << std::endl;
    }

    setConnectorChecked(this->ui, value);
}

void Rofiapp_MainWindow::onPartSelected(vtkObject *vtkNotUsed(caller), long unsigned int vtkNotUsed(eventId), void *clientData, void *callData) {
    Rofiapp_MainWindow *self = static_cast<Rofiapp_MainWindow *>(clientData);

    self->ui->connectedLabel->setVisible(false);
    self->ui->connectedCheckBox->setVisible(false);
    self->ui->angleAlphaLabel->setVisible(false);
    self->ui->angleAlphaDial->setVisible(false);
    self->ui->angleBetaLabel->setVisible(false);
    self->ui->angleBetaDial->setVisible(false);
    self->ui->angleGammaLabel->setVisible(false);
    self->ui->angleGammaDial->setVisible(false);

    self->partSelected = (callData != nullptr);

    if (!self->partSelected) {
        return;
    }

    vtkSmartPointer<vtkActor> pickedActor = vtkActor::SafeDownCast(static_cast<vtkObject *>(callData));

    vtkSmartPointer<vtkInformation> info = pickedActor->GetPropertyKeys();

    if (!info) {
        // Test sphere does not have info.
        return;
    }

    self->selectedModuleId = info->Get(VtkSupp::moduleIdKey);
    self->selectedPartType = static_cast<ModelPartType>(info->Get(VtkSupp::partTypeKey));

    switch (self->selectedPartType) {
    case ModelPartType::BODY: {
        QTextCursor cursor = self->findBodyInCode(self->selectedModuleId);
        self->setActiveCursor(cursor);
        self->ui->angleGammaDial->setValue(cursor.selectedText().toInt());
        self->ui->angleGammaLabel->setVisible(true);
        self->ui->angleGammaDial->setVisible(true);
    } break;
    case ModelPartType::SHOE: {
        self->selectedShoeId = static_cast<ShoeId>(info->Get(VtkSupp::shoeIdKey));
        QTextCursor cursor = self->findShoeInCode(self->selectedModuleId, self->selectedShoeId);
        self->setActiveCursor(cursor);
        if (self->selectedShoeId == ShoeId::A) {
            self->ui->angleAlphaDial->setValue(cursor.selectedText().toInt());
            self->ui->angleAlphaLabel->setVisible(true);
            self->ui->angleAlphaDial->setVisible(true);
        } else {
            self->ui->angleBetaDial->setValue(cursor.selectedText().toInt());
            self->ui->angleBetaLabel->setVisible(true);
            self->ui->angleBetaDial->setVisible(true);
        }
    } break;
    case ModelPartType::CONNECTOR: {
        self->selectedShoeId = static_cast<ShoeId>(info->Get(VtkSupp::shoeIdKey));
        self->selectedConnectorId = static_cast<ConnectorId>(info->Get(VtkSupp::connectorIdKey));
        QTextCursor cursor = self->findConnectorInCode(self->selectedModuleId, self->selectedShoeId, self->selectedConnectorId);
        self->setActiveCursor(cursor);
        setConnectorChecked(self->ui, !cursor.isNull());
        self->ui->connectedCheckBox->setChecked(!cursor.isNull());
    } break;
    }
}
