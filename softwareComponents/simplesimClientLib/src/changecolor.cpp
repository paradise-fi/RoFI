#include "changecolor.hpp"
#include "ui_changecolor.h"

#include <iostream>

using rofi::simplesim::ChangeColor;

ChangeColor::ChangeColor( QWidget* parent, size_t size ) :
    ui( new Ui::ChangeColor ),
    parent( parent ),
    to_color( size, false )
{
    //QWidget( parent );
    ui->setupUi( this );
    ui->plainTextEdit->hide();

    connect( ui->buttonBox, SIGNAL( rejected() ), this, SLOT( hide() ) );
    connect( ui->buttonBox, SIGNAL( accepted() ), this, SLOT( accept() ) );
    connect( ui->toolButton, SIGNAL( clicked() ), this, SLOT( showHelp() ) );
}

ChangeColor::~ChangeColor(){
    delete ui;
}

void ChangeColor::accept(){
    hide();
    try {
        parseRange();
    } catch ( std::exception& e ){
        std::cout << "couldn't parse input\n";
        QString empty;
        ui->textEdit->setPlainText( empty );
    }

    int color = ui->listWidget->currentRow();
    if( color == -1 ){
        std::cout << "no color selected\n";
    }
    emit pickedColor( color );
}

void ChangeColor::showHelp(){
    if( showingHelp ){
        ui->plainTextEdit->hide();
    } else {
        ui->plainTextEdit->show();
    }
    showingHelp = !showingHelp;
}
void ChangeColor::parseRange(){
    QString source = ui->textEdit->toPlainText();
    int i = 0;
    int number = -1;

    auto parseNum = [&](){
        std::string buffer;
        if( !source[ i ].isDigit() ){
            throw std::exception();
        }
        while( i < source.size() && source[ i ].isDigit() ){
            buffer += source[ i++ ].toLatin1();
        }
        return std::stoi( buffer );
    };
    auto parseNext = [&](){
        to_color[ number ] = true;
        if( source[ i++ ] == ',' ){
            number = parseNum();
        }
    };
    auto parseRange = [&](){
        if( source[ i++ ] != '.' || source[ i++ ] != '.' ){
            throw std::exception();
        }
        int upper = parseNum();
        if( upper >= static_cast< int >( to_color.size() ) ){
            throw std::exception();
        }
        for( int j = number; j <= upper; j++ ){
            to_color[ j ] = true;
        }
    };
    number = parseNum();
    while( i < source.size() ){
        if( source[ i ] == ',' ){
            parseNext();
        }
        if( source[ i ] == '.' ){
            parseRange();
        }
    }
    if( number < 0 || number > static_cast< int >( to_color.size() ) ){
        throw std::exception();
    }
    to_color[ number ] = true;
}
