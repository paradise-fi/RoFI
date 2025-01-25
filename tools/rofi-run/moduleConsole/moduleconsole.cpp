#include "moduleconsole.h"

#include <QStringList>

#include "ui_moduleconsole.h"

// moduleId currently unused, need to find a way on how to pass a true id to the console.
ModuleConsole::ModuleConsole( 
    int moduleId, bool readErr,
    QWidget* parent )
    : QWidget( parent ),
      _ui( std::make_unique< Ui::ModuleConsole >() ),
      _moduleProcess( std::make_unique< QProcess >( this ) )
{
    _ui->setupUi( this );

    // connect signal for readReady
    connect( _moduleProcess.get(), SIGNAL ( readyReadStandardOutput() ), this, SLOT (readProcessOut() ) );
    if ( readErr )
    {
        connect( _moduleProcess.get(), SIGNAL ( readyReadStandardError() ), this, SLOT(readProcessErr() ) );
    }
}

ModuleConsole::~ModuleConsole() {}

void ModuleConsole::runProgram( QString program )
{
    QStringList arguments;
    _moduleProcess->start( program, arguments );
}

void ModuleConsole::readProcessOut()
{
    _ui->outputField->append(_moduleProcess->readAllStandardOutput());
    // _ui->outputField->setPlainText("FEE\n");
}

void ModuleConsole::readProcessErr()
{
    _ui->outputField->append(_moduleProcess->readAllStandardError());
}