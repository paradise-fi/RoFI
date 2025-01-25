#pragma once

#include <memory>
#include "../explorer/explorer.h"
#include "../moduleConsole/moduleconsole.h"
#include "../selectProgram/selectprogram.h"
#include <QAbstractButton>
#include <QMainWindow>

#include <QProcess>

QT_BEGIN_NAMESPACE
namespace Ui 
{
    class RofiRun;
}
QT_END_NAMESPACE

class RofiRun : public QMainWindow
{
    Q_OBJECT

public:
    RofiRun( QWidget *parent = nullptr );
    ~RofiRun();

private slots:
    void loadConfiguration();
    void selectConfiguration();
    void selectProgram( bool );
    void selectFilter( QAbstractButton* button );

private:
    void runSimulator( std::string simplesim_path );
    void runModules( int delay, int count, bool printDetailed );
    void stopRunningSimulation();
    
    QString _config_path;
    QString _program_path;
    QString _filter_path;
    std::unique_ptr< Ui::RofiRun > ui;
    std::unique_ptr< Explorer > _explorer;
    std::unique_ptr< SelectProgram > _selector;

    std::unique_ptr< QProcess > _simProcess;
    std::vector< std::unique_ptr< ModuleConsole > > _moduleProcesses;

    void setUpRadioButton();
    void openExplorer( QString& result, std::string acceptedExtension );
};
