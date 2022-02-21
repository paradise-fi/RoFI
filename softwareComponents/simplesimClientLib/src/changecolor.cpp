#include "changecolor.hpp"
#include "ui_changecolor.h"

#include <iostream>

using rofi::simplesim::ChangeColor;

ChangeColor::ChangeColor( QWidget* parent, size_t size ) :
    ui( std::make_unique< Ui::ChangeColor >() ),
    parent( parent ),
    toColor( size, false )
{
    ui->setupUi( this );
    ui->plainTextEdit->hide();

    connect( ui->buttonBox, SIGNAL( rejected() ), this, SLOT( hide() ) );
    connect( ui->buttonBox, SIGNAL( accepted() ), this, SLOT( accept() ) );
    connect( ui->toolButton, SIGNAL( clicked() ), this, SLOT( showHelp() ) );
}

ChangeColor::~ChangeColor(){}

struct syntaxError : std::exception {};
struct rangeError : std::exception {};

void ChangeColor::accept(){
    hide();
    std::fill( toColor.begin(), toColor.end(), false );
    try {
        parseInput();
    } catch( syntaxError& e ){
        std::cerr << "Couldn't parse input\n";
    } catch( rangeError& e ){
        std::cerr << "Given module out of range\n";
    }

    ui->textEdit->setPlainText( "" );

    int color = ui->listWidget->currentRow();
    if( color == -1 ){
        std::cerr << "No color selected\n";
        return;
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

void ChangeColor::parseInput(){
    QString source = ui->textEdit->toPlainText();
    int i = 0;
    int number = -1;

    auto parseNum = [&](){
        std::string buffer;
        if( !source[ i ].isDigit() ){
            throw syntaxError();
        }
        while( i < source.size() && source[ i ].isDigit() ){
            buffer += source[ i++ ].toLatin1();
        }
        return std::stoi( buffer );
    };
    auto parseNext = [&](){
        toColor[ number ] = true;
        if( source[ i++ ] == ',' ){
            number = parseNum();
        }
    };
    auto parseRange = [&](){
        if( source[ i++ ] != '.' || source[ i++ ] != '.' ){
            throw syntaxError();
        }
        int upper = parseNum();
        if( upper >= static_cast< int >( toColor.size() ) ){
            throw syntaxError();
        }
        for( int j = number; j <= upper; j++ ){
            toColor[ j ] = true;
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
    if( number < 0 || to_unsigned( number ) >= toColor.size() ){
        throw rangeError();
    }
    toColor[ number ] = true;
}
