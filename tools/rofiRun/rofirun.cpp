#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include "rofirun.h"
#include "./ui_rofirun.h"

RofiRun::RofiRun( QWidget *parent )
    : QMainWindow( parent )
    , ui( std::make_unique< Ui::RofiRun >() )
{
    ui->setupUi(this);
    connect( ui->loadButton, SIGNAL( released() ), this, SLOT( loadConfiguration() ) );
}

void RofiRun::loadConfiguration()
{
    std::string config_path = ui->configEdit->text().toStdString();
    std::string filter_path = ui->filterEdit->text().toStdString();
    std::string simplesimPath = std::string( std::getenv( "ROFI_BUILD_DIR" ) ) 
                                + "/desktop/bin/rofi-simplesim";
    const char* executable = simplesimPath.c_str();
    std::vector< const char* > args;
    args.push_back( executable );
    args.push_back( config_path.c_str() );
    if ( filter_path.size() > 0 ) {
        args.push_back( filter_path.c_str() );
    }
    // TODO: Use QProcess to launch instead.
    std::cout << "Loading config " << config_path.c_str() << " with filter: " << filter_path << std::endl;
    if ( execl( executable, executable, config_path.c_str(), static_cast< char * >( 0 ) ) == -1 ) {
        std::cerr << "Failed to launch simplesim: " << std::strerror( errno ) << std::endl; 
    }
}

RofiRun::~RofiRun() {}
