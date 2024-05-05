#include "runmodules.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>

#include "ui_runmodules.h"

using rofi::simplesim::RunModules;

RunModules::RunModules( bool& isRunning, QWidget* parent, std::size_t moduleCount )
    : QWidget( parent ), _ui( std::make_unique< Ui::RunModules >() ), 
      _moduleCount( moduleCount ), _isRunning( isRunning )
{
    _ui->setupUi( this );

    this->setWindowFlags( Qt::Window );
    connect( _ui->buttonBox, SIGNAL( rejected() ), this, SLOT( hide() ) );
    connect( _ui->buttonBox, SIGNAL( accepted() ), this, SLOT( accept() ) );
}

RunModules::~RunModules() {}

void RunModules::accept()
{
    hide();
    std::string program = _ui->textEdit->toPlainText().toStdString();
    _ui->textEdit->setPlainText( "" );

    std::string simplesimPath = std::string(std::getenv("ROFI_BUILD_DIR")) + "/desktop/bin/rofi-simplesim";
    std::string programPath = std::string(std::getenv("ROFI_BUILD_DIR")) + "/desktop/bin/" + program;
    std::cout << "Running " << programPath << " for " << _moduleCount << " modules in the simulation." << std::endl;
    _isRunning = true;

    const char* executable = programPath.c_str();

    for ( std::size_t i = 0; i < _moduleCount; ++i ) 
    {
        std::cout << "Running module " << i << std::endl;
        pid_t module_id = fork();
        if ( module_id == -1 )
        {
            std::cerr << "Failed to run module: " << std::strerror( errno ) << std::endl;
            return;
        }

        if ( module_id == 0 )
        {
            if ( execl( executable, executable, static_cast< char* >( 0 ) ) == -1 ) {
                std::cerr << "Failed to run module: " << std::strerror( errno ) << std::endl;
            }
        }
    }
}