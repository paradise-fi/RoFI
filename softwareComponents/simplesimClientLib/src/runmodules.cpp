#include "runmodules.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>

#include "ui_runmodules.h"

using rofi::simplesim::RunModules;

RunModules::RunModules( QWidget* parent )
    : QDialog( parent ), _ui( std::make_unique< Ui::RunModules >() ) 
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
    _program = _ui->textEdit->toPlainText().toStdString();
    _ui->textEdit->setPlainText( "" );

    done( QDialog::Accepted );
}

std::string RunModules::getProgram()
{
    return _program;
}