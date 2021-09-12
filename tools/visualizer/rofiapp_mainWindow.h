#ifndef ROFIAPP_MAINWINDOW_H
#define ROFIAPP_MAINWINDOW_H

#include "Vtk.h"
#include "InteractorStyle.hpp"

#include <QMainWindow>
#include <QMessageBox>
#include <QErrorMessage>
#include <QFileDialog>
#include <QDebug>
#include <QRegularExpression>
#include <QTextCursor>

#include <CompatQVTKWidget.h>

#include <vtkCallbackCommand.h>

#include <string.h>
#include <Configuration.h>
#include <IO.h>
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

public slots:
    void loadConfFile(QFile &file);

private slots:
    void showSphere();
    void changeBackground();
    void toggleFullScreen();

    void loadConf();
    void showConf();
    void resetCamera();
    void on_configTextWindow_textChanged();

    void saveConf();

    QTextCursor findRegex(QRegularExpression regex);
    QTextCursor findBodyInCode(ID moduleId);
    QTextCursor findShoeInCode(ID moduleId, ShoeId shoe);
    QTextCursor findConnectorInCode(ID moduleId, ShoeId shoe, ConnectorId connId);

    void setActiveCursor(QTextCursor cursor);

    void angleAlphaBetaDial_changed(int value);
    void angleGammaDial_changed(int value);
    void angleDial_released();
    void connectedCheckBox_toggled(bool value);

private:
    Ui::Rofiapp_MainWindow *ui;
    float bckgValue;
    bool fullScreen;

    Configuration *current_cfg;
    bool check_cfg(bool update_current_cfg);

#if (VTK_MAJOR_VERSION == 8 && VTK_MINOR_VERSION >= 2) || VTK_MAJOR_VERSION > 8
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindow;
#else
    vtkSmartPointer<vtkRenderWindow> renderWindow;
#endif

    bool partSelected = false;
    ModelPartType selectedPartType;
    ID selectedModuleId;
    ShoeId selectedShoeId;
    ConnectorId selectedConnectorId;

    /**
     * Currently active cursor in the text editor.
     *
     * Used to group text editing operations together
     * into an edit block so that undo operation reverts
     * the text to the previous state instead of just
     * the previous step of dragging the dial.
     */
    QTextCursor activeCursor;
    /**
     * Whether the user is currently dragging the angle dial.
     * Used to group text editing operations together.
     */
    bool angleDialMoving = false;

    vtkSmartPointer<vtkRenderer> renderer;
    vtkSmartPointer<InteractorStyle> interactorStyle;
    vtkSmartPointer<vtkCamera> camera;

    static void onPartSelected(vtkObject *vtkNotUsed(caller), long unsigned int vtkNotUsed(eventId), void *clientData, void *callData);
};

#endif // ROFIAPP_MAINWINDOW_H
