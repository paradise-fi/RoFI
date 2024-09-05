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
    ui->setupUi(this);
    connect( ui->configSelectButton, SIGNAL( released() ), this, SLOT( selectConfiguration() ) );
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

    std::string filter = "";
    if ( !_filter_path.isEmpty() )
    {
        filter = "-p" + _filter_path.toStdString();
    }

    std::string simplesim_path = std::string( std::getenv( "ROFI_BUILD_DIR" ) ) 
                                + "/desktop/bin/rofi-simplesim";
                                
    if ( execl( simplesim_path.c_str(),
         simplesim_path.c_str(), 
         _config_path.toStdString().c_str(),
         filter.c_str(),
         static_cast< char* >( 0 ) ) )
    {
        std::cerr << "Failed to launch simplesim: " << std::strerror( errno ) << std::endl; 
    }
}

void RofiRun::selectConfiguration()
{
    openExplorer(_config_path, ".json");
    ui->configEdit->setText(_config_path);
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
