#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <QMessageBox>
#include "rofirun.h"
#include "./ui_rofirun.h"

RofiRun::RofiRun( QWidget *parent )
    : QMainWindow( parent )
    , ui( std::make_unique< Ui::RofiRun >() )
{
    ui->setupUi( this );
    connect( ui->configSelectButton, SIGNAL( released() ), this, SLOT( selectConfiguration() ) );
    connect( ui->programSelectButton, SIGNAL( clicked( bool ) ), this, SLOT( selectProgram( bool ) ) );
    connect( ui->loadButton, SIGNAL( released() ), this, SLOT( loadConfiguration() ) );
    setUpRadioButton();
}

void RofiRun::setUpRadioButton()
{
    ui->buttonGroup->addButton(ui->radioNoFilter);
    ui->buttonGroup->setId(ui->radioNoFilter, 0);
    ui->buttonGroup->addButton(ui->radio25Reliability);
    ui->buttonGroup->setId(ui->radio25Reliability, 25);
    ui->buttonGroup->addButton(ui->radio50Reliability);
    ui->buttonGroup->setId(ui->radio50Reliability, 50);
    ui->buttonGroup->addButton(ui->radio75Reliability);
    ui->buttonGroup->setId(ui->radio75Reliability, 75);
    ui->buttonGroup->addButton(ui->radioCustomFilter);
    ui->buttonGroup->setId(ui->radioCustomFilter, 1);
    connect( ui->buttonGroup, SIGNAL( buttonClicked( QAbstractButton* ) ), this, SLOT( selectFilter( QAbstractButton* ) ) );
}

void RofiRun::runSimulator( std::string simplesim_path )
{
    std::string filter = "";
    if ( !_filter_path.isEmpty() )
    {
        filter = "-p" + _filter_path.toStdString();
    }

    QStringList sim_arguments;
    sim_arguments << _config_path;

    _simProcess = std::make_unique< QProcess >( this );

    _simProcess->start( QString::fromStdString( simplesim_path ), sim_arguments );
}

void RofiRun::runModules( int delay, int count, bool printDetailed )
{
    for (auto i = 0; i < count; ++i)
    {
        _moduleProcesses.push_back( std::make_unique< ModuleConsole >( i, printDetailed, nullptr ) );
        _moduleProcesses[ i ]->show();
        _moduleProcesses[ i ]->runProgram( _program_path );
        sleep( delay );
    }
}

void RofiRun::stopRunningSimulation()
{
    for( auto it = _moduleProcesses.begin(); it != _moduleProcesses.end(); ++it)
    {
        it->reset( nullptr );
    }

    while( !_moduleProcesses.empty() )
    {
        _moduleProcesses.pop_back();
    }

}

void RofiRun::loadConfiguration()
{
    if ( _config_path.isEmpty() )
    {
        QMessageBox::critical
        (
            this,
            tr( "No configuration selected!" ),
            tr( "You need to select a configuration that will be loaded." )
        );
        return;
    }

    stopRunningSimulation();

    std::string simplesim_path = std::string( std::getenv( "ROFI_BUILD_DIR" ) ) 
                                + "/desktop/bin/rofi-simplesim";

    runSimulator( simplesim_path );
    int simulationSleepDelay = ui->simulationDelay->value();
    sleep(simulationSleepDelay);

    QStringList arguments;
    int modulesDelay = ui->moduleDelay->value();
    int moduleCount = ui->moduleCount->value();
    bool printDetailed = ui->detailedBox->isChecked();

    runModules( modulesDelay, moduleCount, printDetailed );
}

void RofiRun::selectConfiguration()
{
    openExplorer( _config_path, ".json");
    ui->configEdit->setText(_config_path);
}

void RofiRun::selectProgram( bool )
{
    QDir directory( QString::fromStdString( std::string( std::getenv( "ROFI_BUILD_DIR" ) ) + "/desktop/bin/" ) );
    std::vector< std::string > executables;

    if ( !_selector )
    {
        QString executablesDirPath =  QString::fromStdString( std::getenv( "ROFI_BUILD_DIR" ) ) + "/desktop/bin/";
        _selector = std::make_unique< SelectProgram >( executablesDirPath, this );
    }

    if ( _selector->exec() != QDialog::Accepted )
    {
        return;
    }

    _program_path = _selector->programPath();
    ui->programEdit->setText( _program_path );
}

void RofiRun::selectFilter( QAbstractButton* button )
{
    std::cout << button->text().toStdString() << std::endl;
    const int id = ui->buttonGroup->checkedId();
    const std::string filter_directory = std::string( std::getenv( "ROFI_ROOT" ) ) + "/tools/rofi-run/filters/";
    
    switch (id) {
        case 1:
            openExplorer( _filter_path, ".py" );
            return; 
        case 25:
            _filter_path = QString::fromStdString(filter_directory + "reliable25.py");
            break;
        case 50:
            _filter_path = QString::fromStdString(filter_directory + "reliable50.py");
            break;
        case 75:
            _filter_path = QString::fromStdString(filter_directory + "reliable75.py");
            break;
        default:
            _filter_path = QString::fromStdString("");;
            break;
    }
}

void RofiRun::openExplorer( QString& result, std::string acceptedExtension )
{
    if ( !_explorer ) {
        _explorer = std::make_unique< Explorer >( acceptedExtension, this );
    } else {
        _explorer->clearPath();
        _explorer->setAcceptedExtension( acceptedExtension );
    }

    if ( _explorer->exec() == QDialog::Accepted )
    {
        result = _explorer->getPath();
    }
}

RofiRun::~RofiRun() {}
