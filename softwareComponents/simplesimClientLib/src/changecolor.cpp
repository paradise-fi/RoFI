#include "changecolor.hpp"

#include <iostream>

#include "ui_changecolor.h"


using rofi::simplesim::ChangeColor;

ChangeColor::ChangeColor( QWidget * parent, size_t size )
        : QWidget( parent ), _ui( std::make_unique< Ui::ChangeColor >() ), _toColor( size, false )
{
    _ui->setupUi( this );
    _ui->plainTextEdit->hide();

    this->setWindowFlags( Qt::Window );
    connect( _ui->buttonBox, SIGNAL( rejected() ), this, SLOT( hide() ) );
    connect( _ui->buttonBox, SIGNAL( accepted() ), this, SLOT( accept() ) );
    connect( _ui->toolButton, SIGNAL( clicked() ), this, SLOT( toggleHelp() ) );
}

ChangeColor::~ChangeColor() {}

void ChangeColor::accept()
{
    hide();
    std::fill( _toColor.begin(), _toColor.end(), false );
    try {
        auto idxs = parseModuleIdxs( _ui->textEdit->toPlainText() );
        for ( size_t idx : idxs ) {
            assert( idx < _toColor.size() );
            _toColor[ idx ] = true;
        }
    } catch ( const std::invalid_argument & e ) {
        std::cerr << "Couldn't parse input (" << e.what() << ")\n";
        return;
    } catch ( const std::out_of_range & e ) {
        std::cerr << "Input out of range (" << e.what() << ")\n";
        return;
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

std::set< size_t > ChangeColor::parseModuleIdxs( const QString & source ) const
{
    auto parseUInt = []( QStringRef numStr ) {
        bool ok = true;
        auto result = numStr.trimmed().toUInt( &ok );
        if ( !ok ) {
            throw std::invalid_argument( "expected number (uint)" );
        }
        return result;
    };

    auto result = std::set< size_t >();
    for ( QStringRef part : source.splitRef( ',', Qt::SkipEmptyParts ) ) {
        if ( part.contains( ".." ) ) {
            auto numberStrings = part.split( ".." );
            if ( numberStrings.size() != 2 ) {
                throw std::invalid_argument( "expected at most one '..' in range" );
            }
            auto start = parseUInt( numberStrings[ 0 ] );
            auto end = parseUInt( numberStrings[ 1 ] );

            if ( start > end ) {
                throw std::invalid_argument( "invalid range" );
            }
            if ( end >= _toColor.size() ) {
                throw std::out_of_range( "module range out of range" );
            }

            for ( auto i = start; i <= end; i++ ) {
                result.insert( i );
            }
        } else {
            auto idx = parseUInt( part );
            if ( idx >= _toColor.size() ) {
                throw std::out_of_range( "module out of range" );
            }
            result.insert( idx );
        }
    }
    return result;
}
