#include "changecolor.hpp"

#include <iostream>

#include "ui_changecolor.h"


using rofi::simplesim::ChangeColor;

ChangeColor::ChangeColor( QWidget * parent, size_t size )
        : _ui( std::make_unique< Ui::ChangeColor >() ), _parent( parent ), _toColor( size, false )
{
    _ui->setupUi( this );
    _ui->plainTextEdit->hide();

    connect( _ui->buttonBox, SIGNAL( rejected() ), this, SLOT( hide() ) );
    connect( _ui->buttonBox, SIGNAL( accepted() ), this, SLOT( accept() ) );
    connect( _ui->toolButton, SIGNAL( clicked() ), this, SLOT( toggleHelp() ) );
}

ChangeColor::~ChangeColor() {}

struct syntaxError : std::exception {};
struct rangeError : std::exception {};

void ChangeColor::accept()
{
    hide();
    std::fill( _toColor.begin(), _toColor.end(), false );
    try {
        parseInput();
    } catch ( syntaxError & e ) {
        std::cerr << "Couldn't parse input\n";
    } catch ( rangeError & e ) {
        std::cerr << "Given module out of range\n";
    }

    _ui->textEdit->setPlainText( "" );

    int color = _ui->listWidget->currentRow();
    if ( color == -1 ) {
        std::cerr << "No color selected\n";
        return;
    }
    emit pickedColor( color );
}

void ChangeColor::toggleHelp()
{
    if ( _showingHelp ) {
        _ui->plainTextEdit->hide();
    } else {
        _ui->plainTextEdit->show();
    }
    _showingHelp = !_showingHelp;
}

void ChangeColor::parseInput()
{
    QString source = _ui->textEdit->toPlainText();
    int i = 0;
    int number = -1;

    auto parseNum = [ & ]() {
        std::string buffer;
        if ( !source[ i ].isDigit() ) {
            throw syntaxError();
        }
        while ( i < source.size() && source[ i ].isDigit() ) {
            buffer += source[ i++ ].toLatin1();
        }
        return std::stoi( buffer );
    };
    auto parseNext = [ & ]() {
        _toColor[ number ] = true;
        if ( source[ i++ ] == ',' ) {
            number = parseNum();
        }
    };
    auto parseRange = [ & ]() {
        if ( source[ i++ ] != '.' || source[ i++ ] != '.' ) {
            throw syntaxError();
        }
        int upper = parseNum();
        if ( upper >= static_cast< int >( _toColor.size() ) ) {
            throw syntaxError();
        }
        for ( int j = number; j <= upper; j++ ) {
            _toColor[ j ] = true;
        }
    };
    number = parseNum();
    while ( i < source.size() ) {
        if ( source[ i ] == ',' ) {
            parseNext();
        }
        if ( source[ i ] == '.' ) {
            parseRange();
        }
    }
    if ( number < 0 || to_unsigned( number ) >= _toColor.size() ) {
        throw rangeError();
    }
    _toColor[ number ] = true;
}
